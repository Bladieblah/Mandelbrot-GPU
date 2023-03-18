#include "coordinates.hpp"
#include "mainWindow.hpp"

void WorldCoordinate::rotate(float sinTheta, float cosTheta) {
    float tmp = cosTheta * x - sinTheta * y;

    y = sinTheta * x + cosTheta * y;
    x = tmp;
}

PixelCoordinate WorldCoordinate::toPixel(ViewSettings view) {
    WorldCoordinate tmp({
        (x - view.centerX),
        (y - view.centerY)
    });

    tmp.rotate(view.sinTheta, view.cosTheta);

    return (PixelCoordinate) {
        (int)((1 + tmp.x / view.scaleX) / 2 * view.sizeX),
        (int)((1 + tmp.y / view.scaleY) / 2 * view.sizeY)
    };
}

WorldCoordinate PixelCoordinate::toWorld(ViewSettings view) {
    WorldCoordinate tmp({
        (2 * (x / (float)view.sizeX) - 1) * view.scaleX,
        (2 * (y / (float)view.sizeY) - 1) * view.scaleY
    });

    tmp.rotate(-view.sinTheta, view.cosTheta);

    return (WorldCoordinate) {
        tmp.x + view.centerX,
        tmp.y + view.centerY
    };
}

WorldCoordinate complex_mul(WorldCoordinate f1, WorldCoordinate f2) {
    return {f1.x * f2.x - f1.y * f2.y, f1.x * f2.y + f1.y * f2.x};
}

WorldCoordinate complex_square(WorldCoordinate f) {
    return {f.x * f.x - f.y * f.y, 2 * f.x * f.y};
}

WorldCoordinate operator+(WorldCoordinate f1, WorldCoordinate f2) {
    return {f1.x + f2.x, f1.y + f2.y};
}

WorldCoordinate operator*(float x, WorldCoordinate f) {
    return {x * f.x, x * f.y};
}

WorldCoordinate operator*(WorldCoordinate f, float x) {
    return {x * f.x, x * f.y};
}

ScreenCoordinate PixelCoordinate::toScreen(WindowSettings settings) {
    return (ScreenCoordinate) {
        (int)((x / (float)settings.width  - settings.centerX) * settings.zoom * settings.windowW),
        (int)((y / (float)settings.height - settings.centerY) * settings.zoom * settings.windowH)
    };
}

PixelCoordinate ScreenCoordinate::toPixel(WindowSettings settings) {
    return (PixelCoordinate) {
        (int)((x / (float)(settings.windowW * settings.zoom) + settings.centerX) * settings.width),
        (int)(((settings.windowH - y) / (float)(settings.windowH * settings.zoom) + settings.centerY) * settings.height)
    };
}
