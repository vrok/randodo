/* License: GPL v2 */
/* Contact author: wrochniak@gmail.com */

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <memory>
#include <stack>
#include <algorithm>
#include <cassert>

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
        if (!_file.is_open() || !_file.good()) {
            return false;
        }
        std::getline(_file, where);
        return true;
    }
};

class Generator
{
public:
    virtual void generate(std::stringstream &output) = 0;

    virtual bool isEmpty() = 0;

    virtual void optimize() = 0;

    virtual ~Generator() {}
};

typedef std::map<std::string, std::unique_ptr<Generator>> MapOfGenerators;

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

    bool isEmpty()
    {
        return _value.size() == 0;
    }

    void optimize() {}
};

template<typename RandNumGenerator>
class CharAlternativeGenerator : public Generator
{
private:
    std::string _possibleChars;
    RandNumGenerator _randNumGenerator;
public:
    CharAlternativeGenerator(const std::string &possibleChars) : _possibleChars(possibleChars) {}

    void generate(std::stringstream &output)
    {
        output << _possibleChars[_randNumGenerator.get() % _possibleChars.size()];
    }

    bool isEmpty()
    {
        return _possibleChars.size() == 0;
    }

    void optimize() {}
};

class VariableGenerator : public Generator
{
private:
    std::string _varName;
    const MapOfGenerators &_mapOfGenerators;
public:
    VariableGenerator(std::string &&varName, const MapOfGenerators &mapOfGenerators)
        : _varName(std::move(varName)), _mapOfGenerators(mapOfGenerators) {}

    void generate(std::stringstream &output)
    {
        auto &&it = _mapOfGenerators.find(_varName);
        if (it != _mapOfGenerators.end()) {
            it->second->generate(output);
        }
    }

    bool isEmpty()
    {
        // TODO
        return false;
    }

    void optimize()
    {
        // TODO: inline referenced generator
    }
};

template<typename RandNumGenerator>
class RepetitionsGenerator : public Generator
{
private:
    const int _from, _to;
    std::unique_ptr<Generator> _generator;
    RandNumGenerator _randNumGenerator;
public:
    RepetitionsGenerator(int from, int to, std::unique_ptr<Generator> &&generator)
        : _from(from), _to(to), _generator(std::move(generator)) {}
    
    void generate(std::stringstream &output)
    {
        int howMany = _from + (_randNumGenerator.get() % (_to - _from + 1));
        for (int i = 0; i < howMany; i++) {
            _generator->generate(output);
        }
    }

    bool isEmpty()
    {
        return _from == 0 && _to == 0;
    }

    void optimize()
    {
        // TODO
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

    bool isEmpty()
    {
        return _generators.size() == 0;
    }

    void optimize()
    {
        for (auto &gen : _generators) {
            gen->optimize();
        }

        auto emptyBegin = std::stable_partition(_generators.begin(), _generators.end(),
                                                [](std::unique_ptr<Generator> &gen) {
            return !gen->isEmpty();
        });

        _generators.erase(emptyBegin, _generators.end()); 
    }
};

template<typename RandNumGenerator>
class AlternativeOfGeneratorsGenerator : public Generator
{
private:
    std::vector<std::unique_ptr<Generator>> _generators;
    RandNumGenerator _randNumGenerator;
public:
    void swapContents(std::vector<std::unique_ptr<Generator>> &generators)
    {
        _generators.swap(generators);
    }

    void generate(std::stringstream &output)
    {
        _generators[_randNumGenerator.get() % _generators.size()]->generate(output);
    }

    bool isEmpty()
    {
        return _generators.size() == 0;
    }

    void optimize()
    {
        // TODO
    }
};

class PlainRandomNumberGenerator
{
public:
    int get()
    {
        return rand();
    }
};

const int EOL = -1;

template<typename FileReader = PlainFileReader,
         typename RandNumGenerator = PlainRandomNumberGenerator>
class ConfigFile
{
public:

    typedef CharAlternativeGenerator<RandNumGenerator> CharAlternativeGenerator_;
    typedef AlternativeOfGeneratorsGenerator<RandNumGenerator> AlternativeOfGeneratorsGenerator_;
    typedef RepetitionsGenerator<RandNumGenerator> RepetitionsGenerator_;

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
        RegexParser regexParser;
        return regexParser.parseRegex(regex, _generatorsMap);
    }

