#ifndef COORDINATES_H
#define COORDINATES_H

#include "config.hpp"
#include "mainWindow.hpp"

#ifdef USE_DOUBLE
#define float_type double
#else
#define float_type float
#endif

// Forward declarations

typedef struct WorldCoordinate WorldCoordinate;
typedef struct PixelCoordinate PixelCoordinate;
typedef struct ScreenCoordinate ScreenCoordinate;

/**
 * Coordinates in the world space, which is the complex plane.
 */
struct WorldCoordinate {
    float_type x, y;
    PixelCoordinate toPixel(ViewSettings view);
    void rotate(float_type sinTheta, float_type cosTheta);
};

WorldCoordinate complex_mul(WorldCoordinate f1, WorldCoordinate f2);
WorldCoordinate complex_square(WorldCoordinate f);
WorldCoordinate operator+(WorldCoordinate f1, WorldCoordinate f2);
WorldCoordinate operator*(float_type x, WorldCoordinate f);
WorldCoordinate operator*(WorldCoordinate f, float_type x);

/**
 * Coordinates in the pixel array that is drawn to the screen.
 */
struct PixelCoordinate {
    int x, y;
    WorldCoordinate toWorld(ViewSettings view);
    ScreenCoordinate toScreen(WindowSettings settings);
};

/**
 * Pixel coordinates on the screen. These will be identical to
 * the PixelCoordinate on the screen if the window hasn't
 * been resized and there is no zoom/translating happening.
*/
struct ScreenCoordinate {
    int x, y;
    PixelCoordinate toPixel(WindowSettings settings);
};

#endif