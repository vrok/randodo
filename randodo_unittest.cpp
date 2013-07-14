/* License: GPL v3 */
/* Contact author: wrochniak@gmail.com */

#include "gtest/gtest.h"
#include "randodo.h"

class FakeFileReader
{
private:
    std::vector<std::string> _contents;
    std::vector<std::string>::iterator _it;
    bool _readingStarted;

public:
    FakeFileReader()
        : _readingStarted(false) {}

    void addLine(const std::string &line)
    {
        _contents.push_back(line);
    }

    bool readLine(std::string &where)
    {
        if (!_readingStarted) {
            _readingStarted = true;
            _it = _contents.begin();
        }

        if (_it == _contents.end())
            return false;
        where = *(_it++);
        return true;
    }
};

class FakeRandomNumberGenerator
{
private:
    int _current;
public:
    int get()
    {
        return _current++;
    }
};

TEST(ConfigFile, TestBasicFileParsing)
{
    Randodo::ConfigFile<FakeFileReader> configFile;
    std::string regex = "abcdef";
    std::unique_ptr<Randodo::Generator> gen = configFile.parseRegex(regex);
    std::stringstream stream;

    gen->generate(stream);

    ASSERT_EQ("abcdef", stream.str());
}

TEST(ConfigFile, TestBasicFileParsing2)
{
    Randodo::ConfigFile<FakeFileReader> configFile;
    std::string regex = "abc[def][ghi]";
    std::unique_ptr<Randodo::Generator> gen = configFile.parseRegex(regex);
    std::stringstream str1, str2;

    gen->generate(str1);
    gen->generate(str2);

    ASSERT_EQ("abcdg", str1.str());
    ASSERT_EQ("abceh", str2.str());
}

TEST(ConfigFile, TestConstant)
{
    Randodo::ConfigFile<FakeFileReader> configFile;
    FakeFileReader fakeFileReader;
    fakeFileReader.addLine("# some comment");
    ASSERT_TRUE(configFile.parse(fakeFileReader));
    ASSERT_EQ(0U, configFile.getLines().size());
}

