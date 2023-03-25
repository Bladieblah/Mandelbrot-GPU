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
        {"profile", {'b', (void *)&profile}},
        {"verbose", {'b', (void *)&verbose}},
    };
};

#endif
