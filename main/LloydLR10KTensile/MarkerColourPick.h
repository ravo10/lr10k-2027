/**

    @file      MarkerColourPick.h
    @brief     The marker colour from a click on the camera image (eyedropper): the HSV ranges for the mask.
    @details   Portable (no OpenCV or GUI code) and unit tested (tests/marker_colour_pick_test.cpp).

               The user clicks a marker in the extensometer's camera view. The camera thread takes the pixels around
               that point in the full-resolution frame (not the displayed image, which has the overlay drawn on it),
               in OpenCV's HSV (H 0..179, S and V 0..255), and this file turns them into the ranges for cv::inRange():

               - the median of each channel, so a few pixels of background or of the marker's edge do not count;
               - hue: the median +/- kHueHalfWidth (the marker's own colour, with room for light changes), wrapping
                 around for reds;
               - saturation and value: from somewhat below the sample up to 255, because the marker's edges and
                 its shaded side are less saturated and darker than its centre, never brighter.

               A sample that is hardly coloured (white, grey, black: the specimen, a grip, a shadow) is refused:
               the mask would then find the background instead of the marker.

               Hue is a circle (OpenCV: 0..179, 2 degrees per step): red is at both ends. A hue range with
               hMin > hMax wraps around, e.g. 170..10 for a red marker (170..179 and 0..10). The helpers below
               (HueWidth, HueCentre, HueRangeAround, InHueRange) handle that; the mask is then made of two parts
               (MaskHueParts), and ExtensometerCentroid.h's SoftHue() already accepts a wrapping range.

**/
#pragma once

#include <algorithm>
#include <vector>

namespace MarkerColourPick
{
    inline constexpr int kHueHalfWidth = 15;   //!< Hue range +/- this (OpenCV hue: 2 degrees per step)
    inline constexpr int kSatBelow = 80;       //!< Saturation range starts this much below the sample
    inline constexpr int kValBelow = 90;       //!< Value (brightness) range starts this much below the sample
    inline constexpr int kMinSat = 40;         //!< Lowest saturation range start (below: greys)
    inline constexpr int kMinVal = 30;         //!< Lowest value range start (below: black)
    inline constexpr int kMarkerMinSat = 70;   //!< A sample less saturated than this is not a marker colour
    inline constexpr int kMarkerMinVal = 50;   //!< A sample darker than this is not a marker colour

    struct HsvRange
    {
        int hMin = 0, hMax = 179, sMin = 0, sMax = 255, vMin = 0, vMax = 255;
    };

    struct Result
    {
        bool ok = false;      //!< False: no pixels, or not a marker colour (see the file description)
        int h = 0, s = 0, v = 0; //!< The median colour of the sample
        HsvRange range;       //!< The ranges to use (only when ok)
    };

    // ---------------------------------------------------------------------------------------------------------
    // Hue ranges that can wrap around (hMin > hMax: through 179 and 0)
    // ---------------------------------------------------------------------------------------------------------

    inline constexpr int kHueSteps = 180; //!< OpenCV hue: 0..179

    inline bool HueWraps(int hMin, int hMax) { return hMin > hMax; }

    //! Number of steps from hMin to hMax (going up, through 179 -> 0 when the range wraps).
    inline int HueWidth(int hMin, int hMax) { return HueWraps(hMin, hMax) ? hMax + kHueSteps - hMin : hMax - hMin; }

    //! The hue in the middle of the range (0 .. <180).
    inline double HueCentre(int hMin, int hMax)
    {
        const double c = hMin + HueWidth(hMin, hMax) / 2.0;
        return c >= kHueSteps ? c - kHueSteps : c;
    }