    const MapOfGenerators &getMapOfGenerators() const
    {
        return _generatorsMap;
    }

private:

    std::vector<std::pair<std::string, std::string>> _lines;

    MapOfGenerators _generatorsMap;

    static bool isDigit(int c)
    {
        return c >= '0' && c <= '9';
    }

    static bool isAlpha(int c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || isDigit(c);
    }

    class RegexParser {

        enum State {
            DEFAULT,
            CHAR_ALTERNATIVE, // [abc]
            VARIABLE_NAME, // $foo
            REPETITIONS_SPECS, // {1,10} or {10}, or {,10}, etc.
            BACKSLASH, // for special characters
        };

        std::stack<State> _stateStack;
        State _state = DEFAULT;
        std::vector<std::vector<std::unique_ptr<Generator>>> _generators;

        std::stringstream _stream;
        std::vector<int> _repetitions;
        bool _wasDashInCharAlternative = false;

        template<typename GeneratorType, typename... Rest>
        void pushGenerator(std::stringstream &stream, Rest... otherArgs)
        {
            if (stream.str().size() > 0) {
                _generators.back().push_back(std::unique_ptr<GeneratorType>
                        (new GeneratorType(stream.str(), otherArgs...)));
                stream.str("");
            }
        }

        void setState(State s)
        {
            _stateStack.push(_state);
            _state = s;
        }

        void restoreState()
        {
            _state = _stateStack.top();
            _stateStack.pop();
        }

        void processCharInDefaultState(int character)
        {
            switch (character) {
                case '\\':
                    setState(BACKSLASH);
                    break;
                case '$':
                    pushGenerator<ConstGenerator>(_stream);
                    setState(VARIABLE_NAME);
                    break;
                case '(':
                    pushGenerator<ConstGenerator>(_stream);
                    setState(DEFAULT);
                    _generators.push_back(std::vector<std::unique_ptr<Generator>>());
                    _generators.push_back(std::vector<std::unique_ptr<Generator>>());
                    break;
                case ')':
                    pushGenerator<ConstGenerator>(_stream);
                    assert(_generators.size() >= 3);

                    {
                        auto seriesGen = std::unique_ptr<SeriesOfGeneratorsGenerator>
                            (new SeriesOfGeneratorsGenerator());
                        seriesGen->swapContents(_generators.back());
                        _generators.pop_back();
                        _generators.back().push_back(std::move(seriesGen));
                    }

                    {
                        auto altGen = std::unique_ptr<AlternativeOfGeneratorsGenerator_>
                            (new AlternativeOfGeneratorsGenerator_());
                        altGen->swapContents(_generators.back());
                        _generators.pop_back();
                        _generators.back().push_back(std::move(altGen));
                    }
                    restoreState();
                    break;
                case '{':
                    pushGenerator<ConstGenerator>(_stream);
                    setState(REPETITIONS_SPECS);
                    break;
                case '[':
                    pushGenerator<ConstGenerator>(_stream);
                    setState(CHAR_ALTERNATIVE);
                    break;
                case '|':
                    pushGenerator<ConstGenerator>(_stream);

                    {
                        auto seriesGen = std::unique_ptr<SeriesOfGeneratorsGenerator>
                            (new SeriesOfGeneratorsGenerator());
                        seriesGen->swapContents(_generators.back());
                        assert(_generators.size() >= 2);
                        (_generators.end() - 2)->push_back(std::move(seriesGen));
                    }

                    break;

                case EOL:
                    pushGenerator<ConstGenerator>(_stream);
                    
                    assert(_generators.size() == 2);

                    {
                        auto seriesGen = std::unique_ptr<SeriesOfGeneratorsGenerator>
                            (new SeriesOfGeneratorsGenerator());
                        seriesGen->swapContents(_generators.back());
                        _generators.pop_back();
                        _generators.back().push_back(std::move(seriesGen));
                    }

                    {
                        auto altGen = std::unique_ptr<AlternativeOfGeneratorsGenerator_>
                            (new AlternativeOfGeneratorsGenerator_());
                        altGen->swapContents(_generators.back());
                        _generators.back().push_back(std::move(altGen));
                    }

                    break;

                default:
                    _stream << static_cast<char>(character);
            }
        }

