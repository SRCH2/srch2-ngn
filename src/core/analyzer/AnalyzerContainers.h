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
#include <vector>
#include <fstream>



namespace srch2 {
namespace instantsearch{

class SynonymContainer {
public:
	void initSynonymContainer(const std::string filePath);
	void loadSynonymContainer(boost::archive::binary_iarchive& ia);
	void saveSynonymContainer(boost::archive::binary_oarchive& oa);
	bool contains(const std::string& str);
	void getValue(const std::string& str, std::pair<SynonymTokenType, std::string>& returnValue);
	// this is thread unsafe. Make sure you call it from main thread only.
	static SynonymContainer& getInstance();
private:
	static SynonymContainer *synonymContainer;
	/*
	 * If we have folllwing synonym rules
	 * s1: new york = ny
	 * s2: new york city = nyc
	 * s3: bill = william
	 *
	 * The map elements will be as following:
	 *
	 * new => <SYNONYM_PREFIX_ONLY, "" >
	 * new york => <SYNONYM_PREFIX_AND_COMPLETE, 'ny'>
	 * new york city => <SYNONYM_COMPLETE_ONLY, 'nyc'>
	 * bill => <SYNONYM_COMPLETE_ONLY, 'william'>
	 * orange: Nothing will be in the map for it.
	 */
	std::map<std::string, std::pair<SynonymTokenType, std::string> > synonymMap;
	const std::string synonymDelimiter;
	SynonymContainer(const std::string &delimiter):synonymDelimiter(delimiter) {}
	SynonymContainer(const SynonymContainer&) {}
	void operator == (const SynonymContainer&){}
};

class StemmerContainer {
public:
	void initStemmerContainer(const std::string filePath);
	void loadStemmerContainer(boost::archive::binary_iarchive& ia);
	void saveStemmerContainer(boost::archive::binary_oarchive& oa);
	bool contains(const std::string& str);
	// this is thread unsafe. Make sure you call it from main thread only.
	static StemmerContainer& getInstance();
private:
	static StemmerContainer *stemmerContainer;
	std::map<std::string, int> dictionaryWords;
	StemmerContainer() {}
	StemmerContainer(const StemmerContainer&) {}
	void operator == (const StemmerContainer&){}
};

class StopWordContainer {
public:
	void initStopWordContainer(const std::string filePath);
	void loadStopWordContainer(boost::archive::binary_iarchive& ia);
	void saveStopWordContainer(boost::archive::binary_oarchive& oa);
	bool contains(const std::string& str);
	// this is thread unsafe. Make sure you call it from main thread only.
	static StopWordContainer& getInstance();
private:
	static StopWordContainer *stopWordContainer;
	std::vector<std::string> stopWordsVector;
	StopWordContainer() {}
	StopWordContainer(const StopWordContainer&) {}
	void operator == (const StopWordContainer&){}
};

} // instantsearch
} // namespace srch2
#endif /* __CORE_ANALYZER_ANALYZERCONTAINERS_H__ */
