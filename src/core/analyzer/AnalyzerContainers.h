/*
 * AnalyzerContainers.h
 *
 *  Created on: Aug 21, 2013
 *      Author: sbisht
 */

#ifndef __CORE_ANALYZER_ANALYZERCONTAINERS_H__
#define __CORE_ANALYZER_ANALYZERCONTAINERS_H__

#include <instantsearch/Analyzer.h>
#include "SynonymFilter.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <string>
#include <map>
#include <set>
#include <fstream>

namespace srch2 {
namespace instantsearch{

// helper class for analyzer file based resources
class AnalyzerContainer {

 public:
    virtual void init(const std::string &filePath) = 0;
    virtual ~AnalyzerContainer();

    void free(const string &path);
    static void freeAll();

 protected:

    // generic map that can hold any path based analyzer resource
    // path must be unique
    typedef map <const string, AnalyzerContainer *> Map_t;
    static Map_t containers;
};

class SynonymContainer : public AnalyzerContainer {
public:
	void init(const std::string &filePath);
	void loadSynonymContainer(boost::archive::binary_iarchive& ia);
	void saveSynonymContainer(boost::archive::binary_oarchive& oa);
	bool contains(const std::string& str);
	void getValue(const std::string& str, std::pair<SynonymTokenType, std::string>& returnValue);
	// this is thread unsafe. Make sure you call it from main thread only.
	static SynonymContainer& getInstance(const std::string &filePath);

private:
	std::map<std::string, std::pair<SynonymTokenType, std::string> > synonymMap;
	const std::string synonymDelimiter;
	SynonymContainer(const std::string &delimiter):synonymDelimiter(delimiter) {}
	SynonymContainer(const SynonymContainer&) {}
	void operator == (const SynonymContainer&){}
};

class StemmerContainer : public AnalyzerContainer {
public:
	void init(const std::string &filePath);
	void loadStemmerContainer(boost::archive::binary_iarchive& ia);
	void saveStemmerContainer(boost::archive::binary_oarchive& oa);
	bool contains(const std::string& str);
	// this is thread unsafe. Make sure you call it from main thread only.
	static StemmerContainer& getInstance(const std::string &filePath);

private:
	std::map<std::string, int> dictionaryWords;
	StemmerContainer() {}
	StemmerContainer(const StemmerContainer&) {}
	void operator == (const StemmerContainer&){}
};

class StopWordContainer : public AnalyzerContainer {
public:
	void init(const std::string &filePath);
	void loadStopWordContainer(boost::archive::binary_iarchive& ia);
	void saveStopWordContainer(boost::archive::binary_oarchive& oa);
	bool contains(const std::string& str);
	// this is thread unsafe. Make sure you call it from main thread only.
	static StopWordContainer& getInstance(const std::string &filePath);

private:
	std::set<std::string> stopWordsSet;
	StopWordContainer() {}
	StopWordContainer(const StopWordContainer&) {}
	void operator == (const StopWordContainer&){}
};

/*
 *   This class is container which stores the protected words required by the protectedWordFilter.
 */
class ProtectedWordsContainer : public AnalyzerContainer {
public:
	void init(const std::string &filePath);
	bool isProtected(const string& val);
	static ProtectedWordsContainer& getInstance(const std::string &filePath);

private:
	set<string> protectedWords;
	ProtectedWordsContainer() {}
	ProtectedWordsContainer(const ProtectedWordsContainer&) {}
	void operator == (const ProtectedWordsContainer&){}
};

} // instantsearch
} // namespace srch2
#endif /* __CORE_ANALYZER_ANALYZERCONTAINERS_H__ */
