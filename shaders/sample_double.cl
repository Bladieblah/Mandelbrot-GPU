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
    ulong new_fra = a.fract + b.fract;
    ulong new_int = a.integ + b.integ + (int)((new_fra < a.fract || new_fra < b.fract));
    
    bool is_larger = ipgt(a, b);
    
    new_fra  = a.sign == b.sign ? new_fra  : (is_larger ? a.fract - b.fract : b.fract - a.fract);
    new_int  = a.sign == b.sign ? new_int  : (is_larger ? a.integ - b.integ - (int)(new_fra > a.fract) : b.integ - a.integ - (int)(new_fra > b.fract));
    bool new_sign = a.sign == b.sign ? a.sign : (is_larger ? a.sign : b.sign);
    
    return (IntPair){new_sign, new_int, new_fra};
}

IntPair add_intpair_int(ulong a, IntPair b) {
    ulong new_fra = b.fract;
    ulong new_int = a + b.integ + (int)((new_fra < b.fract));
    
    bool is_larger = a > b.integ;
    
    new_fra       = b.sign ? new_fra : (is_larger ? -b.fract : b.fract);
    new_int       = b.sign ? new_int : (is_larger ? a - b.integ - (int)(new_fra > 0) : b.integ - a - (int)(new_fra > b.fract));
    bool new_sign = b.sign ? true  : (is_larger ? true : b.sign);
    
    return (IntPair){new_sign, new_int, new_fra};
}

// Subtraction
IntPair sub_intpair(IntPair a, IntPair b) {
    b.sign = !b.sign;
    return add_intpair(a, b);
}

