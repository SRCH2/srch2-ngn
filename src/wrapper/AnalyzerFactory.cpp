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

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

Analyzer* AnalyzerFactory::createAnalyzer(const ConfigManager* configMgr) {

	// This flag shows if we need to stem or not. (StemmerNormalizerType is an enum)
	StemmerNormalizerFlagType stemmerFlag;
	// gets the stem flag and set the stemType
	if (configMgr->getStemmerFlag()) {
		stemmerFlag = srch2is::ENABLE_STEMMER_NORMALIZER;
	} else {
		stemmerFlag = srch2is::DISABLE_STEMMER_NORMALIZER;
	}
	// This flag shows if we need to keep the origin word or not.
	SynonymKeepOriginFlag synonymKeepOriginFlag;
	// gets the stem flag and set the stemType
	if (configMgr->getSynonymKeepOrigFlag()) {
		synonymKeepOriginFlag = srch2is::SYNONYM_KEEP_ORIGIN;
	} else {
		synonymKeepOriginFlag = srch2is::SYNONYM_DONOT_KEEP_ORIGIN;
	}

	// append the stemmer file to the install direcrtory
	std::string stemmerFilterFilePath = configMgr->getInstallDir() + configMgr->getStemmerFile();
	// gets the path of stopFilter
	std::string stopFilterFilePath = configMgr->getStopFilePath();
	// gets the path of stopFilter
	std::string  synonymFilterFilePath = configMgr->getSynonymFilePath();

	// Create an analyzer
	return new Analyzer(stemmerFlag,
			stemmerFilterFilePath,
			stopFilterFilePath,
			synonymFilterFilePath,
			synonymKeepOriginFlag,
			configMgr->getRecordAllowedSpecialCharacters());
}

void AnalyzerHelper::initializeAnalyzerResource(
		const ConfigManager* conf) {

	SynonymContainer::getInstance().initSynonymContainer(conf->getSynonymFilePath());
	StemmerContainer::getInstance().initStemmerContainer(conf->getStemmerFile());
	StopWordContainer::getInstance().initStopWordContainer(conf->getStopFilePath());

}

void AnalyzerHelper::loadAnalyzerResource(
		const ConfigManager* conf) {

	const std::string& directoryName = conf->getIndexPath();
	std::ifstream ifs((directoryName + "/" + string(IndexConfig::analyzerFileName)).c_str(), std::ios::binary);
	boost::archive::binary_iarchive ia(ifs);
	SynonymContainer::getInstance().loadSynonymContainer(ia);
	StemmerContainer::getInstance().loadStemmerContainer(ia);
	StopWordContainer::getInstance().loadStopWordContainer(ia);
	ifs.close();
}

void AnalyzerHelper::saveAnalyzerResource(
		const ConfigManager* conf) {
	const std::string& directoryName = conf->getIndexPath();
	std::ofstream ofs((directoryName + "/" + string(IndexConfig::analyzerFileName)).c_str(), std::ios::binary);
	boost::archive::binary_oarchive oa(ofs);
	SynonymContainer::getInstance().saveSynonymContainer(oa);
	StemmerContainer::getInstance().saveStemmerContainer(oa);
	StopWordContainer::getInstance().saveStopWordContainer(oa);
	ofs.close();
}

} // namesoace wrapper
} // namespace srch2


