__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;

/**
 * RNG stuff
 */

__constant ulong PCG_SHIFT = 6364136223846793005ULL;
__constant float PCG_MAX_1 = 4294967296.0;

#define PDF_SIDES 100
#define PDF_SIZE 201

#define UNIFORM_LOW 0.02425
#define UNIFORM_HIGH 0.97575

__constant float a[] = {
    -3.969683028665376e+01,
     2.209460984245205e+02,
    -2.759285104469687e+02,
     1.383577518672690e+02,
    -3.066479806614716e+01,
     2.506628277459239e+00
};

__constant float b[] = {
    -5.447609879822406e+01,
     1.615858368580409e+02,
    -1.556989798598866e+02,
     6.680131188771972e+01,
    -1.328068155288572e+01
};

__constant float c[] = {
    -7.784894002430293e-03,
    -3.223964580411365e-01,
    -2.400758277161838e+00,
    -2.549732539343734e+00,
     4.374664141464968e+00,
     2.938163982698783e+00
};

__constant float d[] = {
    7.784695709041462e-03,
    3.224671290700398e-01,
    2.445134137142996e+00,
    3.754408661907416e+00
};

inline float inverseNormalCdf(float u) {
    float q, r;

    if (u <= 0) {
        return -HUGE_VAL;
    }
    else if (u < UNIFORM_LOW) {
        q = sqrt(-2 * log(u));
        return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
            ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
    }
    else if (u <= UNIFORM_HIGH) {
        q = u - 0.5;
        r = q * q;
        
        return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
            (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1);
    }
    else if (u < 1) {
        q  = sqrt(-2 * log(1 - u));

        return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
            ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
    }
    else {
        return HUGE_VAL;
    }
}

inline ulong pcg32Random(global ulong *randomState, global ulong *randomIncrement, int x) {
    ulong oldstate = randomState[x];
    randomState[x] = oldstate * PCG_SHIFT + randomIncrement[x];
    uint xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint rot = oldstate >> 59u;
    uint pcg = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));

    return pcg;
}

__kernel void seedNoise(
    global ulong *randomState,
    global ulong *randomIncrement,
    global ulong *initState,
    global ulong *initSeq
) {
    const int x = get_global_id(0);

    randomState[x] = 0U;
    randomIncrement[x] = (initSeq[x] << 1u) | 1u;
    pcg32Random(randomState, randomIncrement, x);
    randomState[x] += initState[x];
    pcg32Random(randomState, randomIncrement, x);
}

inline float uniformRand(
    global ulong *randomState,
    global ulong *randomIncrement,
    int x
) {
    return (float)pcg32Random(randomState, randomIncrement, x) / PCG_MAX_1;
}

inline unsigned int randint(
    global ulong *randomState,
    global ulong *randomIncrement,
    int x,
    unsigned int maxVal
) {
    return pcg32Random(randomState, randomIncrement, x) % maxVal;
}

inline float gaussianRand(
    global ulong *randomState,
    global ulong *randomIncrement,
    int x
) {
    return inverseNormalCdf(uniformRand(randomState, randomIncrement, x));
}

