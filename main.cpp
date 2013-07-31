
#include "randodo.h"

#include <cstdlib>
#include <ctime> 

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "Usage: randodo <file_name> <generator_name>" << std::endl;
        return -1;
    }

    srand(time(NULL));

    std::string fileName = argv[1], generatorName = argv[2];

    Randodo::ConfigFile<> configFile;
    configFile.parse(fileName);

    auto &mapOfGenerators = configFile.getMapOfGenerators();
    auto iter = mapOfGenerators.find(generatorName);

    if (iter == mapOfGenerators.end()) {
        std::cerr << "Couldn't find specified file or generator" << std::endl;
        return -2;
    }

    std::stringstream stream;
    iter->second->generate(stream);

    std::cout << stream.str() << std::endl;

    return 0;
}

