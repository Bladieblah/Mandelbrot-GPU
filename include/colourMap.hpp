#ifndef COLOURMAP_H
#define COLOURMAP_H

#include <vector>

typedef struct {
    float x;
    float rgb[3];
} ColourFloat;

typedef struct {
    float x;
    unsigned int rgb[3];
} ColourInt;

class ColourMap {
public:
    ColourMap(std::vector<ColourFloat> colours, size_t size);
    ColourMap(std::vector<ColourInt> colours, size_t size);
    
    void apply(float *colourMap);
    void apply(unsigned int *colourMap);
    std::vector<float> get(float p);

private:
    size_t m_size;
    std::vector< std::vector<float> > map;
    void generate(std::vector<float> x, std::vector< std::vector<float> > y);
};

#endif
