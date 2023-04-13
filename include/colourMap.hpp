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
    ColourMap(std::vector<ColourFloat> colours, size_t size, bool symmetric = false);
    ColourMap(std::vector<ColourInt> colours, size_t size, bool symmetric = false);
    
    
    void apply(float *colourMap);
    void apply(unsigned int *colourMap);
    size_t getColorCount();

private:
    size_t m_size;
    size_t m_color_count;
    std::vector< std::vector<float> > map;
    void generate(std::vector<float> x, std::vector< std::vector<float> > y, bool symmetric);
    std::vector<float> get(float p);
};

ColourMap *ColourMapFromFile(char *fn, size_t size);
extern unsigned int *cmap;
extern ColourMap *cm;

#endif