/**
 * Complex math stuff
 */

 inline float2 cmul(float2 a, float2 b) {
    return (float2)(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
 }

 inline float2 csquare(float2 z) {
    return (float2)( z.x * z.x - z.y * z.y, 2 * z.y * z.x);
 }

 inline float cnorm2(float2 z) {
    return z.x * z.x + z.y * z.y;
 }

 inline float cnorm(float2 z) {
    return sqrt(cnorm2(z));
 }

 inline float cdot(float2 a, float2 b) {
    return a.x * b.x + a.y * b.y;
 }

/**
 * Checks
 */

constant float RADIUS_1 = 0.0937;
constant float RADIUS_2 = 0.0585;
constant float RADIUS_3 = 0.0435;
constant float RADIUS_4 = 0.0385;
constant float RADIUS_5 = 0.023;

constant float2 CENTER_1 = {-0.1251, 0.744};
constant float2 CENTER_2 = {-1.309, 0};
constant float2 CENTER_3 = {0.2815, 0.531};
constant float2 CENTER_4 = {-0.5045, 0.563};
constant float2 CENTER_5 = {0.379, 0.336};

inline bool isValid(float2 coord) {
    float c2 = cnorm2(coord);
    float a = coord.x;

    if (c2 >= 4) {
        return false;
    }
    
    // Main bulb
    if (256.0 * c2 * c2 - 96.0 * c2 + 32.0 * a < 3.0) {
        return false;
    }

    // Head
    if (16.0 * (c2 + 2.0 * a + 1.0) < 1.0) {
        return false;
    }

    coord.y = fabs(coord.y);

    // 2-step bulbs
    if (cnorm(coord - CENTER_1) < RADIUS_1) {
        return false;
    }

    // 3-step bulbs
    if (cnorm(coord - CENTER_2) < RADIUS_2) {
        return false;
    }
    if (cnorm(coord - CENTER_3) < RADIUS_3) {
        return false;
    }

    // 4-step bulbs
    if (cnorm(coord - CENTER_4) < RADIUS_4) {
        return false;
    }
    if (cnorm(coord - CENTER_5) < RADIUS_5) {
        return false;
    }

    return true;
}

/**
 * Coordinate transformations
 */

typedef struct ViewSettings {
    float scaleX, scaleY;
    float centerX, centerY;
    float theta, sinTheta, cosTheta;
    int sizeX, sizeY;
    int particlesX, particlesY;
} ViewSettings;

inline float2 rotateCoords(float2 coords, ViewSettings view) {
    return (float2) {
         view.cosTheta * coords.x + view.sinTheta * coords.y,
        -view.sinTheta * coords.x + view.cosTheta * coords.y
    };
}

inline float2 screenToFractal(float2 screenCoord, ViewSettings view) {
    float2 tmp = rotateCoords((float2){
        screenCoord.x * view.scaleX,
        screenCoord.y * view.scaleY
    }, view);

    return (float2){
        tmp.x + view.centerX,
        tmp.y + view.centerY
    };
}

inline float2 pixelToScreen(int2 pixelCoord, ViewSettings view) {
    return (float2) {
        (2. * pixelCoord.x) / (float)view.particlesX - 1.,
        (2. * pixelCoord.y) / (float)view.particlesY - 1.
    };
}

 inline float2 pixelToFractal(int2 pixelCoord, ViewSettings view) {
    return screenToFractal(pixelToScreen(pixelCoord, view), view);
 }

/**
 * Kernels
 */

 typedef struct Particle {
    float2 pos, offset;
    unsigned int iterCount;
    int escaped;
} Particle;

__kernel void initParticles(global Particle *particles, ViewSettings view) {
    const size_t x = get_global_id(0);
    const size_t y = get_global_id(1);
    const size_t W = get_global_size(0);
    
    size_t gid = (W * y + x);

    float2 offset = pixelToFractal((int2){x, y}, view);

    particles[gid].pos = offset;
    particles[gid].offset = offset;
    particles[gid].iterCount = 0;
    particles[gid].escaped = !isValid(offset);
}

__kernel void mandelStep(global Particle *particles, unsigned int stepCount) {
    const size_t x = get_global_id(0);
    const size_t y = get_global_id(1);
    const size_t W = get_global_size(0);

    size_t gid = (W * y + x);

    Particle tmp = particles[gid];

    if (tmp.escaped) {
        return;
    }

    for (size_t i = 0; i < stepCount; i++) {
        if (cnorm2(tmp.pos) > 4.) {
            tmp.escaped = true;
            break;
        }

        tmp.pos = csquare(tmp.pos) + tmp.offset;
        tmp.iterCount++;
    }

    particles[gid] = tmp;
}

inline void add_particle(
    global Particle *particles,
    global uint *data,
    global uint *colourMap,
    uint numColours,
    uint index,
    uint index_p,
    uint count0,
    float issq
) {
    if (particles[index_p].escaped != 1 || !particles[index_p].iterCount) {
        return;
    }

    particles[index_p].escaped++;

    float rad = cnorm2(particles[index_p].pos);
    float count = (float)particles[index_p].iterCount + 1 - log(log(rad)) / M_LN2;
    float ease = clamp(count * 0.03, 0., 1.) * issq;

    float phase = count / 64.;

    data[index]     += ease * pown(cos(phase), 2) * 0.7 * 2147483647;
    data[index + 1] += ease * pown(sin(phase), 2) * 2147483647;
    data[index + 2] += ease * pown(cos(phase + M_1_PI * 0.35), 2) * 1.5 * 2147483647;
}

__kernel void renderImage(
    global Particle *particles,
    global uint *data,
    global uint *colourMap,
    uint numColours,
    uint count0,
    uint superSample
) {
    const uint x = get_global_id(0);
    const uint y = get_global_id(1);
    const uint W = get_global_size(0);

    int index = 3 * (W * y + x);
    
    int ssq = superSample * superSample;
    float issq = 1. / (float)ssq;

    data[index] = 0;
    data[index + 1] = 0;
    data[index + 2] = 0;

    for (int i = 0; i < superSample; i++) {
        for (int j = 0; j < superSample; j++) {
            add_particle(
                particles,
                data,
                colourMap,
                numColours,
                index,
                (superSample * W * (superSample * y + j) + superSample * x + i), 
                count0,
                issq
            );
        }
    }
}

// function setPixelHSV(x, y, h, s, v) {
//     var r, g, b, i, f, p, q, t;
//     i = Math.floor(h * 6);
//     f = h * 6 - i;
//     p = v * (1 - s);
//     q = v * (1 - f * s);
//     t = v * (1 - (1 - f) * s);
//     switch (i % 6) {
//         case 0: r = v, g = t, b = p; break;
//         case 1: r = q, g = v, b = p; break;
//         case 2: r = p, g = v, b = t; break;
//         case 3: r = p, g = q, b = v; break;
//         case 4: r = t, g = p, b = v; break;
//         case 5: r = v, g = p, b = q; break;
//     }
//     setPixelRGB(x, y, Math.round(r * 255), Math.round(g * 255), Math.round(b * 255));
// }