        void processCharInRepetitionsSpecsState(int character)
        {
            if (isDigit(character)) {
                _stream << static_cast<char>(character);
            } else {
                assert(character == ',' || character == '}');

                int val = atoi(_stream.str().c_str());
                _stream.str("");
                _repetitions.push_back(val);

                if (character == '}') {

                    if (_repetitions.size() == 1) {
                        _repetitions.push_back(_repetitions.front());
                    }

                    assert(_generators.back().size() > 0);

                    auto prevGenerator = std::move(_generators.back().back());
                    _generators.back().pop_back();
                    _generators.back().push_back(std::unique_ptr<RepetitionsGenerator_>
                            (new RepetitionsGenerator_(_repetitions[0], _repetitions[1],
                                                       std::move(prevGenerator))));

                    restoreState();
                }
            }
        }

        void processCharInCharAlternativeState(int character)
        {
            switch (character) {
                case '\\':
                    setState(BACKSLASH);
                    break;
                case '-':
                    _wasDashInCharAlternative = true;
                    break;
                case ']':
                    restoreState();
                case EOL:
                    pushGenerator<CharAlternativeGenerator_>(_stream);
                    break;
                default:
                    if (_wasDashInCharAlternative) {
                        _wasDashInCharAlternative = false;
                        char from = _stream.str().back();
                        if (from >= character) {
                            // TODO: maybe it'd be better to throw an error than silently ignore
                            break;
                        }
                        for (char ch = from + 1; ch <= character; ch++) {
                            _stream << static_cast<char>(ch);
                        }
                    } else {
                        _stream << static_cast<char>(character);    
                    }
            }
        }

        bool processCharInVariableNameStateAndTellIfShouldReturn(int character, const MapOfGenerators &mapOfGenerators)
        {
            if (isAlpha(character)) {
                _stream << static_cast<char>(character);
            } else {
                pushGenerator<VariableGenerator>(_stream, std::cref(mapOfGenerators));
                restoreState();
                return true;
            }
            return false;
        }

        bool processCharAndTellIfShouldRerun(int character, const MapOfGenerators &mapOfGenerators)
        {
            switch (_state) {
                case DEFAULT:
                    processCharInDefaultState(character);
                    break;
                case REPETITIONS_SPECS:
                    processCharInRepetitionsSpecsState(character);
                    break;
                case VARIABLE_NAME:
                    if (processCharInVariableNameStateAndTellIfShouldReturn(character, mapOfGenerators))
                        return true;
                    break;
                case CHAR_ALTERNATIVE:
                    processCharInCharAlternativeState(character);
                    break;
                case BACKSLASH:
                    _stream << static_cast<char>(character);
                    restoreState();
                    break;
            }

            return false;
        }

    public:

        RegexParser() : _generators(2) {}

        std::unique_ptr<Generator> parseRegex(const std::string &regex, const MapOfGenerators &mapOfGenerators)
        {
            std::function<void(int)> processChar = [&, this](int character)
            {
                while (processCharAndTellIfShouldRerun(character, mapOfGenerators));
            };

            std::for_each(regex.begin(), regex.end(), processChar);
            processChar(EOL);

            return std::move(_generators.back().back());
        }
    };

    bool parseLine(const std::string &line, std::string &errMsg)
    {
        enum State {
            DEFAULT,
            READING_NAME,
            WHITESPACE_AFTER_NAME,
            WHITESPACE_BEFORE_VAL,
            READING_VAL,
        };

        State state = DEFAULT;

        std::stringstream nameStream, valueStream;

        for (std::string::const_iterator it = line.begin(); it != line.end(); ++it) {
            switch (state) {
                case DEFAULT:
                    switch (*it) {
                        case ' ': break;
                        case '#': return true; // comment
                        default:
                            nameStream << *it;
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

        if (state == DEFAULT) {
            // blank line
            return true;
        }

        if (state < READING_VAL) {
            errMsg = "Finished parsing line in an unexpected state";
            return false;
        }

        std::string &&name = nameStream.str(), &&value = valueStream.str();
        _lines.push_back(std::make_pair(name, value));
        _generatorsMap.insert(std::make_pair(name, parseRegex(value)));

        return true;
    }
};


}

