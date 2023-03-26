#include <vector>

#include "colourMap.hpp"
#include "interp.hpp"

using namespace std;

colourMap::colourMap(vector<float> _x, vector< vector<float> > _y, int _size) {
    int i;
    
    size = _size;
    
    Interp1d interp(_x, _y);
    
    float p = 0;
    float dp = 1. / (size - 1.);
    
    for (i=0; i<size; i++) {
        vector<float> result = interp.getValue(p);
        map.push_back(result);
        
        p += dp;
    }
}

vector<float> colourMap::get(float p) {
    int i = (int)(p * size);
    
    return map[i];
}

void colourMap::apply(float *colourMap) {
    for (int i=0; i<size; i++)
        for (int j=0; j<3; j++)
            colourMap[3 * i + j] = map[i][j];
}
