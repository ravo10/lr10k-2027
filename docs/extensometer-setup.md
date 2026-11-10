# Suggested extensometer setup

The optical extensometer works with any USB webcam Windows recognises. This page describes the setup used at USN: the camera, the 3D-printed holder that fixes it to the LR 10K, and how to mount it. A camera that is fixed to the machine stays square to the specimen and does not move during the test, which matters more for the strain than the resolution.

The 3D models are attached to the **[extensiometer-holder-lr10k release](https://github.com/ravo10/lr10k-2027/releases/tag/extensiometer-holder-lr10k)**.

How to make and place the markers: [Extensometer markers](extensometer-markers.md).

## The camera

| | |
|---|---|
| Camera | Logitech BRIO 4K (Business) webcam, USB |
| Resolution | Up to 3840 × 2160; choose it in the extensometer window under *Camera Properties* |

The software turns the camera's autofocus, automatic exposure and automatic white balance off when it opens the camera, so the picture does not change during a test. If the console says that the white balance could not be turned off, turn it off in Logitech's own camera software.

## The holder

The holder is 3D printed and fixed to the LR 10K with **M3 screws, 20 mm long**, in the two holes in the centre of the LR 10K, about **42 mm apart**. Measure the distance on your machine before you print: if it is not 42 mm, the holes in the holder will not line up.

## Mounting it

1. Print the holder (files in the [release](https://github.com/ravo10/lr10k-2027/releases/tag/extensiometer-holder-lr10k)).
2. Fix it to the LR 10K with the two M3 × 20 mm screws.
3. Put the camera in the holder and connect it to the PC.
4. Clamp a specimen with markers, open *View* > *Extensometer* and choose the camera under *Hardware*.
5. Check in the camera picture that the specimen is upright and in the middle, and that both markers are well inside the picture for the whole stretch. Even light, no reflections on the markers.
6. Do not touch the camera or the holder during the test: if the camera moves, the markers seem to move.
