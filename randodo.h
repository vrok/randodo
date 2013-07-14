/* License: GPL v3 */
/* Contact author: wrochniak@gmail.com */

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <memory>
#include <stack>
#include <algorithm>

namespace Randodo
{

class PlainFileReader
{
private:
    std::ifstream _file;

public:
    PlainFileReader(const std::string &fileName)
        : _file(fileName) {}
    
    bool readLine(std::string &where) {
        if (!_file.is_open() || !_file.good())
            return false;
        std::getline(_file, where);
        return true;
    }
};

class Generator
{
public:
    virtual void generate(std::stringstream &output) = 0;

    virtual ~Generator() {}
};

class ConstGenerator : public Generator
{
private:
    std::string _value;
public:
    ConstGenerator(const std::string &value)
        : _value(value) {}

    void generate(std::stringstream &output)
    {
        output << _value;
    }
};

template<typename RandNumGenerator>
class CharAlternativeGenerator : public Generator
{
private:
    std::string _possibleChars;
    RandNumGenerator _randNumGenerator;
public:
    CharAlternativeGenerator(const std::string &possibleChars)
        : _possibleChars(possibleChars) {}

    void generate(std::stringstream &output)
    {
        output << _possibleChars[_randNumGenerator.get() % _possibleChars.size()];
    }
};

class SeriesOfGeneratorsGenerator : public Generator
{
private:
    std::vector<std::unique_ptr<Generator>> _generators;
public:
    void swapContents(std::vector<std::unique_ptr<Generator>> &generators)
    {
        _generators.swap(generators);
    }

    void generate(std::stringstream &output)
    {
        for (auto &generator : _generators) {
            generator->generate(output);
        }
    }
};

class PlainRandomNumberGenerator
{
private:
    int _current;
public:
    int get()
    {
        return _current++;
    }
};

const int EOL = -1;

template<typename FileReader = PlainFileReader,
         typename RandNumGenerator = PlainRandomNumberGenerator>
class ConfigFile
{
public:

    typedef CharAlternativeGenerator<RandNumGenerator> CharAlternativeGenerator_;

    bool parse(std::string fileName)
    {
        FileReader file(fileName);
        return parse(file);
    }

    bool parse(FileReader &file)
    {
        int lineNum = 0;

        std::string line;
        while (file.readLine(line)) {
            lineNum++;
            std::string errMsg;
            if (! parseLine(line, errMsg)) {
                std::cerr << lineNum << ": " << errMsg << std::endl;
                return false;
            }
        }
        return true;
    }
    
    const std::vector<std::pair<std::string, std::string>> & getLines()
    {
        return _lines;
    }

    std::unique_ptr<Generator> parseRegex(const std::string &regex)
    {
        enum State {
            CLEAR,
            CHAR_ALTERNATIVE, // [abc]
            //SUB_REGEX_ALTERNATIVE, // (abc|def|geh)
        };

        State state = CLEAR;
        //bool backslash = false;
        std::stringstream stream;

        std::stack<State> stateStack;

        std::function<void(State)> setState = [&stateStack, &state](State s)
        {
            stateStack.push(state);
            state = s;
        };

        std::function<void()> restoreState = [&stateStack, &state]()
        {
            state = stateStack.top();
            stateStack.pop();
        };

        std::vector<std::unique_ptr<Generator>> generators;

        std::function<void(int)> processChar = [&](int character)
        {
            switch (state) {
                case CLEAR:
                    switch (character) {
                        case '[':
                            setState(CHAR_ALTERNATIVE);
                        case EOL:

                        if (stream.str().size() > 0) {
                            generators.push_back(std::unique_ptr<Generator>(new ConstGenerator(stream.str())));
                            stream.str("");
                        }
                        break;

                        default:
                            stream << static_cast<char>(character);
                    }
                    break;
                case CHAR_ALTERNATIVE:
                    switch (character) {
                        case ']':
                            restoreState();
                        case EOL:
                        
                        generators.push_back(std::unique_ptr<Generator>(new CharAlternativeGenerator_(stream.str())));
                        stream.str("");
                        break;

                        default:
                            stream << static_cast<char>(character);    
                    }
            }
        };

        std::for_each(regex.begin(), regex.end(), processChar);
        processChar(EOL);

        if (generators.size() == 1) {
            return std::move(generators.front());
        }

        auto result = std::unique_ptr<SeriesOfGeneratorsGenerator>(new SeriesOfGeneratorsGenerator);
        result->swapContents(generators);
        return std::move(result);
    }

private:

    std::vector<std::pair<std::string, std::string>> _lines;

    std::map<std::string, std::unique_ptr<Generator>> _variables;

    bool parseLine(const std::string &line, std::string &errMsg)
    {
        enum State {
            CLEAR,
            READING_NAME,
            WHITESPACE_AFTER_NAME,
            WHITESPACE_BEFORE_VAL,
            READING_VAL,
        };

        State state = CLEAR;

        std::stringstream nameStream, valueStream;

        for (std::string::const_iterator it = line.begin(); it != line.end(); ++it) {
            switch (state) {
                case CLEAR:
                    switch (*it) {
                        case ' ': break;
                        case '#': return true; // comment
                        default:
                            state = READING_NAME;
                    }
                    break;
                case READING_NAME:
                    switch (*it) {
                        case ' ':
                            state = WHITESPACE_AFTER_NAME;
                            break;
                        case '=':
                            state = WHITESPACE_BEFORE_VAL;
                            break;
                        default:
                            nameStream << *it;
                    }
                    break;
               case WHITESPACE_AFTER_NAME:
                    switch (*it) {
                        case ' ': break;
                        case '=':
                            state = WHITESPACE_BEFORE_VAL;
                            break;
                        default:
                            errMsg = "Unexpected chars after variable name";
                            return false;
                    }
                    break;
               case WHITESPACE_BEFORE_VAL:
                    switch (*it) {
                        case ' ': break;
                        default:
                            state = READING_VAL;
                            valueStream << *it;
                    }
                    break;
               case READING_VAL:
                    valueStream << *it;
            }
        }

        if (state < READING_VAL) {
            errMsg = "Finished parsing line in an unexpected state";
            return false;
        }

        std::string name, value;
        nameStream >> name;
        valueStream >> value;

        _lines.push_back(std::make_pair(name, value));

        auto generator = parseRegex(value);

        return true;
    }
};


}

