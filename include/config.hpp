#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>

#ifdef MANDEL_GPU_USE_DOUBLE
#define float_type double
#else
#define float_type float
#endif

typedef struct Setting {
    char type;
    void *pointer;
} Setting;

class Config {
public:
    unsigned int width = 1080;
    unsigned int height = 720;
    unsigned int steps = 100;
    unsigned int anti_alias_samples = 2;
    
    unsigned int num_colours = 1000;
    unsigned int colour_offset = 0;
    float colour_variance = 3;

    bool profile = true;
    bool verbose = true;
    bool useGpu = true;

    char colour_file[100];

    float_type scale = 1.3;
    float_type center_x = -0.5;
    float_type center_y = 0.;
    float_type theta = 0.;
    
    float stripe_freq = 3.;
    float stripe_mix = 0.2;

    Config(char *filename);
    void printValues();

private:
    void processLine(std::string line);
    void setValue(std::string name, char *value);
    void printSetting(std::string name, Setting setting);

    const std::map<std::string, Setting> typeMap = {
        {"width", {'i', (void *)&width}},
        {"height", {'i', (void *)&height}},
        {"steps", {'i', (void *)&steps}},
        {"anti_alias_samples", {'i', (void *)&anti_alias_samples}},
        
        {"num_colours", {'i', (void *)&num_colours}},
        {"colour_offset", {'i', (void *)&colour_offset}},
        {"colour_variance", {'f', (void *)&colour_variance}},
        
        {"scale", {'d', (void *)&scale}},
        {"center_x", {'d', (void *)&center_x}},
        {"center_y", {'d', (void *)&center_y}},
        {"theta", {'d', (void *)&theta}},
        {"stripe_freq", {'f', (void *)&stripe_freq}},
        {"stripe_mix", {'f', (void *)&stripe_mix}},
        {"profile", {'b', (void *)&profile}},
        {"verbose", {'b', (void *)&verbose}},
        {"useGpu", {'b', (void *)&useGpu}},
        {"colour_file", {'s', (void *)colour_file}},
    };
};

extern Config *config;

#endif
