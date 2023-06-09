#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <math.h>

#include <GLUT/glut.h>

#include "colourMap.hpp"
#include "config.hpp"
// #include "gradientWindow.hpp"
#include "mainWindow.hpp"
#include "opencl.hpp"
#include "pcg.hpp"

using namespace std;

/**
 * Declarations
 */

Config *config;
unsigned int *cmap;
ColourMap *cm;

chrono::high_resolution_clock::time_point timePoint;
unsigned int frameCount = 0;
float frameTime = 0;
uint32_t iterCount = 0;
/**
 * OpenCL
 */

OpenCl *opencl;
uint64_t *initState, *initSeq;

unsigned int superSample;

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

#ifdef MANDEL_GPU_USE_DOUBLE
double to_double(IntPair num) {
    return (num.sign ? 1 : -1) * ((double)num.integ + (((double)num.fract) / (~0ULL)));
}
#endif

void setKernelArgs() {
    opencl->setKernelBufferArg("initParticles", 0, "particles");
#ifdef MANDEL_GPU_USE_DOUBLE
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
    fprintf(stderr, "createBufferSpecs\n");
    createBufferSpecs();
    fprintf(stderr, "createKernelSpecs\n");
    createKernelSpecs();

    fprintf(stderr, "OpenCl\n");
    opencl = new OpenCl(
#ifdef MANDEL_GPU_USE_DOUBLE
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

    fprintf(stderr, "filename\n");
    char filename[120] = "colourmaps/default.cm";
    if (strlen(config->colour_file)) {
        fprintf(stderr, "Writing fn\n");
        sprintf(filename, "colourmaps/%s", config->colour_file);
        fprintf(stderr, "Writing fn done\n");
    }
    fprintf(stderr, "ColourMapFromFile\n");
    cm = ColourMapFromFile(filename, config->num_colours);
    fprintf(stderr, "cmap\n");
    cmap = (unsigned int *)malloc(3 * config->num_colours * sizeof(unsigned int));
    fprintf(stderr, "apply\n");
    cm->apply(cmap);
    fprintf(stderr, "writeBuffer\n");
    opencl->writeBuffer("colourMap", cmap);

    fprintf(stderr, "setKernelArgs\n");
    setKernelArgs();

    fprintf(stderr, "Start initparticles\n");
    opencl->step("initParticles");
    fprintf(stderr, "End initparticles\n");
}

void prepare() {

    #ifdef MANDEL_GPU_USE_DOUBLE
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

    iterCount += config->steps;

    chrono::high_resolution_clock::time_point temp = chrono::high_resolution_clock::now();
    chrono::duration<float> time_span = chrono::duration_cast<chrono::duration<float>>(temp - timePoint);
    frameTime = time_span.count();
    
    fprintf(stderr, "Step = %d, time = %.4g            \n", frameCount / 2, frameTime);
    fprintf(stderr, "\x1b[%dA", opencl->printCount + 1);
    timePoint = temp; 
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
    superSample = config->anti_alias_samples;

    timePoint = chrono::high_resolution_clock::now();

    prepare();

    fprintf(stderr, "atexit\n");
    atexit(&cleanAll);

    fprintf(stderr, "glutInit\n");
    glutInit(&argc, argv);
    fprintf(stderr, "createMainWindow\n");
    createMainWindow("Main", config->width, config->height);
    fprintf(stderr, "glutDisplayFunc\n");
    glutDisplayFunc(&display);

    fprintf(stderr, "glutIdleFunc\n");
    glutIdleFunc(&display);

    fprintf(stderr, "\nStarting main loop\n\n");
    glutMainLoop();

    return 0;
}
