/* License: GPL v2 */
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
    int _current = 0;
public:
    int get()
    {
        return _current++;
    }
};

TEST(ConfigFile, TestRegexConst)
{
    std::string regex = "abcdef";
    std::unique_ptr<Randodo::Generator> gen = Randodo::RegexParser<FakeFileReader, FakeRandomNumberGenerator>::parseExpression(regex);
    std::stringstream stream;

    gen->generate(stream);

    ASSERT_EQ("abcdef", stream.str());
}

TEST(ConfigFile, TestRegexTwoCharAlternatives)
{
    Randodo::ConfigFile<FakeFileReader, FakeRandomNumberGenerator> configFile;
    std::string regex = "abc[def][ghi]";
    std::unique_ptr<Randodo::Generator> gen = Randodo::RegexParser<FakeFileReader, FakeRandomNumberGenerator>::parseExpression(regex);
    std::stringstream str1, str2;

    gen->generate(str1);
    gen->generate(str2);

    ASSERT_EQ("abcdg", str1.str());
    ASSERT_EQ("abceh", str2.str());
}

TEST(ConfigFile, TestRegexTwoCharAlternativeRange)
{
    Randodo::ConfigFile<FakeFileReader, FakeRandomNumberGenerator> configFile;
    std::string regex = "abc[a-c][c-d]";
    std::unique_ptr<Randodo::Generator> gen = Randodo::RegexParser<FakeFileReader, FakeRandomNumberGenerator>::parseExpression(regex);
    std::stringstream str1, str2, str3;

    gen->generate(str1);
    gen->generate(str2);
    gen->generate(str3);

    ASSERT_EQ("abcac", str1.str());
    ASSERT_EQ("abcbd", str2.str());
    ASSERT_EQ("abccc", str3.str());
}


TEST(ConfigFile, TestRegexOneConstAlternative)
{
    Randodo::ConfigFile<FakeFileReader, FakeRandomNumberGenerator> configFile;
    std::string regex = "abc|def";
    std::unique_ptr<Randodo::Generator> gen = Randodo::RegexParser<FakeFileReader, FakeRandomNumberGenerator>::parseExpression(regex);
    std::stringstream str1, str2;

    gen->generate(str1);
    gen->generate(str2);

    ASSERT_EQ("abc", str1.str());
    ASSERT_EQ("def", str2.str());
}


TEST(ConfigFile, TestRegexEmbeddedConstAlternative)
{
    Randodo::ConfigFile<FakeFileReader, FakeRandomNumberGenerator> configFile;
    std::string regex = "abc(def|ghi)jkl";
    std::unique_ptr<Randodo::Generator> gen = Randodo::RegexParser<FakeFileReader, FakeRandomNumberGenerator>::parseExpression(regex);
    std::stringstream str1, str2;

    gen->generate(str1);
    gen->generate(str2);

    ASSERT_EQ("abcdefjkl", str1.str());
    ASSERT_EQ("abcghijkl", str2.str());
}


TEST(ConfigFile, TestRegexRepetitionsVaried)
{
    Randodo::ConfigFile<FakeFileReader, FakeRandomNumberGenerator> configFile;
    std::string regex = "abc{3,5}";
    std::unique_ptr<Randodo::Generator> gen = Randodo::RegexParser<FakeFileReader, FakeRandomNumberGenerator>::parseExpression(regex);
    std::stringstream str1, str2;

    gen->generate(str1);
    gen->generate(str2);

    ASSERT_EQ("abcabcabc", str1.str());
    ASSERT_EQ("abcabcabcabc", str2.str());
}

TEST(ConfigFile, TestRegexBackslash)
{
    Randodo::ConfigFile<FakeFileReader, FakeRandomNumberGenerator> configFile;
    std::string regex = "a\\{b\\}[\\[\\]]";
    std::unique_ptr<Randodo::Generator> gen = Randodo::RegexParser<FakeFileReader, FakeRandomNumberGenerator>::parseExpression(regex);
    std::stringstream str1, str2;

    gen->generate(str1);
    gen->generate(str2);

    ASSERT_EQ("a{b}[", str1.str());
    ASSERT_EQ("a{b}]", str2.str());
}


TEST(ConfigFile, TestRegexRepetitionsConst)
{
    Randodo::ConfigFile<FakeFileReader, FakeRandomNumberGenerator> configFile;
    std::string regex = "abc{3}";
    std::unique_ptr<Randodo::Generator> gen = Randodo::RegexParser<FakeFileReader, FakeRandomNumberGenerator>::parseExpression(regex);
    std::stringstream str1, str2;

    gen->generate(str1);
    gen->generate(str2);

    ASSERT_EQ("abcabcabc", str1.str());
    ASSERT_EQ("abcabcabc", str2.str());
}


TEST(ConfigFile, TestRegexRepetitionsFrom0)
{
    Randodo::ConfigFile<FakeFileReader, FakeRandomNumberGenerator> configFile;
    std::string regex = "abc{,3}";
    std::unique_ptr<Randodo::Generator> gen = Randodo::RegexParser<FakeFileReader, FakeRandomNumberGenerator>::parseExpression(regex);
    std::stringstream str1, str2;

    gen->generate(str1);
    gen->generate(str2);

    ASSERT_EQ("", str1.str());
    ASSERT_EQ("abc", str2.str());
}


TEST(ConfigFile, TestRegexAlternative)
{
    Randodo::ConfigFile<FakeFileReader, FakeRandomNumberGenerator> configFile;
    std::string regex = "abc(def|[ghi])jkl";
    std::unique_ptr<Randodo::Generator> gen = Randodo::RegexParser<FakeFileReader, FakeRandomNumberGenerator>::parseExpression(regex);
    std::stringstream str1, str2, str3, str4;

    gen->generate(str1);
    gen->generate(str2);
    gen->generate(str3);
    gen->generate(str4);

    ASSERT_EQ("abcdefjkl", str1.str());
    ASSERT_EQ("abcgjkl", str2.str());
    ASSERT_EQ("abcdefjkl", str3.str());
    ASSERT_EQ("abchjkl", str4.str());
}

TEST(ConfigFile, TestConstant)
{
    Randodo::ConfigFile<FakeFileReader> configFile;
    FakeFileReader fakeFileReader;
    fakeFileReader.addLine("# some comment");
    ASSERT_TRUE(configFile.parse(fakeFileReader));
    ASSERT_EQ(0U, configFile.getLines().size());
}

TEST(ConfigFile, TestVariable)
{
    Randodo::ConfigFile<FakeFileReader, FakeRandomNumberGenerator> configFile;
    FakeFileReader fakeFileReader;
    fakeFileReader.addLine("gnome=(dwarf|lilliput)");
    fakeFileReader.addLine("hobbit=$gnome [goblin]");
    ASSERT_TRUE(configFile.parse(fakeFileReader));

    auto &mapOfGenerators = configFile.getMapOfGenerators();
    auto iter = mapOfGenerators.find("hobbit");

    ASSERT_NE(mapOfGenerators.end(), iter);

    std::stringstream str1, str2;

    iter->second->generate(str1);
    iter->second->generate(str2);

    ASSERT_EQ("dwarf g", str1.str());
    ASSERT_EQ("lilliput o", str2.str());
}
