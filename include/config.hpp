#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>

#define USE_DOUBLE

typedef struct Setting {
    char type;
    void *pointer;
} Setting;

class Config {
public:
    unsigned int width = 1080;
    unsigned int height = 720;
    unsigned int steps = 100;

    bool profile = true;
    bool verbose = true;

    float scale = 1.3;
    float center_x = -0.5;
    float center_y = 0.;
    float theta = 0.;

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
        {"scale", {'f', (void *)&scale}},
        {"center_x", {'f', (void *)&center_x}},
        {"center_y", {'f', (void *)&center_y}},
        {"theta", {'f', (void *)&theta}},
        {"profile", {'b', (void *)&profile}},
        {"verbose", {'b', (void *)&verbose}},
    };
};

#endif
