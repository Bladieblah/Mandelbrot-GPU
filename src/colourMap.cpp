#include <vector>

#include "colourMap.hpp"
#include "interp.hpp"

using namespace std;


vector<ColourInt> defaultColours = {
    {0.0, {0, 10, 2}},
    {0.05, {3, 34, 12}},
    {0.2, {8, 76, 33}},
    {0.4, {13, 117, 53}},
    {0.7, {10, 172, 102}},
    {0.9, {63, 216, 112}},
    {1.0, {25, 236, 173}},
};

// --------------------------- IO ---------------------------

ColourMap *ColourMapFromInt(FILE *f, size_t size, bool symmetric) {
    ColourInt tmp;
    vector<ColourInt> colours;

    while (fscanf(f, "%f, {%d, %d, %d}\n", &(tmp.x), &(tmp.rgb[0]), &(tmp.rgb[1]), &(tmp.rgb[2])) == 4) {
        colours.push_back(ColourInt(tmp));
    }

    return new ColourMap(colours, size, symmetric);
}

ColourMap *ColourMapFromFloat(FILE *f, size_t size, bool symmetric) {
    ColourFloat tmp;
    vector<ColourFloat> colours;

    while (fscanf(f, "%f, {%f, %f, %f}\n", &(tmp.x), &(tmp.rgb[0]), &(tmp.rgb[1]), &(tmp.rgb[2])) == 4) {
        colours.push_back(ColourFloat(tmp));
    }

    return new ColourMap(colours, size, symmetric);
}

ColourMap *ColourMapFromFile(char *fn, size_t size) {
    FILE *f;
    char kind = 'a';
    int symmetric = 0;

    f = fopen(fn, "r");

    if (!f) {
        fprintf(stderr, "Error loading cm.\n");
        return new ColourMap(defaultColours, size, false);
    }

    if (fscanf(f, "kind = %c\n", &kind) == EOF || fscanf(f, "symmetric = %d\n", &symmetric) == EOF) {
        fprintf(stderr, "Error loading cm.\n");
        return new ColourMap(defaultColours, size, false);
    }

    switch (kind) {
        case 'i':
            return ColourMapFromInt(f, size, symmetric);
        case 'f':
            return ColourMapFromFloat(f, size, symmetric);
        default:
            break;
    }

    fprintf(stderr, "Error loading cm.\n");
    return new ColourMap(defaultColours, size, true);
}

// --------------------------- Class implementation ---------------------------

ColourMap::ColourMap(vector<ColourFloat> colours, size_t size, bool symmetric) {
    m_size = size;
    m_color_count = colours.size();
    m_symmetric = symmetric;

    map.reserve(m_size);

    for (ColourFloat colour : colours) {
        vector<float> tmp(colour.rgb, colour.rgb + 3 * sizeof(float));
        m_x.push_back(colour.x);
        m_y.push_back(tmp);
    }
    
    generate();
}

ColourMap::ColourMap(vector<ColourInt> colours, size_t size, bool symmetric) {
    m_size = size;
    m_color_count = colours.size();
    m_symmetric = symmetric;

    map.reserve(m_size);

    for (ColourInt colour : colours) {
        vector<float> tmp;
        for (size_t i = 0; i < 3; i++) {
            tmp.push_back((float)colour.rgb[i] / 255.);
        }
        
        m_x.push_back(colour.x);
        m_y.push_back(tmp);
    }
    
    generate();
}

void ColourMap::generate() {
    // fprintf(stderr, "%.2f, %.2f, %.2f\n", m_y[0][0], m_y[0][1], m_y[0][2]);
    Interp1d interp(m_x, m_y);
    
    float p = 0;
    float dp = 1. / (m_size - 1.);
    
    for (size_t i = 0; i < m_size; i++) {
        vector<float> result = interp.getValue(m_symmetric ? 1 - fabs(2 * p - 1) : p);
        map[i] = result;
        
        p += dp;
    }
}

vector<float> ColourMap::get(float p) {
    size_t i = (size_t)(p * m_size);
    
    return map[i];
}

void ColourMap::apply(float *colourMap) {
    for (size_t i = 0; i < m_size; i++) {
        for (size_t j = 0; j < 3; j++) {
            colourMap[3 * i + j] = map[i][j];
        }
    }
}

void ColourMap::apply(unsigned int *colourMap) {
    for (size_t i = 0; i < m_size; i++) {
        for (size_t j = 0; j < 3; j++) {
            colourMap[3 * i + j] = (unsigned int)(map[i][j] * UINT_MAX);
        }
    }
}

size_t ColourMap::getColorCount() {
    return m_color_count;
}

void ColourMap::save(char *fn) {
    fprintf(stderr, "Saving to %s          \n", fn);
    FILE *outFile = fopen(fn, "w");
    
    if (outFile) {
        fprintf(outFile, "kind = f\n");
        fprintf(outFile, "symmetric = %d\n", (int)m_symmetric);

        for (int i = 0; i < m_color_count; i++) {
            fprintf(outFile, "%f, {%f, %f, %f}\n", m_x[i], m_y[i][0], m_y[i][1], m_y[i][2]);
        }

        fclose(outFile);
    }
}
