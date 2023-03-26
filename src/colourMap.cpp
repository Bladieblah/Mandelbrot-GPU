#include <vector>

#include "colourMap.hpp"
#include "interp.hpp"

using namespace std;

ColourMap::ColourMap(vector<ColourFloat> colours, size_t size) {
    m_size = size;

    vector<float> x;
    vector< vector<float> > y;

    for (ColourFloat colour : colours) {
        vector<float> tmp(colour.rgb, colour.rgb + 3 * sizeof(float));
        x.push_back(colour.x);
        y.push_back(tmp);
    }
    
    generate(x, y);
}

ColourMap::ColourMap(vector<ColourInt> colours, size_t size) {
    m_size = size;

    vector<float> x;
    vector< vector<float> > y;

    for (ColourInt colour : colours) {
        vector<float> tmp;
        for (size_t i = 0; i < 3; i++) {
            tmp.push_back((float)colour.rgb[i] / 255.);
        }
        
        x.push_back(colour.x);
        y.push_back(tmp);
    }
    
    generate(x, y);
}

void ColourMap::generate(vector<float> x, vector< vector<float> > y) {
    Interp1d interp(x, y);
    
    float p = 0;
    float dp = 1. / (m_size - 1.);
    
    for (size_t i = 0; i < m_size; i++) {
        vector<float> result = interp.getValue(p);
        map.push_back(result);
        
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
