/**

    @file      ExtensometerCentroid.h
    @brief     Sub-pixel centre of a marker, weighted by how well each pixel matches the marker colour.
    @details   Portable (no OpenCV: plain HSV bytes) and unit tested (tests/extensometer_centroid_test.cpp).

               The centre used to be the centre of the blob's outline in a black-and-white mask, after the mask
               was grown by 8x8 pixels. That rounds it to about whole pixels, and the growing moves it when one
               side of the dot is lit differently. With a 50 mm gauge length over ~400 px, 1 px is ~0.25 %
               strain: as much as the whole default E-modulus window (0.05 % .. 0.25 %).

               Now every pixel near the blob gets a weight from 0 to 1, from its chroma (S * V / 255, which is
               max - min of R, G, B: against a grey background it grows in proportion to how much of the pixel
               the marker covers), for pixels of the marker's hue:
               - 0 at the background's chroma (the median of the search rectangle's border, plus 3 times its
                 noise);
               - 1 at the chroma of the brightest marker pixel within 2 pixels, so a dot that is darker on one
                 side (uneven light) is compared with its own dark side and not pulled to its bright side;
               - in between linear, so an edge pixel counts by how much of it is marker: that gives the sub-pixel
                 position. The centre is the weighted mean.

**/
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace ExtensometerCentroid
{
    //! 1 inside [lo, hi], falling linearly to 0 at `band` outside.
    inline double Soft(double value, double lo, double hi, double band)
    {
        if (value >= lo && value <= hi)
            return 1.0;
        const double outside = value < lo ? lo - value : value - hi;
        return band > 0.0 ? std::max(0.0, 1.0 - outside / band) : 0.0;
    }

    //! The same for hue (OpenCV: 0..179), which wraps around. A range with hMin > hMax wraps too (reds).
    inline double SoftHue(double hue, int hMin, int hMax, double band)
    {
        const bool inside = hMin <= hMax ? (hue >= hMin && hue <= hMax) : (hue >= hMin || hue <= hMax);
        if (inside)
            return 1.0;
        auto circular = [](double a, double b) { const double d = std::fabs(a - b); return std::min(d, 180.0 - d); };
        const double outside = std::min(circular(hue, hMin), circular(hue, hMax));
        return band > 0.0 ? std::max(0.0, 1.0 - outside / band) : 0.0;
    }

    struct Rect
    {
        int x = 0, y = 0, width = 0, height = 0;
    };

    struct Options
    {
        int hMin = 25, hMax = 85;  //!< The marker's hue range (from the settings)
        double hueBand = 8.0;      //!< Hue this far outside the range still counts a little
        int localRadius = 2;       //!< A pixel is compared with the brightest marker pixel within this many pixels
        double minLocalContrast = 0.25; //!< ... which must reach this fraction of the marker's chroma (else: noise)
        double minWeight = 5.0;    //!< Less total weight: not a marker
        double noiseFloor = 3.0;   //!< Pixels below the background + this many noise deviations do not count
    };

    struct Centre
    {
        bool ok = false;
        double x = 0.0;
        double y = 0.0;
        double weight = 0.0; //!< Sum of the weights (about the marker's area in pixels)
    };

    /**
        @brief  Weighted centre of the marker in `rect`.
        @param  hsv  - HSV image (OpenCV CV_8UC3: H 0..179, S, V), `hsvStep` bytes per row.
        @param  mask - Optional (nullptr: all pixels): only pixels where mask != 0 are weighted (so another blob in
                       the rectangle is left out). rect.width x rect.height, `maskStep` bytes per row: mask[0] is
                       the pixel (rect.x, rect.y).
        @param  rect - The blob's rectangle with a margin of a few pixels: its border is taken as background.
    **/
    inline Centre WeightedCentre(const std::uint8_t* hsv, int hsvStep, int imageWidth, int imageHeight,
        const std::uint8_t* mask, int maskStep, Rect rect, const Options& options = {})
    {
        Centre c;
        const int x0 = std::max(0, rect.x), y0 = std::max(0, rect.y);
        const int x1 = std::min(imageWidth, rect.x + rect.width), y1 = std::min(imageHeight, rect.y + rect.height);
        if (hsv == nullptr || x1 - x0 < 3 || y1 - y0 < 3)
            return c;

        auto markerness = [&](int x, int y) {
            const std::uint8_t* p = hsv + (std::size_t)y * hsvStep + 3 * x;
            return SoftHue(p[0], options.hMin, options.hMax, options.hueBand) * p[1] * p[2] / 255.0;
        };

        // Background: the median of the border (robust to a bit of another blob in it), and its noise (the median
        // absolute deviation). Below background + 3 noise a pixel does not count: noise in the many background
        // pixels would otherwise pull the centre to the middle of the rectangle.
        std::vector<double> border;
        for (int x = x0; x < x1; ++x) { border.push_back(markerness(x, y0)); border.push_back(markerness(x, y1 - 1)); }
        for (int y = y0 + 1; y < y1 - 1; ++y) { border.push_back(markerness(x0, y)); border.push_back(markerness(x1 - 1, y)); }
        auto median = [](std::vector<double> values) {
            std::nth_element(values.begin(), values.begin() + values.size() / 2, values.end());
            return values[values.size() / 2];
        };
        const double borderMedian = median(border);
        for (double& value : border)
            value = std::fabs(value - borderMedian);
        const double background = borderMedian + options.noiseFloor * 1.4826 * median(border);

        // The marker's chroma: the highest in the rectangle
        const int w = x1 - x0, h = y1 - y0;
        std::vector<double> m((std::size_t)w * h, 0.0);
        double peak = background;
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (!mask || mask[(std::size_t)(y - rect.y) * maskStep + (x - rect.x)] != 0)
                {
                    const double value = markerness(x, y);
                    m[(std::size_t)(y - y0) * w + (x - x0)] = value;
                    peak = std::max(peak, value);
                }
        if (!(peak - background > 0.0))
            return c;

        // Each pixel against the brightest marker pixel near it (localRadius): an edge pixel on the dark side of a
        // dot is compared with the dark side, so uneven light does not pull the centre to the bright side, and the
        // weight stays proportional to how much of the pixel is marker (exact sub-pixel positions)
        const int r = options.localRadius;
        const double minContrast = options.minLocalContrast * (peak - background);
        double sum = 0.0, sx = 0.0, sy = 0.0;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const double value = m[(std::size_t)y * w + x];
                if (value <= background)
                    continue;
                double local = value;
                for (int dy = std::max(0, y - r); dy <= std::min(h - 1, y + r); ++dy)
                    for (int dx = std::max(0, x - r); dx <= std::min(w - 1, x + r); ++dx)
                        local = std::max(local, m[(std::size_t)dy * w + dx]);
                if (local - background < minContrast)
                    continue; // Only noise around here
                const double weight = std::clamp((value - background) / (local - background), 0.0, 1.0);
                sum += weight;
                sx += weight * (x + x0);
                sy += weight * (y + y0);
            }
        if (sum < options.minWeight)
            return c;
        c.ok = true;
        c.x = sx / sum;
        c.y = sy / sum;
        c.weight = sum;
        return c;
    }
}
