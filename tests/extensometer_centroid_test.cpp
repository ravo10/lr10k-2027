// Unit tests for ExtensometerCentroid (main/LloydLR10KTensile/ExtensometerCentroid.h).
//
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -I main/LloydLR10KTensile tests/extensometer_centroid_test.cpp -o extensometer_centroid_test
//   ./extensometer_centroid_test
//
// Run automatically by .github/workflows/tests.yml.
//
// The images are drawn discs with a known sub-pixel centre: each pixel's colour is a mix of the marker and the
// background by how much of the pixel the disc covers (16 x 16 samples per pixel), as a camera sees it.

#include "ExtensometerCentroid.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

using namespace ExtensometerCentroid;

constexpr int W = 120, H = 120;

struct Image
{
    std::vector<std::uint8_t> hsv = std::vector<std::uint8_t>(W * H * 3);

    //! Stores an RGB colour as OpenCV's 8-bit COLOR_BGR2HSV does (H 0..179, S and V 0..255).
    void SetRgb(int x, int y, double r, double g, double b)
    {
        r = std::clamp(std::round(r), 0.0, 255.0); g = std::clamp(std::round(g), 0.0, 255.0); b = std::clamp(std::round(b), 0.0, 255.0);
        const double v = std::max({ r, g, b }), mn = std::min({ r, g, b }), d = v - mn;
        const double sat = v > 0 ? 255.0 * d / v : 0.0;
        double h = 0.0;
        if (d > 0)
        {
            if (v == r) h = 60.0 * (g - b) / d;
            else if (v == g) h = 120.0 + 60.0 * (b - r) / d;
            else h = 240.0 + 60.0 * (r - g) / d;
            if (h < 0) h += 360.0;
        }
        std::uint8_t* p = &hsv[(y * W + x) * 3];
        p[0] = (std::uint8_t)std::lround(h / 2.0) % 180;
        p[1] = (std::uint8_t)std::lround(sat);
        p[2] = (std::uint8_t)std::lround(v);
    }
};

// A green dot (RGB 40, 200, 60) on a grey background (120): each pixel is a mix by how much of it the dot covers.
// `shade`: the right half of the dot this much darker (0.4 = 40 %, uneven light). `noise`: RGB noise.
static Image Disc(double cx, double cy, double radius, double shade = 0.0, double noise = 0.0, unsigned seed = 1)
{
    Image img;
    std::mt19937 rng(seed);
    std::normal_distribution<double> n(0.0, noise > 0 ? noise : 1e-12);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            int inside = 0;
            for (int sy = 0; sy < 16; ++sy)
                for (int sx = 0; sx < 16; ++sx)
                {
                    const double px = x + (sx + 0.5) / 16.0, py = y + (sy + 0.5) / 16.0;
                    inside += (px - cx) * (px - cx) + (py - cy) * (py - cy) <= radius * radius;
                }
            const double cover = inside / 256.0;
            const double light = x + 0.5 > cx ? 1.0 - shade : 1.0;
            img.SetRgb(x, y, 120 + cover * (40 * light - 120) + n(rng), 120 + cover * (200 * light - 120) + n(rng), 120 + cover * (60 * light - 120) + n(rng));
        }
    return img;
}

struct HsvRange { int hMin, hMax, sMin, sMax, vMin, vMax; };

// The old way: a black-and-white mask (inside the range or not), grown by `grow` pixels, centre of the mask
static void OldCentre(const Image& img, const HsvRange& r, int grow, double& ox, double& oy)
{
    std::vector<std::uint8_t> mask(W * H), grown(W * H);
    for (int i = 0; i < W * H; ++i)
    {
        const std::uint8_t* p = &img.hsv[i * 3];
        mask[i] = p[0] >= r.hMin && p[0] <= r.hMax && p[1] >= r.sMin && p[1] <= r.sMax && p[2] >= r.vMin && p[2] <= r.vMax;
    }
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            bool any = false;
            for (int dy = -grow; dy <= grow && !any; ++dy)
                for (int dx = -grow; dx <= grow && !any; ++dx)
                {
                    const int xx = x + dx, yy = y + dy;
                    any = xx >= 0 && yy >= 0 && xx < W && yy < H && dx * dx + dy * dy <= grow * grow && mask[yy * W + xx];
                }
            grown[y * W + x] = any;
        }
    double s = 0, sx = 0, sy = 0;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (grown[y * W + x]) { s += 1; sx += x + 0.5; sy += y + 0.5; }
    ox = sx / s;
    oy = sy / s;
}

static HsvRange Green() { return HsvRange{ 25, 85, 110, 255, 50, 255 }; }

// Pixel (x, y) covers x .. x+1, so its centre is x + 0.5
static Centre Find(const Image& img, const HsvRange& r, Rect rect = { 40, 40, 40, 40 })
{
    Options o;
    o.hMin = r.hMin;
    o.hMax = r.hMax;
    Centre c = WeightedCentre(img.hsv.data(), W * 3, W, H, nullptr, 0, rect, o);
    c.x += 0.5;
    c.y += 0.5;
    return c;
}

