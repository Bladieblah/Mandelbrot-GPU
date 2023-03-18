#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include "config.hpp"
#include "opencl.hpp"

typedef struct Particle {
    cl_float2 pos, offset;
    unsigned int iterCount;
    bool escaped;
} Particle;

typedef struct WindowSettings {
    uint32_t width, height;
    uint32_t windowW, windowH;
    float zoom = 1, centerX = 0, centerY = 0;
    bool grid = false;
} WindowSettings;

typedef struct MouseState {
    int xDown, yDown;
    int x, y;
    int state;
} MouseState;

typedef struct ViewSettings {
    float scaleX, scaleY;
    float centerX, centerY;
    float theta, sinTheta, cosTheta;
    int sizeX, sizeY;
} ViewSettings;

void displayMain();

void createMainWindow(char *name, uint32_t width, uint32_t height);
void destroyMainWindow();

extern ViewSettings viewMain;
extern WindowSettings settingsMain;
extern uint32_t *pixelsMain;
extern OpenCl *opencl;
extern Config *config;

#endif
