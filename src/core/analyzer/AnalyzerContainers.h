/*
 * AnalyzerContainers.h
 *
 *  Created on: Aug 21, 2013
 *      Author: sbisht
 */

#ifndef __CORE_ANALYZER_ANALYZERCONTAINERS_H__
#define __CORE_ANALYZER_ANALYZERCONTAINERS_H__

#include <instantsearch/Analyzer.h>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <boost/unordered_set.hpp>
#include "Dictionary.h"

namespace srch2 {
namespace instantsearch{

// helper class for analyzer file based resources
class AnalyzerContainer {

 public:

    virtual void init() = 0;
    virtual ~AnalyzerContainer();

    const string &getFilePath() const { return filePath; };

    static void free(const string &path);
    static void freeAll();
    void free() { AnalyzerContainer::free(filePath); }

 protected:

    // generic map that can hold any path based analyzer resource
    // path must be unique
    typedef map <const string, AnalyzerContainer *> Map_t;
    static Map_t containers;

    string filePath;
};
typedef vector<std::string> SynonymVector;
class SynonymContainer : public AnalyzerContainer {
public:
    void init();
    void loadSynonymContainer(boost::archive::binary_iarchive& ia);
    void saveSynonymContainer(boost::archive::binary_oarchive& oa);
    bool contains(const std::string& str) const;
    bool isPrefix(const std::string& str) const;
    bool getValue(const std::string& str, SynonymVector& returnValue) const;
    // this is thread unsafe. Make sure you call it from main thread only.
    static SynonymContainer *getInstance(const std::string &filePath,
                                             SynonymKeepOriginFlag synonymKeepOriginFlag);

     SynonymKeepOriginFlag keepOrigin() const { return synonymKeepOriginFlag; }

private:
    std::map<std::string, std::pair<bool, SynonymVector> > synonymMap;
    std::set<string>  prefixMap;
     const std::string synonymDelimiter;

     SynonymKeepOriginFlag synonymKeepOriginFlag;

     SynonymContainer(const std::string &delimiter) : synonymDelimiter(delimiter),synonymKeepOriginFlag(SYNONYM_KEEP_ORIGIN) {}
    SynonymContainer(const SynonymContainer&) {}
    SynonymContainer& operator = (const SynonymContainer&){ return *this;}
};

class StemmerContainer : public AnalyzerContainer {
public:
    void init();
    void loadStemmerContainer(boost::archive::binary_iarchive& ia);
    void saveStemmerContainer(boost::archive::binary_oarchive& oa);
    bool contains(const std::string& str) const;
    // this is thread unsafe. Make sure you call it from main thread only.
    static StemmerContainer *getInstance(const std::string &filePath);

private:
    std::map<std::string, int> dictionaryWords;
    StemmerContainer() {}
    StemmerContainer(const StemmerContainer&) {}
    StemmerContainer& operator = (const StemmerContainer&){ return *this;}
};

class StopWordContainer : public AnalyzerContainer {
public:
    void init();
    void loadStopWordContainer(boost::archive::binary_iarchive& ia);
    void saveStopWordContainer(boost::archive::binary_oarchive& oa);
    bool contains(const std::string& str) const;
    // this is thread unsafe. Make sure you call it from main thread only.
    static StopWordContainer *getInstance(const std::string &filePath);

private:
    std::set<std::string> stopWordsSet;
    StopWordContainer() {}
    StopWordContainer(const StopWordContainer&) {}
    StopWordContainer& operator = (const StopWordContainer&){ return *this;}
};

/*
 *   This class is container which stores the protected words required by the protectedWordFilter.
 */
class ProtectedWordsContainer : public AnalyzerContainer {
public:
    void init();
    bool isProtected(const string& val) const;
    static ProtectedWordsContainer *getInstance(const std::string &filePath);
    void loadProtectedWordsContainer(boost::archive::binary_iarchive& ia);
    void saveProtectedWordsContainer(boost::archive::binary_oarchive& oa);


private:
    set<string> protectedWords;
    ProtectedWordsContainer() {}
    ProtectedWordsContainer(const ProtectedWordsContainer&) {}
    ProtectedWordsContainer& operator = (const ProtectedWordsContainer&){return *this;}
};

class ChineseDictionaryContainer : public AnalyzerContainer {
public:
    static ChineseDictionaryContainer* getInstance(const std::string &filePath);
    void init();
    void loadDictionaryContainer(boost::archive::binary_iarchive &ia);
    void saveDictionaryContainer(boost::archive::binary_oarchive &oa);

    short getFreq(const std::vector<CharType> &buffer, unsigned istart, unsigned length) const;
    short getFreq(const std::string &str) const;
    int getMaxWordLength() const;
private:
    Dictionary chineseDictionary;
    ChineseDictionaryContainer():chineseDictionary() {}
    ChineseDictionaryContainer(const ChineseDictionaryContainer&):chineseDictionary() {}
    ChineseDictionaryContainer& operator = (const ChineseDictionaryContainer&){ return *this;}
};

} // instantsearch
} // namespace srch2
#endif /* __CORE_ANALYZER_ANALYZERCONTAINERS_H__ */
