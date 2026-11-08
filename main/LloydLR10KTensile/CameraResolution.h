/**

    @file      CameraResolution.h
    @brief     The extensometer camera's resolution, and how a large frame is processed without slowing down.
    @details   Portable (no OpenCV) and unit tested (tests/camera_resolution_test.cpp).

               A higher resolution gives finer strain (more pixels between the markers), but every step on the
               whole frame (blur, colour conversion, mask, display, recording) costs ~10x more at 4K than at
               800 x 600. So:
               - the markers are searched in a copy at most kMaxWorkWidth wide (as much work as before);
               - their sub-pixel centres are measured in the full-resolution frame, in a small rectangle around
                 each marker only (where the extra pixels help);
               - the camera view and the overlay video use the working copy, the raw video at most
                 kMaxRawRecordingWidth wide (writing 4K video would slow the loop down).
               The camera picks the size nearest to the one asked for; the window shows what it really gives.

**/
#pragma once

#include <algorithm>
#include <cmath>

namespace CameraResolution
{
    struct Option
    {
        int width;
        int height;
        const char* name;
    };

    //! Offered in the settings. Index 0 is the default (the size used before there was a choice).
    inline constexpr Option kOptions[] = {
        { 800, 600, "800 x 600" },
        { 1280, 720, "1280 x 720 (HD)" },
        { 1920, 1080, "1920 x 1080 (Full HD)" },
        { 2560, 1440, "2560 x 1440 (QHD)" },
        { 3840, 2160, "3840 x 2160 (4K)" },
    };
    inline constexpr int kOptionCount = (int)(sizeof(kOptions) / sizeof(kOptions[0]));

    constexpr int kMaxWorkWidth = 1280;          //!< The markers are searched at most this wide
    constexpr int kMaxRawRecordingWidth = 1920;  //!< The raw video is written at most this wide
    constexpr int kCompressedAboveWidth = 1280;  //!< Ask for MJPG above this (uncompressed 4K is a few frames/s over USB)

    inline int ClampIndex(int index) { return std::clamp(index, 0, kOptionCount - 1); }

    //! Full-resolution pixels per working pixel (1 if the frame is not wider than kMaxWorkWidth).
    inline double WorkScale(int frameWidth) { return frameWidth > kMaxWorkWidth ? (double)frameWidth / kMaxWorkWidth : 1.0; }

    //! The size the raw video is written at (the frame's, made smaller to at most kMaxRawRecordingWidth wide).
    inline void RawRecordingSize(int frameWidth, int frameHeight, int& width, int& height)
    {
        if (frameWidth <= kMaxRawRecordingWidth)
        {
            width = frameWidth;
            height = frameHeight;
            return;
        }
        width = kMaxRawRecordingWidth;
        height = (int)std::lround((double)frameHeight * kMaxRawRecordingWidth / frameWidth) & ~1; // Even: video codecs want it
    }

    //! How far (px, full resolution) a marker may move between good frames to be followed past extra blobs.
    //! 25 px was chosen for 800 px wide frames; the same movement is more pixels at a higher resolution.
    inline double MaxMarkerMovePx(int frameWidth) { return 25.0 * std::max(1.0, frameWidth / 800.0); }

    struct Rect
    {
        int x = 0, y = 0, width = 0, height = 0;
    };

    /**
        @brief  A rectangle of the working copy in full-resolution pixels, with `pad` pixels around it, clipped to
                the frame. Working pixel i covers full-resolution pixels i * scale .. (i + 1) * scale.
    **/
    inline Rect ToFullResolution(Rect work, double scale, int pad, int frameWidth, int frameHeight)
    {
        const int x0 = std::max(0, (int)std::floor(work.x * scale) - pad);
        const int y0 = std::max(0, (int)std::floor(work.y * scale) - pad);
        const int x1 = std::min(frameWidth, (int)std::ceil((work.x + work.width) * scale) + pad);
        const int y1 = std::min(frameHeight, (int)std::ceil((work.y + work.height) * scale) + pad);
        return Rect{ x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0) };
    }

    //! A position in working pixels (pixel centres at whole numbers) in full-resolution pixels, and back.
    inline double WorkToFull(double workPosition, double scale) { return (workPosition + 0.5) * scale - 0.5; }
    inline double FullToWork(double fullPosition, double scale) { return (fullPosition + 0.5) / scale - 0.5; }
}
