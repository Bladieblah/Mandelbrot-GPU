#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>

#define USE_DOUBLE

#ifdef USE_DOUBLE
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

    bool profile = true;
    bool verbose = true;
    bool useGpu = true;

    float_type scale = 1.3;
    float_type center_x = -0.5;
    float_type center_y = 0.;
    float_type theta = 0.;

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
        {"scale", {'f', (void *)&scale}},
        {"center_x", {'f', (void *)&center_x}},
        {"center_y", {'f', (void *)&center_y}},
        {"theta", {'f', (void *)&theta}},
        {"profile", {'b', (void *)&profile}},
        {"verbose", {'b', (void *)&verbose}},
        {"useGpu", {'b', (void *)&useGpu}},
    };
};

#endif