// Subtraction
IntPair sub_intpair_int(IntPair a, int b) {
    a.sign = !a.sign;
    IntPair result = add_intpair_int(b, a);
    result.sign = !result.sign;
    return result;
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

// Multiplication
// Assumes a < 256
IntPair square_intpair(IntPair a) {
    IntPair result;
    
    result.sign = true;
    result.integ = a.integ * a.integ + (a.integ * (a.fract >> 8) >> 55);
    result.fract = 2 * a.integ * a.fract + (((a.fract >> 32) * (a.fract >> 32))) + (((a.fract & (~0ull >> 32)) * (a.fract >> 32)) >> 31);
    
    return result;
}

// Multiplication
// Assumes a,b < 256
IntPair mul_intpair_int(ulong a, IntPair b) {
    IntPair result;
    
    result.sign = b.sign;
    result.integ = a * b.integ + (a * (b.fract >> 8) >> 56);
    result.fract = a * b.fract;
    
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


/**
 * Complex double math stuff
 */

typedef struct {
    IntPair x, y;
} ComplexDouble;

inline ComplexDouble add_complex(ComplexDouble a, ComplexDouble b) {
    return (ComplexDouble) {
        add_intpair(a.x, b.x),
        add_intpair(a.y, b.y)
    };
}

inline ComplexDouble sub_complex(ComplexDouble a, ComplexDouble b) {
    return (ComplexDouble) {
        sub_intpair(a.x, b.x),
        sub_intpair(a.y, b.y)
    };
}

inline ComplexDouble cmuld(ComplexDouble a, ComplexDouble b) {
return (ComplexDouble){
    sub_intpair(mul_intpair(a.x, b.x), mul_intpair(a.y, b.y)), 
    add_intpair(mul_intpair(a.x, b.y), mul_intpair(a.y, b.x))
};
}

inline ComplexDouble csquared(ComplexDouble z) {
return (ComplexDouble){
    sub_intpair(square_intpair(z.x), square_intpair(z.y)),
    mul_intpair_int(2, mul_intpair(z.y, z.x))
};
}

inline IntPair cnorm2d(ComplexDouble z) {
return mul_intpair(square_intpair(z.x), square_intpair(z.y));
}

//  inline float cdotd(float2 a, float2 b) {
//     return a.x * b.x + a.y * b.y;
//  }

/**
 * Checks
 */

constant IntPair RADIUS_1 = {true, 0ULL, 166481865265228704ULL};
constant IntPair RADIUS_3 = {true, 0ULL, 35712896526701688ULL};
constant IntPair RADIUS_4 = {true, 0ULL, 25253592636908372ULL};
constant IntPair RADIUS_5 = {true, 0ULL, 9338664187315460ULL};


constant ComplexDouble CENTER_1 = {{false, 0ULL, 4334763900ULL},          {true, 0ULL, 13740274379125600256ULL}};
constant ComplexDouble CENTER_2 = {{false, 0ULL, 4334763900ULL},          {true, 0ULL, 0ULL}};
constant ComplexDouble CENTER_3 = {{true,  0ULL, 5206988104807323648ULL}, {true, 0ULL, 9777892556023484416ULL}};
constant ComplexDouble CENTER_4 = {{false, 0ULL, 4334763900ULL},          {true, 0ULL, 10381195974969425920ULL}};
constant ComplexDouble CENTER_5 = {{true,  0ULL, 7000790030624987136ULL}, {true, 0ULL, 617841052337451212ULL}};

inline bool isValid(ComplexDouble coord) {
    IntPair c2 = cnorm2d(coord);
    IntPair a = coord.x;

    if (c2.integ >= 4) {
        return false;
    }
    
    // Main bulb
    IntPair test1 = add_intpair(sub_intpair(mul_intpair_int(256ULL, square_intpair(c2)), mul_intpair_int(96ULL, c2)), mul_intpair_int(32ULL, a));
    if (test1.integ < 3 || !test1.sign) {
        return false;
    }

    // Head
    IntPair test2 = mul_intpair_int(16ULL, add_intpair_int(1ULL, add_intpair(c2, mul_intpair_int(2, a))));
    if (test2.integ < 1 || !test2.sign) {
        return false;
    }

    coord.y.sign = true;

    // 2-step bulbs
    if (ipgt(RADIUS_1, cnorm2d(sub_complex(coord, CENTER_1)))) {
        return false;
    }

    // 3-step bulbs
    if (ipgt(RADIUS_3, cnorm2d(sub_complex(coord, CENTER_2)))) {
        return false;
    }
    if (ipgt(RADIUS_3, cnorm2d(sub_complex(coord, CENTER_3)))) {
        return false;
    }

    // 4-step bulbs
    if (ipgt(RADIUS_4, cnorm2d(sub_complex(coord, CENTER_4)))) {
        return false;
    }
    if (ipgt(RADIUS_5, cnorm2d(sub_complex(coord, CENTER_5)))) {
        return false;
    }

    return true;
}

/**
 * Coordinate transformations
 */

typedef struct ViewSettings {
    IntPair scaleX, scaleY;
    IntPair centerX, centerY;
    IntPair theta, sinTheta, cosTheta;
    ulong sizeX, sizeY;
} ViewSettings;

inline ComplexDouble rotateCoords(ComplexDouble coords, ViewSettings view) {
    IntPair tmp = mul_intpair(view.sinTheta, coords.x);
    tmp.sign = !tmp.sign;

    return (ComplexDouble) {
        add_intpair(mul_intpair(view.cosTheta, coords.x), mul_intpair(view.sinTheta, coords.y)),
        add_intpair(tmp,                                  mul_intpair(view.cosTheta, coords.y))
    };
}

inline ComplexDouble screenToFractal(ComplexDouble screenCoord, ViewSettings view) {
    ComplexDouble tmp = rotateCoords((ComplexDouble){
        mul_intpair(screenCoord.x, view.scaleX),
        mul_intpair(screenCoord.y, view.scaleY)
    }, view);

    return (ComplexDouble){
        mul_intpair(tmp.x, view.centerX),
        mul_intpair(tmp.y, view.centerY)
    };
}

inline ComplexDouble pixelToScreen(ulong2 pixelCoord, ViewSettings view) {
    return (ComplexDouble) {
        div_ints((2 * pixelCoord.x), view.sizeX),
        div_ints((2 * pixelCoord.y), view.sizeY)
    };
}

 inline ComplexDouble pixelToFractal(ulong2 pixelCoord, ViewSettings view) {
    return screenToFractal(pixelToScreen(pixelCoord, view), view);
 }

/**
 * Kernels
 */

 typedef struct Particle {
    ComplexDouble pos, offset;
    unsigned int iterCount;
    bool escaped;
} Particle;

__kernel void initParticles(global Particle *particles, ViewSettings view) {
    const size_t x = get_global_id(0);
    const size_t y = get_global_id(1);
    const size_t W = get_global_size(0);
    
    size_t gid = (W * y + x);

    ComplexDouble offset = pixelToFractal((ulong){x, y}, view);

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
        if (cnorm2d(tmp.pos).integ > 4ULL) {
            tmp.escaped = true;
            break;
        }

        tmp.pos = add_complex(csquared(tmp.pos), tmp.offset);
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
