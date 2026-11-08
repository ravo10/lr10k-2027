/**

    @file      MarkerColourPick.h
    @brief     The marker colour from a click on the camera image (eyedropper): the HSV ranges for the mask.
    @details   Portable (no OpenCV or GUI code) and unit tested (tests/marker_colour_pick_test.cpp).

               The user clicks a marker in the extensometer's camera view. The camera thread takes the pixels around
               that point in the full-resolution frame (not the displayed image, which has the overlay drawn on it),
               in OpenCV's HSV (H 0..179, S and V 0..255), and this file turns them into the ranges for cv::inRange():

               - the median of each channel, so a few pixels of background or of the marker's edge do not count;
               - hue: the median +/- kHueHalfWidth (the marker's own colour, with room for light changes);
               - saturation and value: from somewhat below the sample up to 255, because the marker's edges and
                 its shaded side are less saturated and darker than its centre, never brighter.

               A sample that is hardly coloured (white, grey, black: the specimen, a grip, a shadow) is refused:
               the mask would then find the background instead of the marker.

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

    inline int Median(std::vector<int> values)
    {
        if (values.empty()) return 0;
        const std::size_t mid = values.size() / 2;
        std::nth_element(values.begin(), values.begin() + (std::ptrdiff_t)mid, values.end());
        return values[mid];
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
        r.h = Median(h);
        r.s = Median(s);
        r.v = Median(v);
        if (r.s < kMarkerMinSat || r.v < kMarkerMinVal)
            return r; // Grey, white or black: not a marker
        r.range.hMin = std::max(0, r.h - kHueHalfWidth);
        r.range.hMax = std::min(179, r.h + kHueHalfWidth);
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
