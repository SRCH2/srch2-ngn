/*
 * AnalyzerContainers.cpp
 *
 *  Created on: Aug 21, 2013
 *      Author: sbisht
 */

#include "AnalyzerContainers.h"
#include "util/Logger.h"
#include "util/Assert.h"
#include <boost/serialization/set.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/algorithm/string.hpp>

using srch2::util::Logger;

namespace srch2 {
namespace instantsearch{

SynonymContainer* SynonymContainer::synonymContainer = NULL;

// this is thread unsafe. Make sure you call it from main thread only.
SynonymContainer& SynonymContainer::getInstance() {
	if (synonymContainer == NULL)
	{
		synonymContainer = new SynonymContainer("=>");
	}
	return *synonymContainer;
}

void SynonymContainer::free() {
	if (synonymContainer)
	{
		delete synonymContainer;
		synonymContainer = NULL;
	}
}

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
void SynonymContainer::initSynonymContainer (const std::string synonymFilePath) {
	// using file path to create an ifstream object
	std::ifstream input(synonymFilePath.c_str());

	if (!input.good())
	{
		Logger::warn("The synonym file = \"%s\" could not be opened.", synonymFilePath.c_str());
		return;
	}
	this->synonymMap.clear();
	// Reads the map file line by line and fills the map
	std::string line;
	while (getline(input, line)) {
		/*
		 * for example we have "A=>B" in the file
		 * "A" is leftHandSide
		 * "B" is rightHandSide
		 */
		std::size_t index = line.find(this->synonymDelimiter);
		// if we don't have any synonymDelimeter in this line OR leftHandSide is empty, we should go to next line.
		if (index <= 0) {
			continue;
		}
		std::string leftHandSide = line.substr(0, index);
		std::string rightHandSide = line.substr(index + this->synonymDelimiter.length());

		boost::algorithm::trim(leftHandSide);
		boost::algorithm::trim(rightHandSide);

		/*
		 * This part will put the whole lefthandside into the map.
		 * It checks if it already exists or not.
		 * If the lefthandside is already there: only if it was prefix_only we will change it to prefix_and_complete, Otherwise, we won't touch it.
		 * If the lefthandside is not already there: it inserst it into the map.
		 */
		std::map<std::string, std::pair<SynonymTokenType, std::string> >::const_iterator pos = this->synonymMap.find(leftHandSide);
		if (pos != this->synonymMap.end()) {
			if (this->synonymMap[leftHandSide].first == SYNONYM_PREFIX_ONLY) {
				ASSERT(this->synonymMap[leftHandSide].second == "");
				this->synonymMap[leftHandSide].first =  SYNONYM_PREFIX_AND_COMPLETE;
				this->synonymMap[leftHandSide].second =  rightHandSide;
			}
		} else {
			this->synonymMap.insert(make_pair(leftHandSide, make_pair(SYNONYM_COMPLETE_ONLY, rightHandSide)));
		}
		ASSERT(this->synonymMap[leftHandSide].first != SYNONYM_NOT_PREFIX_NOT_COMPLETE);

		/*
		 * Here will add the sub sequence of Tokens to the map.
		 * For example, if the lefthandside is "new york city", the whole std::string is already inserted into the map.
		 * Now we should take care of "new york" and "new"
		 * In the while() loop, first "new york" will be added and then "new"
		 * For each of them, if it is already there and its flag is complete_only, we change it to prefix_and_complete and if it is not there, we add it as prefix_only
		 */
		std::size_t found ;
		while (true) {
			found = leftHandSide.rfind(" ");
			if (found == std::string::npos) {
				break;
			}
			leftHandSide = leftHandSide.substr(0, found);
			std::map<std::string, std::pair<SynonymTokenType, std::string> >::const_iterator pos = this->synonymMap.find(leftHandSide);
			if (pos != this->synonymMap.end()) {
				if (this->synonymMap[leftHandSide].first == SYNONYM_COMPLETE_ONLY) {
					ASSERT(this->synonymMap[leftHandSide].second != ""); // unless the righthandsde is empty in the synonym file
					this->synonymMap[leftHandSide].first =  SYNONYM_PREFIX_AND_COMPLETE;
				}
			} else {
				this->synonymMap.insert(std::make_pair(leftHandSide, std::make_pair(SYNONYM_PREFIX_ONLY, "")));
			}
			ASSERT(this->synonymMap[leftHandSide].first != SYNONYM_NOT_PREFIX_NOT_COMPLETE);
		}

	}

}

bool SynonymContainer::contains(const std::string& str) {
	std::map<std::string, pair<SynonymTokenType, std::string> >::const_iterator pos = synonymMap.find(str);
	if (pos != this->synonymMap.end())
		return true;
	else
		return false;
}

void SynonymContainer::getValue(const std::string& str, std::pair<SynonymTokenType, std::string>& returnValue) {
	std::map<std::string, pair<SynonymTokenType, std::string> >::const_iterator pos = synonymMap.find(str);
	if (pos != this->synonymMap.end()) {
		returnValue = pos->second;
	}
	else {
		returnValue = make_pair(SYNONYM_NOT_PREFIX_NOT_COMPLETE, "");
	}
}

void SynonymContainer::loadSynonymContainer (boost::archive::binary_iarchive& ia) {
	ia >> synonymMap;
}

void SynonymContainer::saveSynonymContainer (boost::archive::binary_oarchive& oa) {
	oa << synonymMap;
}

StemmerContainer* StemmerContainer::stemmerContainer = NULL;
StemmerContainer& StemmerContainer::getInstance() {
	if (stemmerContainer == NULL)
	{
		stemmerContainer = new StemmerContainer();
	}
	return *stemmerContainer;
}

void StemmerContainer::free() {
	if (stemmerContainer)
	{
		delete stemmerContainer;
		stemmerContainer = NULL;
	}
}

void StemmerContainer::initStemmerContainer( const std::string stemmerFilePath) {
	std::ifstream input(stemmerFilePath.c_str());
	//  If the file path is OK, it will be passed, else this if will run and the error will be shown
	if (input.fail()) {
        Logger::warn("The stemmer file = \"%s\" could not be opened.", stemmerFilePath.c_str());
 		return;
	}
	//	Reads the dictionary file line by line and makes the Map, dictionaryWords are the words extracted from the dictionary file
	this->dictionaryWords.clear();
	std::string str;
	while (getline(input, str)) {
		boost::algorithm::trim(str);
		this->dictionaryWords.insert(make_pair(str, 1));
	}
}

bool StemmerContainer::contains(const std::string& str){
	std::map<std::string, int>::const_iterator iter = this->dictionaryWords.find(str);
	if (iter != this->dictionaryWords.end()) {
		return true;
	} else {
		return false;
	}
}

void StemmerContainer::loadStemmerContainer(boost::archive::binary_iarchive& ia) {
	ia >> dictionaryWords;
}

void StemmerContainer::saveStemmerContainer(boost::archive::binary_oarchive& oa) {
	oa << dictionaryWords;
}

StopWordContainer* StopWordContainer::stopWordContainer = NULL;
StopWordContainer& StopWordContainer::getInstance() {
	if (stopWordContainer == NULL)
	{
		stopWordContainer = new StopWordContainer();
	}
	return *stopWordContainer;
}
void StopWordContainer::free() {
	if (stopWordContainer)
	{
		delete stopWordContainer;
		stopWordContainer = NULL;
	}
}
void StopWordContainer::initStopWordContainer(const std::string stopWordsFilePath) {
	std::string str;
	//  using file path to create an ifstream object
	std::ifstream input(stopWordsFilePath.c_str());
		//  If the file path is OK, it will be passed, else this if will run and the error will be shown
	if (input.fail()) {
	    Logger::warn("The stop words file = \"%s\" could not be opened.", stopWordsFilePath.c_str());
		return;
	}
	//	Reads the stop word files line by line and fills the vector
	this->stopWordsSet.clear();
	while (getline(input, str)) {
		boost::algorithm::trim(str);
		this->stopWordsSet.insert(str);
	}
}

bool StopWordContainer::contains(const std::string& str) {
	return (this->stopWordsSet.find(str) != this->stopWordsSet.end());
}
void StopWordContainer::loadStopWordContainer( boost::archive::binary_iarchive& ia) {
	ia >> this->stopWordsSet;
}

void StopWordContainer::saveStopWordContainer(boost::archive::binary_oarchive& oa) {
	oa << this->stopWordsSet;
}

} // instantsearch
} // namespace srch2