    //! The range centre +/- halfWidth, wrapping when it passes 0 or 179. A range of the whole circle is 0..179.
    inline HsvRange HueRangeAround(int centre, int halfWidth, HsvRange range = HsvRange{})
    {
        if (2 * halfWidth >= kHueSteps - 1)
        {
            range.hMin = 0; range.hMax = kHueSteps - 1;
            return range;
        }
        auto wrap = [](int h) { return ((h % kHueSteps) + kHueSteps) % kHueSteps; };
        range.hMin = wrap(centre - halfWidth);
        range.hMax = wrap(centre + halfWidth);
        return range;
    }

    inline bool InHueRange(int hue, int hMin, int hMax)
    {
        return HueWraps(hMin, hMax) ? (hue >= hMin || hue <= hMax) : (hue >= hMin && hue <= hMax);
    }

    //! The one or two plain (not wrapping) hue intervals for cv::inRange(): count is 1 or 2.
    struct HueParts { int count = 1; int lo[2] = { 0, 0 }; int hi[2] = { 179, 179 }; };
    inline HueParts MaskHueParts(int hMin, int hMax)
    {
        HueParts parts;
        if (!HueWraps(hMin, hMax)) { parts.lo[0] = hMin; parts.hi[0] = hMax; return parts; }
        parts.count = 2;
        parts.lo[0] = hMin; parts.hi[0] = kHueSteps - 1;
        parts.lo[1] = 0;    parts.hi[1] = hMax;
        return parts;
    }

    // ---------------------------------------------------------------------------------------------------------

    inline int Median(std::vector<int> values)
    {
        if (values.empty()) return 0;
        const std::size_t mid = values.size() / 2;
        std::nth_element(values.begin(), values.begin() + (std::ptrdiff_t)mid, values.end());
        return values[mid];
    }

    //! The median of hues, on the circle: hues on both sides of 0/179 (a red marker) are not averaged to cyan.
    inline int HueMedian(const std::vector<int>& hues)
    {
        if (hues.empty()) return 0;
        // Turn the circle by half a turn if that puts the values closer together (no longer across 0/179)
        auto spread = [&](int shift) {
            int lo = kHueSteps, hi = -1;
            for (int h : hues) { const int v = (h + shift) % kHueSteps; lo = std::min(lo, v); hi = std::max(hi, v); }
            return hi - lo;
        };
        const int shift = spread(kHueSteps / 2) < spread(0) ? kHueSteps / 2 : 0;
        std::vector<int> shifted;
        shifted.reserve(hues.size());
        for (int h : hues) shifted.push_back((h + shift) % kHueSteps);
        return (Median(shifted) - shift + kHueSteps) % kHueSteps;
    }

    /**
        @brief  The ranges from the sampled pixels (OpenCV HSV, interleaved: h, s, v, h, s, v, ...).
        @param  hsv    - The pixels.
        @param  count  - Number of pixels.
    **/
    inline Result FromPixels(const unsigned char* hsv, int count)
    {
        Result r;
        if (hsv == nullptr || count <= 0) return r;
        std::vector<int> h, s, v;
        h.reserve(count); s.reserve(count); v.reserve(count);
        for (int i = 0; i < count; ++i)
        {
            h.push_back(hsv[3 * i]);
            s.push_back(hsv[3 * i + 1]);
            v.push_back(hsv[3 * i + 2]);
        }
        r.h = HueMedian(h);
        r.s = Median(s);
        r.v = Median(v);
        if (r.s < kMarkerMinSat || r.v < kMarkerMinVal)
            return r; // Grey, white or black: not a marker
        r.range = HueRangeAround(r.h, kHueHalfWidth); // Wraps for reds (e.g. 168..12)
        r.range.sMin = std::max(kMinSat, r.s - kSatBelow);
        r.range.sMax = 255;
        r.range.vMin = std::max(kMinVal, r.v - kValBelow);
        r.range.vMax = 255;
        r.ok = true;
        return r;
    }

    //! Half the side of the square that is sampled around the click, for a frame this wide (5x5 at 800, 17x17 at 4K).
    inline int SampleRadius(int frameWidth) { return std::max(2, frameWidth / 480); }
}
