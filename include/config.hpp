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
    unsigned int particle_count = 100;

    unsigned int width = 1080;
    unsigned int height = 720;
    unsigned int steps = 100;

    bool profile = true;
    bool verbose = true;

    Config(char *filename);
    void printValues();

private:
    void processLine(std::string line);
    void setValue(std::string name, char *value);
    void printSetting(std::string name, Setting setting);

    const std::map<std::string, Setting> typeMap = {
        {"particle_count", {'i', (void *)&particle_count}},
        {"width", {'i', (void *)&width}},
        {"height", {'i', (void *)&height}},
        {"frame_steps", {'i', (void *)&steps}},
        {"profile", {'b', (void *)&profile}},
        {"verbose", {'b', (void *)&verbose}},
    };
};

#endif
