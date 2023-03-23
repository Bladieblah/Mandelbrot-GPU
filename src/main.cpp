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

void setKernelArgs() {
    opencl->setKernelBufferArg("seedNoise", 0, "randomState");
    opencl->setKernelBufferArg("seedNoise", 1, "randomIncrement");
    opencl->setKernelBufferArg("seedNoise", 2, "initState");
    opencl->setKernelBufferArg("seedNoise", 3, "initSeq");
    
    opencl->setKernelBufferArg("initParticles", 0, "particles");
    opencl->setKernelArg("initParticles", 1, sizeof(ViewSettingsCL), &(viewMainCL));

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

    double scaleY = 1.3;
    viewMain = {
        scaleY / (double)config->height * (double)config->width, scaleY,
        -0.5, 0.,
        0., 0., 1.,
        (unsigned long)config->width, (unsigned long)config->height
    };

    defaultView = viewMain;
    transformView();

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

// typedef struct IntPair {
//     bool sign;
//     uint64_t integ;
//     uint64_t fract;
// } IntPair;

// bool operator>(IntPair x0, IntPair x1) {
//     uint64_t x8 = x0.integ;
//     uint64_t x9 = x1.integ;
//     int result = (x8 <= x9) ? 0 : 1;
//     uint64_t y8 = x0.fract;
//     uint64_t y9 = x1.fract;
//     result |= (y8 > y9) << 1;
//     return result;
// }

// // Multiplication
// IntPair mul_intpair1(IntPair a, IntPair b) {
//     IntPair result;
    
//     result.sign = a.sign == b.sign;
//     result.integ = a.integ * b.integ + (a.integ * (b.fract >> 8) >> 56) + (b.integ * (a.fract >> 8) >> 56);
//     result.fract = a.integ * b.fract + b.integ * a.fract + (((a.fract >> 32) * (b.fract >> 32))) + (((a.fract << 32 >> 32) * (b.fract >> 32)) >> 32) + (((b.fract << 32 >> 32) * (a.fract >> 32)) >> 32);
    
//     return result;
// }

// IntPair mul_intpair2(IntPair a, IntPair b) {
//     IntPair result;
    
//     result.sign = a.sign == b.sign;
//     result.integ = a.integ * b.integ + (a.integ * (b.fract >> 8) >> 56) + (b.integ * (a.fract >> 8) >> 56);
//     result.fract = a.integ * b.fract + b.integ * a.fract + (((a.fract >> 32) * (b.fract >> 32))) + (((a.fract & (~0ull >> 32)) * (b.fract >> 32)) >> 32) + (((b.fract & (~0ull >> 32)) * (a.fract >> 32)) >> 32);
    
//     return result;
// }

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

    glutIdleFunc(&display);

    fprintf(stderr, "\nStarting main loop\n\n");
    glutMainLoop();

    // size_t N = 10000000;
    // IntPair *a = (IntPair *)malloc(N * sizeof(IntPair));
    // IntPair *b = (IntPair *)malloc(N * sizeof(IntPair));
    // IntPair *c = (IntPair *)malloc(N * sizeof(IntPair));
    
    // for (int i = 0; i < N; i++) {
    //     a[i] = {UNI() > 0.5, ((uint64_t)pcg32_random()), ((uint64_t)pcg32_random()) << 32};
    //     b[i] = {UNI() > 0.5, ((uint64_t)pcg32_random()), ((uint64_t)pcg32_random()) << 32};
    // }


    // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
    
    // for (int i = 0; i < N; i++) {
    //     c[i] = mul_intpair1(a[i], b[i]);
    //     // fprintf(stderr, "%s%llu.%llu\n", c.sign ? "+" : "-", c.integ, c.fract);
    // }

    // chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
    // chrono::duration<float> time_span = chrono::duration_cast<chrono::duration<float>>(end - start);
    // fprintf(stderr, "Branched time = %.4g\n", time_span.count());
    
    // int res = 0;
    // for (int i = 0; i < N; i++) {
    //     res += c[i].integ;
    // }
    // fprintf(stderr, "%d\n", res);


    // start = chrono::high_resolution_clock::now();

    // for (int i = 0; i < N; i++) {
    //     c[i] = mul_intpair2(a[i], b[i]);
    //     // fprintf(stderr, "%s%llu.%llu\n", c.sign ? "+" : "-", c.integ, c.fract);
    // }

    // end = chrono::high_resolution_clock::now();
    // time_span = chrono::duration_cast<chrono::duration<float>>(end - start);
    // fprintf(stderr, "Branchless time = %.4g\n", time_span.count());
    
    // res = 0;
    // for (int i = 0; i < N; i++) {
    //     res += c[i].integ;
    // }
    // fprintf(stderr, "%d\n", res);


    return 0;
}
