
#include "randodo.h"

#include <cstdlib>
#include <ctime> 

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::cerr << "Usage: randodo <file_name> <generator_name> [how_many=1]" << std::endl;
        return -1;
    }

    srand(time(NULL));

    std::string fileName = argv[1], generatorName = argv[2];

    int howMany = 1;
    if (argc > 3) {
        howMany = atoi(argv[3]);
    }

    Randodo::ConfigFile<> configFile;
    configFile.parse(fileName);

    auto &mapOfGenerators = configFile.getMapOfGenerators();
    auto iter = mapOfGenerators.find(generatorName);

    if (iter == mapOfGenerators.end()) {
        std::cerr << "Couldn't find specified file or generator" << std::endl;
        return -2;
    }

    for (int i = 0; i < howMany; ++i) {
        std::stringstream stream;
        iter->second->generate(stream);
        std::cout << stream.str() << std::endl;
    }

    return 0;
}

