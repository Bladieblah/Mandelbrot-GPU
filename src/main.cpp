#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <math.h>

#include <GLUT/glut.h>

#include "colourMap.hpp"
#include "config.hpp"
#include "mainWindow.hpp"
#include "opencl.hpp"
#include "pcg.hpp"

using namespace std;

/**
 * Declarations
 */

Config *config;

chrono::high_resolution_clock::time_point frameTime;
unsigned int frameCount = 0;
/**
 * OpenCL
 */

OpenCl *opencl;
uint64_t *initState, *initSeq;

unsigned int superSample = 2;

vector<BufferSpec> bufferSpecs;
void createBufferSpecs() {
    bufferSpecs = {
        {"particles", {NULL, superSample * superSample * config->width * config->height * sizeof(Particle)}},
        {"image",     {NULL, 3 * config->width * config->height * sizeof(uint32_t)}},
        {"colourMap", {NULL, 3 * config->num_colours * sizeof(unsigned int)}},
    };
}

vector<KernelSpec> kernelSpecs;
void createKernelSpecs() {
    kernelSpecs = {
        {"initParticles", {NULL, 2, {superSample * config->width, superSample * config->height}, {0, 0}, "initParticles"}},
        {"mandelStep",    {NULL, 2, {superSample * config->width, superSample * config->height}, {0, 0}, "mandelStep"}},
        {"renderImage",   {NULL, 2, {config->width, config->height}, {0, 0}, "renderImage"}},
    };
}

#ifdef USE_DOUBLE
double to_double(IntPair num) {
    return (num.sign ? 1 : -1) * ((double)num.integ + (((double)num.fract) / (~0ULL)));
}
#endif

void setKernelArgs() {
    opencl->setKernelBufferArg("initParticles", 0, "particles");
#ifdef USE_DOUBLE
    transformView();
    opencl->setKernelArg("initParticles", 1, sizeof(ViewSettingsCL), &(viewMainCL));
#else
    opencl->setKernelArg("initParticles", 1, sizeof(ViewSettings), &(viewMain));
#endif

    opencl->setKernelBufferArg("mandelStep", 0, "particles");
    opencl->setKernelArg("mandelStep", 1, sizeof(uint32_t), &(config->steps));

    unsigned int zero = 0;
    opencl->setKernelBufferArg("renderImage", 0, "particles");
    opencl->setKernelBufferArg("renderImage", 1, "image");
    opencl->setKernelBufferArg("renderImage", 2, "colourMap");
    opencl->setKernelArg("renderImage", 3, sizeof(unsigned int), &(config->num_colours));
    opencl->setKernelArg("renderImage", 4, sizeof(unsigned int), &zero);
    opencl->setKernelArg("renderImage", 5, sizeof(unsigned int), &superSample);
}

void prepareOpenCl() {
    createBufferSpecs();
    createKernelSpecs();

    opencl = new OpenCl(
#ifdef USE_DOUBLE
        "shaders/sample_double.cl",
#else
        "shaders/sample.cl",
#endif
        bufferSpecs,
        kernelSpecs,
        config->profile,
        config->useGpu,
        config->verbose
    );

    vector<ColourInt> colours = {
        {0.0, {0, 10, 2}},
        {0.05, {3, 34, 12}},
        {0.2, {8, 76, 33}},
        {0.4, {13, 117, 53}},
        {0.7, {10, 172, 102}},
        {0.9, {63, 216, 112}},
        {1.0, {25, 236, 173}},
    };

    ColourMap cm(colours, config->num_colours, true);
    unsigned int *cmap = (unsigned int *)malloc(3 * config->num_colours * sizeof(unsigned int));
    cm.apply(cmap);
    opencl->writeBuffer("colourMap", cmap);

    setKernelArgs();

    opencl->step("initParticles");
}

void prepare() {

    #ifdef USE_DOUBLE
    float scaleY = config->scale;
    viewMain = {
        scaleY / (double)config->height * (double)config->width, scaleY,
        config->center_x, config->center_y,
        config->theta, sin(config->theta), cos(config->theta),
        (unsigned long)config->width, (unsigned long)config->height,
        (unsigned long)(config->width * superSample), (unsigned long)(config->height * superSample),
    };
    #else
    float scaleY = config->scale;
    viewMain = {
        scaleY / (float)config->height * (float)config->width, scaleY,
        config->center_x, config->center_y,
        config->theta, sin(config->theta), cos(config->theta),
        (int)config->width, (int)config->height,
        (int)(config->width * superSample), (int)(config->height * superSample),
    };
    #endif

    defaultView = viewMain;

    prepareOpenCl();
}

void display() {
    frameCount++;

    if (frameCount % 2 == 0) {
        return;
    }

    opencl->startFrame();
    
    displayMain();

    opencl->step("mandelStep");
    opencl->step("renderImage");
    opencl->readBuffer("image", pixelsMain);

    chrono::high_resolution_clock::time_point temp = chrono::high_resolution_clock::now();
    chrono::duration<float> time_span = chrono::duration_cast<chrono::duration<float>>(temp - frameTime);
    fprintf(stderr, "Step = %d, time = %.4g            \n", frameCount / 2, time_span.count());
    fprintf(stderr, "\x1b[%dA", opencl->printCount + 1);
    frameTime = temp; 
}

void cleanAll() {
    for (int i = 0; i <= opencl->printCount; i++) {
        fprintf(stderr, "\n");
    }

    fprintf(stderr, "Exiting\n");
    destroyMainWindow();
    opencl->cleanup();
}

int main(int argc, char **argv) {
    config = new Config("config.cfg");
    config->printValues();

    frameTime = chrono::high_resolution_clock::now();

    prepare();

    atexit(&cleanAll);

    glutInit(&argc, argv);
    createMainWindow("Main", config->width, config->height);
    glutDisplayFunc(&display);

    glutIdleFunc(&display);

    fprintf(stderr, "\nStarting main loop\n\n");
    glutMainLoop();

    return 0;
}
