#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include "config.hpp"
#include "opencl.hpp"

#ifdef MANDEL_GPU_USE_DOUBLE
typedef struct IntPair {
    unsigned int sign;
    unsigned long integ;
    unsigned long fract;
} IntPair;

typedef struct {
    IntPair x, y;
} ComplexDouble;

typedef struct Particle {
    ComplexDouble pos, offset;
    unsigned int iterCount;
    int escaped;
} Particle;

typedef struct ViewSettings {
    double scaleX, scaleY;
    double centerX, centerY;
    double theta, sinTheta, cosTheta;
    unsigned long sizeX, sizeY;
    unsigned long particlesX, particlesY;
} ViewSettings;

typedef struct ViewSettingsCL {
    IntPair scaleX, scaleY;
    IntPair centerX, centerY;
    IntPair theta, sinTheta, cosTheta;
    unsigned long sizeX, sizeY;
    unsigned long particlesX, particlesY;
} ViewSettingsCL;
extern ViewSettingsCL viewMainCL;
#else
typedef struct Particle {
    cl_float2 pos, offset;
    unsigned int iterCount;
    int escaped;
} Particle;

typedef struct ViewSettings {
    float scaleX, scaleY;
    float centerX, centerY;
    float theta, sinTheta, cosTheta;
    int sizeX, sizeY;
    int particlesX, particlesY;
} ViewSettings;
#endif

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

void displayMain();

void createMainWindow(char *name, uint32_t width, uint32_t height);
void destroyMainWindow();
void transformView();

extern ViewSettings viewMain, defaultView;
extern WindowSettings settingsMain;
extern uint32_t *pixelsMain;
extern OpenCl *opencl;
extern Config *config;

#endif
