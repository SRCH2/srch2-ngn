/*
 * AnalyzerFactory.cpp
 *
 *  Created on: Aug 21, 2013
 *      Author: sbisht
 */

#include "AnalyzerFactory.h"
#include "ConfigManager.h"
#include <string>
#include <instantsearch/Analyzer.h>
#include <analyzer/AnalyzerContainers.h>
#include <index/IndexUtil.h>

#include <boost/thread/tss.hpp>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

Analyzer* AnalyzerFactory::createAnalyzer(const CoreInfo_t* config) {

	// This flag shows if we need to stem or not. (StemmerNormalizerType is an enum)
	StemmerNormalizerFlagType stemmerFlag;
	// gets the stem flag and set the stemType
	if (config->getStemmerFlag()) {
		stemmerFlag = srch2is::ENABLE_STEMMER_NORMALIZER;
	} else {
		stemmerFlag = srch2is::DISABLE_STEMMER_NORMALIZER;
	}
	// This flag shows if we need to keep the origin word or not.
	SynonymKeepOriginFlag synonymKeepOriginFlag;
	// gets the stem flag and set the stemType
	if (config->getSynonymKeepOrigFlag()) {
		synonymKeepOriginFlag = srch2is::SYNONYM_KEEP_ORIGIN;
	} else {
		synonymKeepOriginFlag = srch2is::SYNONYM_DONOT_KEEP_ORIGIN;
	}

	// append the stemmer file to the install directory
	std::string stemmerFilterFilePath = config->getStemmerFile();
	// gets the path of stopFilter
	std::string stopFilterFilePath = config->getStopFilePath();
	// gets the path of stopFilter
	std::string  synonymFilterFilePath = config->getSynonymFilePath();

	// Create an analyzer
	return new Analyzer(stemmerFlag,
			stemmerFilterFilePath,
			stopFilterFilePath,
			synonymFilterFilePath,
			synonymKeepOriginFlag,
			config->getRecordAllowedSpecialCharacters());
}

Analyzer* AnalyzerFactory::getCurrentThreadAnalyzer(const CoreInfo_t* config) {

	static boost::thread_specific_ptr<Analyzer> _ts_analyzer_object;
	if (_ts_analyzer_object.get() == NULL)
	{
		Logger::debug("Create Analyzer object for thread = %d ",  pthread_self());
		_ts_analyzer_object.reset(AnalyzerFactory::createAnalyzer(config));
	}

	Analyzer* analyzer = _ts_analyzer_object.get();

	// clear the initial states of the filters in the analyzer, e.g.,
	// for those filters that have an internal buffer to keep tokens.
	// Such an internal buffer can have leftover tokens from
	// the previous query (possibly an invalid query)
	analyzer->clearFilterStates();

	return analyzer;
}

void AnalyzerHelper::initializeAnalyzerResource (const CoreInfo_t* conf) {
	if (conf->getProtectedWordsFilePath().compare("") != 0) {
		ProtectedWordsContainer::getInstance().initProtectedWordsContainer(conf->getProtectedWordsFilePath());
	}
	if (conf->getSynonymFilePath().compare("") != 0) {
		SynonymContainer::getInstance().initSynonymContainer(conf->getSynonymFilePath());
	}
	if (conf->getStemmerFile().compare("") != 0) {
		StemmerContainer::getInstance().initStemmerContainer(conf->getStemmerFile());
	}
	if (conf->getStopFilePath().compare("") != 0) {
		StopWordContainer::getInstance().initStopWordContainer(conf->getStopFilePath());
	}
}

void AnalyzerHelper::loadAnalyzerResource(const CoreInfo_t* conf) {
	try{
		const std::string& directoryName = conf->getIndexPath();
		std::ifstream ifs((directoryName + "/" + string(IndexConfig::analyzerFileName)).c_str(), std::ios::binary);
		if (ifs.good())
		{
			boost::archive::binary_iarchive ia(ifs);
			SynonymContainer::getInstance().loadSynonymContainer(ia);
			StemmerContainer::getInstance().loadStemmerContainer(ia);
			StopWordContainer::getInstance().loadStopWordContainer(ia);
			ifs.close();
		}else {
			ifs.close();
			initializeAnalyzerResource(conf);
			saveAnalyzerResource(conf);
		}
		ProtectedWordsContainer::getInstance().initProtectedWordsContainer(conf->getProtectedWordsFilePath());
	}catch (std::exception& ex){
		Logger::error("Error while loading Analyzer resource files");
		Logger::error(ex.what());
	}
}

void AnalyzerHelper::saveAnalyzerResource(const CoreInfo_t* conf) {
	try{
		const std::string& directoryName = conf->getIndexPath();
		std::ofstream ofs((directoryName + "/" + string(IndexConfig::analyzerFileName)).c_str(), std::ios::binary);
		if (ofs.good()) {
			boost::archive::binary_oarchive oa(ofs);
			SynonymContainer::getInstance().saveSynonymContainer(oa);
			StemmerContainer::getInstance().saveStemmerContainer(oa);
			StopWordContainer::getInstance().saveStopWordContainer(oa);
		}
		ofs.close();
	}catch(std::exception& ex){
		Logger::error("Error while saving Analyzer resource");
		Logger::error(ex.what());
	}
}

} // namesoace wrapper
} // namespace srch2


