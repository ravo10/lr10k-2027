// Checks the optical extensometer's marker detection with the real OpenCV, on drawn camera frames.
//
// DetectNew() repeats the camera thread's steps (ExtensiometerWorkerThread in appstructs.cpp): the working copy
// (at most 1280 px wide), crop, blur, HSV, inRange, erode/dilate, findContours, then the sub-pixel centre in the
// full-resolution frame (ExtensometerCentroid.h, CameraResolution.h). Keep it the same when that code changes.
// DetectOld() is the code before: everything on the whole frame, the centre from the outline's moments.
//
// The same scene is drawn at 800 x 600, 1920 x 1080 and 3840 x 2160 (dot size, positions and movement scale with
// the resolution) while one marker moves as in a test. Checked: the strain error, and the time per frame.
//
//   g++ -std=c++17 -O2 -I main/LloydLR10KTensile tests/extensometer_pipeline_check.cpp -o extensometer_pipeline_check $(pkg-config --cflags --libs opencv4)
//   ./extensometer_pipeline_check
//
// Run by .github/workflows/tests.yml (job "extensometer-opencv").

#include "CameraResolution.h"
#include "ExtensometerCentroid.h"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

constexpr int kHMin = 25, kHMax = 85, kSMin = 110, kVMin = 50;

struct Result { bool ok; double dist; int fallbacks = 0; }; // fallbacks: blobs whose sub-pixel centre failed

static double Distance(const std::vector<cv::Point2f>& c) { return c.size() == 2 ? cv::norm(c[0] - c[1]) : 0.0; }

// Before: the whole frame, the centre from the outline of the grown mask
static Result DetectOld(const cv::Mat& frameIn)
{
    cv::Mat frame = frameIn.clone(), hsv, mask;
    cv::GaussianBlur(frame, frame, cv::Size(5, 5), 0);
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(kHMin, kSMin, kVMin), cv::Scalar(kHMax, 255, 255), mask);
    cv::erode(mask, mask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
    cv::dilate(mask, mask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(8, 8)));
    cv::Mat display; cv::cvtColor(frameIn, display, cv::COLOR_BGR2RGBA); // The display copy (full size before)
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    std::vector<cv::Point2f> centers;
    for (const auto& c : contours)
        if (cv::contourArea(c) > 50) { const cv::Moments m = cv::moments(c); centers.push_back(cv::Point2f((float)(m.m10 / m.m00), (float)(m.m01 / m.m00))); }
    return { centers.size() == 2, Distance(centers), 0 };
}

