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
  * Double precision stuff 
  */

typedef struct IntPair {
    bool sign;
    ulong integ;
    ulong fract;
} IntPair;

bool ipgt(IntPair x0, IntPair x1) {
    int result = (x0.integ <= x1.integ) ? 0 : 1;
    result |= (x0.fract > x1.fract) << 1;
    return result;
}

IntPair add_intpair(IntPair a, IntPair b) {
    IntPair result;

    ulong new_fra = a.fract + b.fract;
    ulong new_int = a.integ + b.integ + (int)((new_fra < a.fract || new_fra < b.fract));
    
    bool is_larger = ipgt(a, b);
    
    new_fra  = a.sign == b.sign ? new_fra  : (is_larger ? a.fract - b.fract : b.fract - a.fract);
    new_int  = a.sign == b.sign ? new_int  : (is_larger ? a.integ - b.integ - (int)(new_fra > a.fract) : b.integ - a.integ - (int)(new_fra > b.fract));
    bool new_sign = a.sign == b.sign ? a.sign : (is_larger ? a.sign : b.sign);
    
    return (IntPair){new_sign, new_int, new_fra};
}

// Subtraction
IntPair sub_intpair(IntPair a, IntPair b) {
    b.sign = !b.sign;
    return add_intpair(a, b);
}

// Multiplication
// Assumes a,b < 256
IntPair mul_intpair(IntPair a, IntPair b) {
    IntPair result;
    
    result.sign = a.sign == b.sign;
    result.integ = a.integ * b.integ + (a.integ * (b.fract >> 8) >> 56) + (b.integ * (a.fract >> 8) >> 56);
    result.fract = a.integ * b.fract + b.integ * a.fract + (((a.fract >> 32) * (b.fract >> 32))) + (((a.fract & (~0ull >> 32)) * (b.fract >> 32)) >> 32) + (((b.fract & (~0ull >> 32)) * (a.fract >> 32)) >> 32);
    
    return result;
}

// Division
// Assumes a%b < 2^14
IntPair div_ints(ulong a, ulong b) {
    IntPair result;
    
    result.integ = a / b;
    result.fract = (((a % b) << 50) / b) << 14;
    
    return result;
}

double to_double(IntPair num) {
    return (num.sign ? 1 : -1) * num.integ + (((double)num.fract) / (~0ULL));
}

IntPair to_pair(double num) {
    return (IntPair){
        num > 0, (ulong)num, (ulong)((num - (ulong)num) * (~0ULL))
    };
}


/**
 * Complex double math stuff
 */

// typedef struct {
//     IntPair x, y;
// } ComplexDouble;

//  inline ComplexDouble cmuld(ComplexDouble a, ComplexDouble b) {
//     return (ComplexDouble){
//         sub_intpair(mul_intpair(a.x, b.x), mul_intpair(a.y, b.y)), 
//         add_intpair(mul_intpair(a.x, b.y), mul_intpair(a.y, b.x))
//     };
//  }

//  inline ComplexDouble csquared(ComplexDouble z) {
//     FloatPair two = {2., 0.};

//     return (ComplexDouble){
//         sub(square(z.x), square(z.y)),
//         mul(two, mul(z.y, z.x))
//     };
//  }

//  inline FloatPair cnorm2d(ComplexDouble z) {
//     return add(square(z.x), square(z.y));
//  }

//  inline float cdotd(float2 a, float2 b) {
//     return a.x * b.x + a.y * b.y;
//  }

/**
 * Checks
 */

constant float2 CENTER_1 = {-0.1225611668766536, 0.7448617666197446};
constant float RADIUS_1 = 0.095;

constant float2 CENTER_2 = {-1.3107026413368228, 0};
constant float2 CENTER_3 = {0.282271390766914, 0.5300606175785252};
constant float RADIUS_3 = 0.044;

constant float2 CENTER_4 = {-0.5043401754462431, 0.5627657614529813};
constant float RADIUS_4 = 0.037;
constant float2 CENTER_5 = {0.3795135880159236, 0.3349323055974974};
constant float RADIUS_5 = 0.0225;

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
    if (cnorm2(coord - CENTER_1) < RADIUS_1 * RADIUS_1) {
        return false;
    }

    // 3-step bulbs
    if (cnorm2(coord - CENTER_2) < RADIUS_3 * RADIUS_3) {
        return false;
    }
    if (cnorm2(coord - CENTER_3) < RADIUS_3 * RADIUS_3) {
        return false;
    }

    // 4-step bulbs
    if (cnorm2(coord - CENTER_4) < RADIUS_4 * RADIUS_4) {
        return false;
    }
    if (cnorm2(coord - CENTER_5) < RADIUS_5 * RADIUS_5) {
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
    ulong sizeX, sizeY;
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

inline float2 pixelToScreen(ulong2 pixelCoord, ViewSettings view) {
    return (float2) {
        (2. * pixelCoord.x) / (float)view.sizeX - 1.,
        (2. * pixelCoord.y) / (float)view.sizeY - 1.
    };
}

 inline float2 pixelToFractal(ulong2 pixelCoord, ViewSettings view) {
    return screenToFractal(pixelToScreen(pixelCoord, view), view);
 }

/**
 * Kernels
 */

 typedef struct Particle {
    float2 pos, offset;
    unsigned int iterCount;
    bool escaped;
} Particle;

__kernel void initParticles(global Particle *particles, ViewSettings view) {
    const size_t x = get_global_id(0);
    const size_t y = get_global_id(1);
    const size_t W = get_global_size(0);
    
    size_t gid = (W * y + x);

    float2 offset = pixelToFractal((ulong2){x, y}, view);

    particles[gid].pos = offset;
    particles[gid].offset = offset;
    particles[gid].iterCount = 1;
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

__kernel void renderImage(
    global Particle *particles,
    global unsigned int *data
) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    
    const int W = get_global_size(0);
    
    int index = (W * y + x);

    if (!particles[index].escaped) {
        data[3 * index] = 0;
        data[3 * index + 1] = 0;
        data[3 * index + 2] = 0;
        return;
    }

    float period = 255;
    float count = (float)particles[index].iterCount;
    float ease = clamp(count * 0.01, 0., 1.);

    float phase = count / 64.;

    data[3 * index] = ease * pown(cos(phase), 2) * 2147483647;
    data[3 * index + 1] = ease * pown(sin(phase), 2) * 2147483647;
    data[3 * index + 2] = ease * pown(cos(phase + M_1_PI * 0.25), 2) * 2147483647;
}
