# Randodo - random strings generator

## What is Randodo?

My goal is to create a simple & fast **random strings generator**, which should be available both as a **command-line utility** and a **C++ library**. Users should be able to control how the generated strings might look like by writing simple specifications, similar to typical **regular expressions**.

Randodo definitely **isn't finished yet**. It should already be quite a useful tool, but lots of optimizations & features which are planned are not done yet (zooming through the code might reveal a few TODOs and alikes); so if you find it useful but lacking a few things, please don't hesitate and tell me about it! This might be the motivation I need to finish it. :-)

## Use cases 

Randodo has been started as a tool useful for testing other stuff, so I would guess that **testing** will be the main use of Randodo. But I wouldn't mind being surprised!

## Examples

### Command-line tool

Let me show you an example of how you can use Randodo as a command-line utility. At first, you need to create a file with a specification for Randodo - let's call it `sample.txt`. Here's what's inside of it:
```
vrok@laptok:~/randodo$ cat sample.txt
male=(John|Paul|Martin|Hubert|Bozydar)
female=(Ann|Sharon|Liza|Janina)
names=($male|$female)
verb=(loves|hates|likes|ignores)
how_much=(| very{1,5} much)
result=$male $verb $female$how_much. By the way, here are 5 random letters: [a-zA-Z]{5}.
```

As you can see, each line in the file is in the form `Something=SomethingElse` - it means that this line defines a generator called `Something`, and its definition is `SomethingElse`. So, each line defines a generator (and every such generator has a name). A generator is just a regex, to which you later refer to generate random strings. Generators can embed each other to create more powerful generators (embedding generators works similarly to embedding strings in Perl/Bash/PHP - just place its name preceded by `$`) . Now, we can choose one of them to get a few strings - let's say that we'd like to have 3 strings from the last generator in the file, called `result`. This is how it can be done:

```
vrok@laptok:~/randodo$ ./randodo sample.txt result 3
Bozydar likes Janina very much. By the way, here are 5 random letters: sEBOM.
John hates Ann very very much. By the way, here are 5 random letters: uLQMt.
Bozydar likes Sharon. By the way, here are 5 random letters: wDgMR.
```

### C++ library

As an example of Randodo's usage, let's study the code of the `randodo` command line utility.

Firstly, we should solve the standard logistic problems - compiling, linking, etc. This should be simple: all Randodo code lives in a single header file, so you just include it:

```c++
#include "randodo.h"
```

Our application will be able to accept 3 parameters: name of the specification file, name of the generator in the file, and the number of random strings which are to be generated (this will be set by default to 1).

```c++
int main(int argc, char **argv)
{
    if (argc < 3) {
        std::cerr << "Usage: randodo <file_name> <generator_name> [how_many=1]" << std::endl;
        return -1;
    }

    srand(time(NULL)); // By default, Randodo uses standard POSIX mechanism to get random numbers.

    std::string fileName = argv[1], generatorName = argv[2];

    int howMany = 1; // This variable tells us how many strings we ought to generate.
    if (argc > 3) {
        howMany = atoi(argv[3]);
    }

    // Here will go rest of the code.

    return 0;
}
```

This was all very simple. Now it's time to use Randodo. Randodo's basic class (or rather a class template) is `ConfigFile`, which represents a Randodo specification file. It parses the file during construction, and after that you can access it's generators (placed in a std::map of unique_ptrs, where names are keys). Here's the finished program:

```c++
#include "randodo.h"

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::cerr << "Usage: randodo <file_name> <generator_name> [how_many=1]" << std::endl;
        return -1;
    }

    srand(time(NULL)); // By default, Randodo uses standard POSIX mechanism to get random numbers.

    std::string fileName = argv[1], generatorName = argv[2];

    int howMany = 1; // This variable tells us how many strings we ought to generate.
    if (argc > 3) {
        howMany = atoi(argv[3]);
    }

    Randodo::ConfigFile<> configFile(fileName);

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
```

That was it. Now a few more details, if you're interested. You might have noticed that diamond (`<>`) here: `Randodo::ConfigFile<>`. That is because we used default values for `ConfigFile` class template. If you took a look at the beginning of its definition, that's what you would see:

```c++
template<typename FileReader = PlainFileReader,
         typename RandNumGenerator = PlainRandomNumberGenerator>
class ConfigFile
{
// Here goes the rest
```

I hope that names are a little self-descriptive (I tried!), so I won't repeat myself. But an obvious conclusion from reading them would be this: **It is possible to alter how `ConfigFile` reads files and generates random numbers by providing you own policy classes.** The protocols they have to implement are as simple as possible, for details take a look at definitions of the default ones (`PlainFileReader` and `PlainRandomNumberGenerator`).

TODO: **It is also possible to parse and use a single regex, without specification files, etc.**
