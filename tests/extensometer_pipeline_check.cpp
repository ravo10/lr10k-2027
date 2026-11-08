// Checks the optical extensometer's marker detection with the real OpenCV, on drawn camera frames.
//
// The steps are the camera thread's (ExtensiometerWorkerThread in appstructs.cpp: crop, blur, HSV, inRange,
// erode/dilate, findContours, then the centre): keep them the same when that code changes. Compared are the old
// centre (the outline's moments) and the new one (ExtensometerCentroid.h), while one marker moves 0.05 px per
// frame as in a test.
//
//   g++ -std=c++17 -O2 -I main/LloydLR10KTensile tests/extensometer_pipeline_check.cpp -o extensometer_pipeline_check $(pkg-config --cflags --libs opencv4)
//   ./extensometer_pipeline_check
//
// Run by .github/workflows/tests.yml (job "extensometer-opencv").
#include "ExtensometerCentroid.h"
#include <opencv2/opencv.hpp>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstdlib>

struct Result { bool ok; double oldDist, newDist; };

static Result Detect(const cv::Mat& frameIn, int margin, int hMin, int hMax, int sMin, int vMin) {
    cv::Mat frame = frameIn.clone();
    std::vector<cv::Point2f> oldC, newC;
    cv::Rect active_roi(margin, 0, frame.cols - 2 * margin, frame.rows);
    cv::Mat cropped_frame = frame(active_roi), cropped_hsv, cropped_mask;
    cv::GaussianBlur(cropped_frame, cropped_frame, cv::Size(5, 5), 0);
    cv::cvtColor(cropped_frame, cropped_hsv, cv::COLOR_BGR2HSV);
    cv::inRange(cropped_hsv, cv::Scalar(hMin, sMin, vMin), cv::Scalar(hMax, 255, 255), cropped_mask);
    cv::erode(cropped_mask, cropped_mask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
    cv::dilate(cropped_mask, cropped_mask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(8, 8)));
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(cropped_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    ExtensometerCentroid::Options o; o.hMin = hMin; o.hMax = hMax;
    const cv::Rect croppedArea(0, 0, cropped_hsv.cols, cropped_hsv.rows);
    for (const auto& c : contours) {
        if (cv::contourArea(c) <= 50) continue;
        cv::Moments m = cv::moments(c);
        oldC.push_back(cv::Point2f((float)(m.m10 / m.m00) + margin, (float)(m.m01 / m.m00)));
        const int pad = 3;
        const cv::Rect box = cv::boundingRect(c);
        const cv::Rect search = cv::Rect(box.x - pad, box.y - pad, box.width + 2 * pad, box.height + 2 * pad) & croppedArea;
        cv::Mat blobMask = cv::Mat::zeros(search.size(), CV_8UC1);
        cv::drawContours(blobMask, std::vector<std::vector<cv::Point>>{ c }, 0, cv::Scalar(255), cv::FILLED, cv::LINE_8, cv::noArray(), INT_MAX, cv::Point(-search.x, -search.y));
        auto centre = ExtensometerCentroid::WeightedCentre(cropped_hsv.data, (int)cropped_hsv.step, cropped_hsv.cols, cropped_hsv.rows,
            blobMask.data, (int)blobMask.step, ExtensometerCentroid::Rect{ search.x, search.y, search.width, search.height }, o);
        newC.push_back(centre.ok ? cv::Point2f((float)centre.x + margin, (float)centre.y) : oldC.back());
    }
    if (oldC.size() != 2) return { false, 0, 0 };
    return { true, cv::norm(oldC[0] - oldC[1]), cv::norm(newC[0] - newC[1]) };
}

// Two green dots on a grey background, anti-aliased, at sub-pixel positions (8 bits of sub-pixel in cv::circle)
static cv::Mat Frame(double x1, double y1, double x2, double y2, double radius, double noise, double shade) {
    // Drawn 4x larger, then shrunk (INTER_AREA averages: like a camera pixel)
    const int k = 4;
    cv::Mat big(600 * k, 800 * k, CV_8UC3, cv::Scalar(120, 120, 120));
    auto dot = [&](double x, double y) {
        cv::circle(big, cv::Point((int)std::lround(x * k * 256), (int)std::lround(y * k * 256)), (int)std::lround(radius * k * 256), cv::Scalar(60, 200, 40), cv::FILLED, cv::LINE_AA, 8);
        if (shade > 0) { // Right half darker
            cv::Mat half = big(cv::Rect((int)(x * k), (int)((y - radius - 1) * k), (int)((radius + 1) * k), (int)((2 * radius + 2) * k)));
            cv::Mat m; cv::inRange(half, cv::Scalar(0, 150, 0), cv::Scalar(255, 255, 255), m);
            cv::Mat darker; half.convertTo(darker, -1, 1.0 - shade); darker.copyTo(half, m);
        }
    };
    dot(x1, y1); dot(x2, y2);
    cv::Mat frame; cv::resize(big, frame, cv::Size(800, 600), 0, 0, cv::INTER_AREA);
    if (noise > 0) { cv::Mat n(frame.size(), CV_16SC3); cv::randn(n, 0, noise); cv::Mat f16; frame.convertTo(f16, CV_16SC3); f16 += n; f16.convertTo(frame, CV_8UC3); }
    return frame;
}

int main() {
    int failures = 0;
    cv::theRNG().state = 12345;
    for (double noise : { 0.0, 3.0 }) for (double shade : { 0.0, 0.2 }) {
        // The lower dot moves down 0.05 px per frame (a test), 100 frames = 5 px; L0 = 400 px
        double sumOld = 0, sumNew = 0, maxOld = 0, maxNew = 0; int n = 0;
        double old0 = 0, new0 = 0;
        for (int i = 0; i <= 100; ++i) {
            const double dy = 0.05 * i;
            const Result r = Detect(Frame(400.3, 100.4, 401.1, 500.2 + dy, 7.0, noise, shade), 50, 25, 85, 110, 50);
            if (!r.ok) { printf("FAIL: markers lost in frame %d\n", i); ++failures; continue; }
            if (i == 0) { old0 = r.oldDist; new0 = r.newDist; }
            // Expected change of the distance (the dots are almost vertical: ~ dy)
            const double trueDist0 = std::hypot(401.1 - 400.3, 500.2 - 100.4), trueDist = std::hypot(401.1 - 400.3, 500.2 + dy - 100.4);
            const double eOld = (r.oldDist - old0) - (trueDist - trueDist0), eNew = (r.newDist - new0) - (trueDist - trueDist0);
            sumOld += eOld * eOld; sumNew += eNew * eNew; maxOld = std::max(maxOld, std::fabs(eOld)); maxNew = std::max(maxNew, std::fabs(eNew)); ++n;
        }
        printf("noise %.0f, shade %.0f %%: distance change error  old rms %.3f px (max %.3f)   new rms %.3f px (max %.3f)   -> strain error at L0 = 400 px: old %.4f %%, new %.4f %%\n",
            noise, shade * 100, std::sqrt(sumOld / n), maxOld, std::sqrt(sumNew / n), maxNew, 100 * std::sqrt(sumOld / n) / 400, 100 * std::sqrt(sumNew / n) / 400);
        const double rmsOld = std::sqrt(sumOld / n), rmsNew = std::sqrt(sumNew / n);
        // Without noise far better than the old centre; with camera noise still at least 2 times
        const bool ok = noise == 0.0 ? (rmsNew < 0.02 && rmsNew < 0.2 * rmsOld) : (rmsNew < 0.06 && rmsNew < 0.6 * rmsOld);
        if (!ok) { printf("FAIL: the new centre is not better enough here\n"); ++failures; }
    }
    if (failures == 0)
        printf("extensometer_pipeline_check: all checks passed\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