static void test_soft_weights()
{
    CHECK(Soft(100, 50, 150, 10) == 1.0);
    CHECK(Soft(45, 50, 150, 10) == 0.5);
    CHECK(Soft(39, 50, 150, 10) == 0.0);
    CHECK(Soft(49, 50, 150, 0) == 0.0);
    (void)Green;
    CHECK(SoftHue(60, 25, 85, 8) == 1.0);
    CHECK(SoftHue(89, 25, 85, 8) == 0.5);
    CHECK(SoftHue(178, 0, 10, 8) > 0.0);       // 178 is 2 away from 0
    CHECK(SoftHue(5, 170, 10, 8) == 1.0);      // A red range that wraps around
    CHECK(SoftHue(90, 170, 10, 8) == 0.0);
}

static void test_sub_pixel_positions()
{
    // Centres at many sub-pixel positions: the new centre is within 0.02 px, the old one more than 5 times further off
    double worstNew = 0.0, worstOld = 0.0;
    for (int i = 0; i < 10; ++i)
    {
        const double cx = 60.0 + 0.1 * i, cy = 58.0 + 0.07 * i;
        const Image img = Disc(cx, cy, 6.0);
        const Centre c = Find(img, Green());
        CHECK(c.ok);
        worstNew = std::max(worstNew, std::hypot(c.x - cx, c.y - cy));
        double ox, oy;
        OldCentre(img, Green(), 4, ox, oy);
        worstOld = std::max(worstOld, std::hypot(ox - cx, oy - cy));
    }
    std::printf("  sub-pixel: worst error new %.3f px, old %.3f px\n", worstNew, worstOld);
    CHECK(worstNew < 0.02);
    CHECK(worstOld > 5 * worstNew);
}

static void test_uneven_light()
{
    // One side of the dot darker (light from the left)
    const double cx = 60.3, cy = 60.6;
    for (double shade : { 0.15, 0.4 })
    {
        const Image img = Disc(cx, cy, 6.0, shade);
        const Centre c = Find(img, Green());
        double ox, oy;
        OldCentre(img, Green(), 4, ox, oy);
        const double errorNew = std::hypot(c.x - cx, c.y - cy), errorOld = std::hypot(ox - cx, oy - cy);
        std::printf("  %.0f %% darker on one side: error new %.3f px, old %.3f px\n", shade * 100, errorNew, errorOld);
        CHECK(c.ok);
        CHECK(errorNew < 0.1);
        if (shade < 0.2)
            CHECK(errorNew < errorOld); // Normal light differences: better than before
    }
}

static void test_noise()
{
    // Camera noise: the centre moves much less from frame to frame than the old one
    double sumSqNew = 0.0, sumSqOld = 0.0;
    const int frames = 30;
    for (int f = 0; f < frames; ++f)
    {
        const Image img = Disc(60.25, 60.25, 6.0, 0.0, 4.0, f + 1); // RGB noise, sigma 4
        const Centre c = Find(img, Green());
        sumSqNew += (c.x - 60.25) * (c.x - 60.25) + (c.y - 60.25) * (c.y - 60.25);
        double ox, oy;
        OldCentre(img, Green(), 4, ox, oy);
        sumSqOld += (ox - 60.25) * (ox - 60.25) + (oy - 60.25) * (oy - 60.25);
    }
    const double rmsNew = std::sqrt(sumSqNew / frames), rmsOld = std::sqrt(sumSqOld / frames);
    std::printf("  noise: rms error new %.3f px, old %.3f px\n", rmsNew, rmsOld);
    CHECK(rmsNew < 0.05);
    CHECK(rmsNew < 0.5 * rmsOld);
}

static void test_rect_and_mask()
{
    const Image img = Disc(30.4, 40.7, 5.0);
    // Only the rectangle around the dot is used
    Centre c = Find(img, Green(), Rect{ 20, 30, 20, 20 });
    CHECK(c.ok && std::fabs(c.x - 30.4) < 0.05 && std::fabs(c.y - 40.7) < 0.05);
    // A rectangle without the dot: not found
    CHECK(!Find(img, Green(), Rect{ 80, 80, 20, 20 }).ok);
    // Rectangles partly outside the image are clipped, empty ones give nothing
    CHECK(!Find(img, Green(), Rect{ 200, 200, 10, 10 }).ok);
    CHECK(Find(img, Green(), Rect{ -10, -10, 60, 70 }).ok);
    CHECK(!WeightedCentre(nullptr, 0, W, H, nullptr, 0, Rect{ 0, 0, W, H }).ok);

    // A mask (the size of the rectangle) that leaves out the bottom half: the centre moves up
    const Rect rect{ 20, 30, 20, 20 };
    std::vector<std::uint8_t> mask(rect.width * rect.height, 1);
    for (int y = 41 - rect.y; y < rect.height; ++y)
        for (int x = 0; x < rect.width; ++x)
            mask[y * rect.width + x] = 0;
    const Centre half = WeightedCentre(img.hsv.data(), W * 3, W, H, mask.data(), rect.width, rect);
    CHECK(half.ok && half.y + 0.5 < 40.7 - 1.0);
}

int main()
{
    test_soft_weights();
    test_sub_pixel_positions();
    test_uneven_light();
    test_noise();
    test_rect_and_mask();

    if (g_failures == 0)
        std::printf("extensometer_centroid_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