// Now: as in the camera thread
static Result DetectNew(const cv::Mat& frame)
{
    const double scale = CameraResolution::WorkScale(frame.cols);
    cv::Mat work;
    if (scale > 1.0) cv::resize(frame, work, cv::Size(), 1.0 / scale, 1.0 / scale, cv::INTER_AREA);
    else work = frame;
    cv::Mat display; cv::cvtColor(work, display, cv::COLOR_BGR2RGBA); // The display copy (working size)

    const int margin = 0;
    const cv::Rect active_roi(margin, 0, work.cols - 2 * margin, work.rows);
    cv::Mat cropped_frame, cropped_hsv, cropped_mask;
    cv::GaussianBlur(work(active_roi), cropped_frame, cv::Size(5, 5), 0);
    cv::cvtColor(cropped_frame, cropped_hsv, cv::COLOR_BGR2HSV);
    cv::inRange(cropped_hsv, cv::Scalar(kHMin, kSMin, kVMin), cv::Scalar(kHMax, 255, 255), cropped_mask);
    cv::erode(cropped_mask, cropped_mask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
    cv::dilate(cropped_mask, cropped_mask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(8, 8)));
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(cropped_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    ExtensometerCentroid::Options options;
    options.hMin = kHMin;
    options.hMax = kHMax;
    std::vector<cv::Point2f> centers;
    int fallbacks = 0;
    for (const auto& c : contours)
    {
        if (cv::contourArea(c) <= 50) continue;
        const cv::Rect box = cv::boundingRect(c);
        const CameraResolution::Rect area = CameraResolution::ToFullResolution(
            CameraResolution::Rect{ box.x + margin, box.y, box.width, box.height }, scale, 3, frame.cols, frame.rows);
        if (area.width >= 3 && area.height >= 3)
        {
            const cv::Rect search(area.x, area.y, area.width, area.height);
            cv::Mat patch, patchHsv;
            cv::GaussianBlur(frame(search), patch, cv::Size(5, 5), 0);
            cv::cvtColor(patch, patchHsv, cv::COLOR_BGR2HSV);
            std::vector<cv::Point> outline;
            for (const cv::Point& p : c)
                outline.push_back(cv::Point((int)std::lround(CameraResolution::WorkToFull(p.x + margin, scale)) - search.x,
                                            (int)std::lround(CameraResolution::WorkToFull(p.y, scale)) - search.y));
            cv::Mat blobMask = cv::Mat::zeros(search.size(), CV_8UC1);
            cv::drawContours(blobMask, std::vector<std::vector<cv::Point>>{ outline }, 0, cv::Scalar(255), cv::FILLED);
            const ExtensometerCentroid::Centre centre = ExtensometerCentroid::WeightedCentre(patchHsv.data, (int)patchHsv.step, patchHsv.cols, patchHsv.rows,
                blobMask.data, (int)blobMask.step, ExtensometerCentroid::Rect{ 0, 0, patchHsv.cols, patchHsv.rows }, options);
            if (centre.ok) { centers.push_back(cv::Point2f((float)(centre.x + search.x), (float)(centre.y + search.y))); continue; }
        }
        ++fallbacks;
        const cv::Moments m = cv::moments(c);
        centers.push_back(cv::Point2f((float)CameraResolution::WorkToFull(m.m10 / m.m00 + margin, scale), (float)CameraResolution::WorkToFull(m.m01 / m.m00, scale)));
    }
    return { centers.size() == 2, Distance(centers), fallbacks };
}

// Grey frame with two green dots, anti-aliased (each dot drawn 8x larger in its own patch, then averaged down)
static cv::Mat Frame(int width, int height, const cv::Point2d dots[2], double radius, double noise, double shade)
{
    cv::Mat frame(height, width, CV_8UC3, cv::Scalar(120, 120, 120));
    const int k = 8;
    for (int d = 0; d < 2; ++d)
    {
        const int half = (int)std::ceil(radius) + 4;
        const int x0 = (int)std::floor(dots[d].x) - half, y0 = (int)std::floor(dots[d].y) - half, size = 2 * half + 1;
        cv::Mat big(size * k, size * k, CV_8UC3, cv::Scalar(120, 120, 120));
        const double cx = (dots[d].x - x0) * k, cy = (dots[d].y - y0) * k;
        cv::circle(big, cv::Point((int)std::lround(cx * 16), (int)std::lround(cy * 16)), (int)std::lround(radius * k * 16), cv::Scalar(60, 200, 40), cv::FILLED, cv::LINE_AA, 4);
        if (shade > 0)
        {
            cv::Mat right = big(cv::Rect((int)cx, 0, big.cols - (int)cx, big.rows)), m;
            cv::inRange(right, cv::Scalar(0, 150, 0), cv::Scalar(255, 255, 255), m);
            cv::Mat darker; right.convertTo(darker, -1, 1.0 - shade); darker.copyTo(right, m);
        }
        cv::Mat small; cv::resize(big, small, cv::Size(size, size), 0, 0, cv::INTER_AREA);
        small.copyTo(frame(cv::Rect(x0, y0, size, size)));
    }
    if (noise > 0)
    {
        cv::Mat n(frame.size(), CV_16SC3), f16;
        cv::randn(n, 0, noise);
        frame.convertTo(f16, CV_16SC3);
        f16 += n;
        f16.convertTo(frame, CV_8UC3);
    }
    return frame;
}

struct Score { double rmsStrainPercent = 0, msPerFrame = 0, worstPercent = 0; int lost = 0, fallbacks = 0; };

template <class Detect>
static Score Run(Detect detect, int width, int height, double noise, double shade, int frames)
{
    const double s = std::min(width / 800.0, height / 600.0); // The same scene at every resolution (fits 16:9 too)
    const double radius = 7.0 * s, l0 = 400.0 * s;     // Dots of 14 px and 400 px apart at 800 x 600
    Score score;
    double sum = 0.0, first = 0.0, time = 0.0;
    int n = 0;
    for (int i = 0; i < frames; ++i)
    {
        const double move = 0.05 * s * i;              // 0.05 px per frame at 800 x 600 (0.0125 % strain)
        const cv::Point2d dots[2] = { { 400.3 * s, 100.4 * s }, { 401.1 * s, (100.4 + 400.0) * s + move } };
        const cv::Mat frame = Frame(width, height, dots, radius, noise, shade);
        const auto t0 = std::chrono::steady_clock::now();
        const Result r = detect(frame);
        time += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        score.fallbacks += r.fallbacks;
        if (!r.ok) { ++score.lost; continue; }
        const double truth = std::hypot(dots[1].x - dots[0].x, dots[1].y - dots[0].y);
        if (n == 0) first = r.dist - truth;            // The fixed offset is L0's (set at the start), not an error
        const double error = (r.dist - truth - first) / l0 * 100.0;
        sum += error * error;
        score.worstPercent = std::max(score.worstPercent, std::fabs(error));
        ++n;
    }
    score.rmsStrainPercent = n > 0 ? std::sqrt(sum / n) : 1e9;
    score.msPerFrame = time / frames;
    return score;
}

int main()
{
    int failures = 0;
    cv::theRNG().state = 12345;
    cv::setNumThreads(1); // Comparable times
    std::printf("OpenCV %s\n", CV_VERSION);

    struct Size { int w, h; int frames; };
    const Size sizes[] = { { 800, 600, 81 }, { 1920, 1080, 41 }, { 3840, 2160, 21 } };
    double newStrainAt800 = 0.0;
    for (const Size& size : sizes)
        for (double noise : { 0.0, 3.0 })
            for (double shade : { 0.0, 0.2 })
            {
                const Score o = Run(DetectOld, size.w, size.h, noise, shade, size.frames);
                const Score n = Run(DetectNew, size.w, size.h, noise, shade, size.frames);
                std::printf("%4d x %4d  noise %.0f  shade %2.0f %%:  strain error old %.4f %%, new %.4f %%   time old %6.2f ms, new %6.2f ms per frame\n",
                    size.w, size.h, noise, shade * 100, o.rmsStrainPercent, n.rmsStrainPercent, o.msPerFrame, n.msPerFrame);
                std::printf("             new: worst frame %.4f %%, whole-pixel fallbacks %d of %d centres\n", n.worstPercent, n.fallbacks, 2 * size.frames);
                if (o.lost || n.lost) { std::printf("FAIL: markers lost (old %d, new %d frames)\n", o.lost, n.lost); ++failures; }
                // Better than before everywhere; without noise far better
                const bool better = noise == 0.0 ? n.rmsStrainPercent < 0.25 * o.rmsStrainPercent + 1e-4 : n.rmsStrainPercent < 0.7 * o.rmsStrainPercent;
                if (!better) { std::printf("FAIL: the new centre is not better enough here\n"); ++failures; }
                if (size.w == 800 && noise > 0 && shade == 0.0) newStrainAt800 = n.rmsStrainPercent;
                // At 4K: finer than at 800 x 600, and much faster than processing the whole frame
                if (size.w == 3840 && noise > 0 && shade == 0.0)
                {
                    if (!(n.rmsStrainPercent < newStrainAt800)) { std::printf("FAIL: 4K is not finer than 800 x 600\n"); ++failures; }
                    if (!(n.msPerFrame < 0.5 * o.msPerFrame)) { std::printf("FAIL: 4K is not faster than the whole-frame processing\n"); ++failures; }
                }
            }
    if (failures == 0)
        std::printf("extensometer_pipeline_check: all checks passed\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
