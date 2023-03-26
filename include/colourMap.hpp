#ifndef COLOURMAP_H
#define COLOURMAP_H

#include <vector>

typedef struct {
    float x;
    float rgb[3];
} Colour;

class ColourMap {
public:
    ColourMap(std::vector<Colour> colours, size_t size);
    
    void apply(float *colourMap);
    std::vector<float> get(float p);
    
    std::vector< std::vector<float> > map;
    
    size_t m_size;
};

#endif
