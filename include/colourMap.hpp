#ifndef COLOURMAP_H
#define COLOURMAP_H

#include <vector>

class colourMap {
public:
    colourMap(std::vector<float> _x, std::vector< std::vector<float> > _y, int _size);
    
    void apply(float *colourMap);
    std::vector<float> get(float p);
    
    std::vector< std::vector<float> > map;
    
    int size;
};

#endif
