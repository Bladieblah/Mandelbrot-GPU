#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <math.h>

#include <GLUT/glut.h>

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

vector<BufferSpec> bufferSpecs;
void createBufferSpecs() {
    bufferSpecs = {
        {"particles", {NULL, config->width * config->height * sizeof(Particle)}},
        {"image",     {NULL, 3 * config->width * config->height * sizeof(uint32_t)}},

        {"randomState",     {NULL, config->particle_count * sizeof(uint64_t)}},
        {"randomIncrement", {NULL, config->particle_count * sizeof(uint64_t)}},
        {"initState",       {NULL, config->particle_count * sizeof(uint64_t)}},
        {"initSeq",         {NULL, config->particle_count * sizeof(uint64_t)}},
    };
}

vector<KernelSpec> kernelSpecs;
void createKernelSpecs() {
    kernelSpecs = {
        {"seedNoise",     {NULL, 2, {config->width, config->height}, {0, 0}, "seedNoise"}},
        {"initParticles", {NULL, 2, {config->width, config->height}, {0, 0}, "initParticles"}},
        {"mandelStep",    {NULL, 2, {config->width, config->height}, {0, 0}, "mandelStep"}},
        {"renderImage",   {NULL, 2, {config->width, config->height}, {0, 0}, "renderImage"}},
    };
}

#ifdef USE_DOUBLE
double to_double(IntPair num) {
    return (num.sign ? 1 : -1) * ((double)num.integ + (((double)num.fract) / (~0ULL)));
}
#endif

void setKernelArgs() {
    opencl->setKernelBufferArg("seedNoise", 0, "randomState");
    opencl->setKernelBufferArg("seedNoise", 1, "randomIncrement");
    opencl->setKernelBufferArg("seedNoise", 2, "initState");
    opencl->setKernelBufferArg("seedNoise", 3, "initSeq");
    
    opencl->setKernelBufferArg("initParticles", 0, "particles");
#ifdef USE_DOUBLE
    transformView();
    // fprintf(stderr, "ViewCenter = (%.5f, %.5f)\n", to_double(viewMainCL.centerX), to_double(viewMainCL.centerY));
    opencl->setKernelArg("initParticles", 1, sizeof(ViewSettingsCL), &(viewMainCL));
#else
    opencl->setKernelArg("initParticles", 1, sizeof(ViewSettings), &(viewMain));
#endif

    opencl->setKernelBufferArg("mandelStep", 0, "particles");
    opencl->setKernelArg("mandelStep", 1, sizeof(uint32_t), &(config->steps));

    opencl->setKernelBufferArg("renderImage", 0, "particles");
    opencl->setKernelBufferArg("renderImage", 1, "image");
}

void initPcg() {
    for (int i = 0; i < config->particle_count; i++) {
        initState[i] = pcg32_random();
        initSeq[i] = pcg32_random();
    }

    opencl->writeBuffer("initState", (void *)initState);
    opencl->writeBuffer("initSeq", (void *)initSeq);
    opencl->step("seedNoise");
    opencl->flush();

    free(initState);
    free(initSeq);
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
        true,
        config->verbose
    );

    setKernelArgs();
    initPcg();

    opencl->step("initParticles");
}

void prepare() {
    pcg32_srandom(time(NULL) ^ (intptr_t)&printf, (intptr_t)&(config->particle_count));

    initState = (uint64_t *)malloc(config->particle_count * sizeof(uint64_t));
    initSeq = (uint64_t *)malloc(config->particle_count * sizeof(uint64_t));

    #ifdef USE_DOUBLE
    double scaleY = 1.3;
    viewMain = {
        scaleY / (double)config->height * (double)config->width, scaleY,
        -0.5, 0.,
        0., 0., 1.,
        (unsigned long)config->width, (unsigned long)config->height
    };
    #else
    float scaleY = 1.3;
    viewMain = {
        scaleY / (float)config->height * (float)config->width, scaleY,
        -0.5, 0.,
        0., 0., 1.,
        (int)config->width, (int)config->height
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

// #ifdef USE_DOUBLE
//     Particle *particles = (Particle *)malloc(config->width * config->height * sizeof(Particle));
//     opencl->readBuffer("particles", particles);
//     int i = config->width * config->height / 3 + config->width / 25;
//     fprintf(stderr, "Particle %d at (%.5f, %.5f)\n", i, to_double(particles[i].pos.x), to_double(particles[i].pos.y));
// #else
//     Particle *particles = (Particle *)malloc(config->width * config->height * sizeof(Particle));
//     opencl->readBuffer("particles", particles);
//     int i = config->width * config->height / 3 + config->width / 25;
//     fprintf(stderr, "Particle %d at (%.5f, %.5f)\n", i, particles[i].pos.s[0], particles[i].pos.s[1]);
// #endif


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
    pcg32_srandom(time(NULL) ^ (intptr_t)&printf, (intptr_t)&(config->particle_count));
    config->printValues();

    frameTime = chrono::high_resolution_clock::now();

    prepare();

    atexit(&cleanAll);

    glutInit(&argc, argv);
    createMainWindow("Main", config->width, config->height);
    glutDisplayFunc(&display);

    // glutIdleFunc(&display);

    fprintf(stderr, "\nStarting main loop\n\n");
    glutMainLoop();

    return 0;
}
