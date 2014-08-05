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
#include <boost/serialization/vector.hpp>
#include <boost/algorithm/string.hpp>

using srch2::util::Logger;

namespace srch2 {
namespace instantsearch{

AnalyzerContainer::Map_t AnalyzerContainer::containers;

void AnalyzerContainer::free(const string &path)
{
    Map_t::iterator target = containers.find(path);
    if (target != containers.end()) {
        containers.erase(target); // map::erase also calls delete on target
    }
}

void AnalyzerContainer::freeAll()
{
	AnalyzerContainer::Map_t::iterator iter = containers.begin();
    for (; iter != containers.end(); ++iter) {
       delete iter->second;
    }
    containers.erase(containers.begin(), containers.end());
}

AnalyzerContainer::~AnalyzerContainer()
{
}

// this is thread unsafe. Make sure you call it from main thread only.
SynonymContainer *SynonymContainer::getInstance(const std::string &filePath,
                                                SynonymKeepOriginFlag synonymKeepOriginFlag)
{
    SynonymContainer *synonymContainer = NULL;
    char buffer[32];

    sprintf(buffer, "SynonymContainer:%d:", synonymKeepOriginFlag);
    string key(buffer);
    key.append(filePath);

    Map_t::iterator iterator = containers.find(key);
    if (iterator == containers.end()) {
        synonymContainer = new SynonymContainer("=>");
        synonymContainer->filePath = filePath;
        synonymContainer->synonymKeepOriginFlag = synonymKeepOriginFlag;
        containers[key] = synonymContainer;
    } else {
        synonymContainer = dynamic_cast<SynonymContainer *> (iterator->second);
        // in case the same path was used to create a different type of container
        if (synonymContainer == NULL) {
            Logger::warn("AnalyzerContainer for %s found but of type %s instead of SynonymContainer",
                         filePath.c_str(), typeid(*(iterator->second)).name());
        }
        ASSERT(synonymContainer != NULL);
    }
    return synonymContainer;
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
void SynonymContainer::init() {
	// using file path to create an ifstream object
	std::ifstream input(filePath.c_str());

	if (!input.good())
	{
		Logger::warn("The synonym file = \"%s\" could not be opened.", filePath.c_str());
		return;
	}
	this->synonymMap.clear();
	this->prefixMap.clear();
	// Reads the map file line by line and fills the map
	std::string line;
	bool expandFlag = false;
	while (getline(input, line)) {

		if (line.size() == 0 || line.at(0) == '#') {
			// line is empty or is a comment.
			continue;
		}

		/* We allow synonym file similar to Solr. There are two types of synonym declaration.
		 * 1.  Explicit declaration
		 * for example we have "A=>B" in the file
		 * "A" is leftHandSide
		 * "B" is rightHandSide
		 *
		 * Each instance of A will be replaced with B in the index or query.
		 *
		 * 2. Equivalent synonyms :  A, B, C
		 *
		 * if Keep Original is ON, then treat it as:
		 * A => B, C B => A, C C => A, B
		 * Otherwise if Keep Original is OFF then it is treated as B, C => A
		 */
		std::size_t index = line.find(this->synonymDelimiter);
		//leftHandSide is empty, we should go to next line.
		if (index == 0) {
			continue;
		}
		vector<string> leftHandSideTokens;
		vector<string> rightHandSideTokens;
		std::string leftHandSide;
		std::string rightHandSide;
		expandFlag = false;
		if (index == string::npos) {
			// if we don't have any synonymDelimeter in this line, then we may have equivalent
			// synonyms declared in this line.
			vector<string> synonyms;
			boost::algorithm::split(synonyms, line, boost::is_any_of(","));
			if (synonyms.size() <= 1) {
				// unknown format ..skip
				continue;
			}
			// we have line in form of A,B,C
			if (synonymKeepOriginFlag == SYNONYM_KEEP_ORIGIN) {
				// A,B,C => A,B,C
				leftHandSideTokens.assign(synonyms.begin(), synonyms.end());
				rightHandSideTokens.assign(synonyms.begin(), synonyms.end());
				expandFlag = true;
			} else {
				// B,C => A
				leftHandSideTokens.assign(synonyms.begin() + 1, synonyms.end());
				rightHandSideTokens.push_back(synonyms[0]);
			}
		} else {
			std::string leftHandSide = line.substr(0, index);
			std::string rightHandSide = line.substr(index + this->synonymDelimiter.length());

			boost::algorithm::split(leftHandSideTokens, leftHandSide, boost::is_any_of(","));
			boost::algorithm::split(rightHandSideTokens, rightHandSide, boost::is_any_of(","));
		}

		/*
		 * Iterate through the synonym mapping of "A, B, C => A, B, C".
		 * For the outer loop, we are dealing with A first. For the inner loop,
		 * we are dealing with "B, C".
		 */

		for (unsigned i = 0; i < leftHandSideTokens.size(); ++i) {
			leftHandSide = leftHandSideTokens[i];
			boost::algorithm::trim(leftHandSide);
			if (leftHandSide.size() == 0) {
				continue;
			}
			transform(leftHandSide.begin(),leftHandSide.end(),leftHandSide.begin(),::tolower);
			for (unsigned j = 0; j < rightHandSideTokens.size(); ++j) {

				rightHandSide = rightHandSideTokens[j];
				boost::algorithm::trim(rightHandSide);
				if (rightHandSide.size() == 0) {
					continue;
				}
				transform(rightHandSide.begin(),rightHandSide.end(),rightHandSide.begin(),::tolower);
				if (leftHandSide == rightHandSide)
					continue;

				/*
				 * This part will put the whole lefthandside into the map.
				 * It checks if it already exists or not.
				 */

				std::map<std::string, std::pair<bool ,SynonymVector> >::const_iterator pos = this->synonymMap.find(leftHandSide);
				if (pos != this->synonymMap.end()) {
					SynonymVector& synonymVector = this->synonymMap[leftHandSide].second;
					if (find(synonymVector.begin(), synonymVector.end(), rightHandSide) == synonymVector.end()){
						this->synonymMap[leftHandSide].second.push_back(rightHandSide);
					}
				} else {
					SynonymVector synonymVector;
					this->synonymMap.insert(make_pair(leftHandSide, make_pair(expandFlag, synonymVector)));
					this->synonymMap[leftHandSide].second.push_back(rightHandSide);
				}

				/*
				 * Here will add the sub sequence of Tokens to the map.
				 * For example, if the lefthandside is "new york city", the whole std::string is
				 * already inserted into the map. Now we should take care of "new york" and "new"
				 * In the while() loop, first "new york" will be added and then "new"
				 * The reason that we need to keep the prefix map is to keep track of multi-word
				 * synonyms which arrive from upstream filters as a individual token. So when we
				 * see "new" token then using prefix map we know that we might get a possible synonym
				 * in next iteration.
				 */
				std::size_t found ;
				string prefixToken = leftHandSide;
				while (true) {
					found = prefixToken.rfind(" ");
					if (found == std::string::npos) {
						break;
					}
					prefixToken = prefixToken.substr(0, found);
					this->prefixMap.insert(prefixToken);
				}
			}
		}

	}

}

bool SynonymContainer::contains(const std::string& str) const
{
	std::map<std::string, std::pair<bool, SynonymVector> >::const_iterator pos = synonymMap.find(str);
	if (pos != this->synonymMap.end())
		return true;
	else
		return false;
}

bool SynonymContainer::isPrefix(const std::string& str) const
{
	std::set<std::string>::const_iterator pos = prefixMap.find(str);
	if (pos != this->prefixMap.end())
		return true;
	else
		return false;
}
bool SynonymContainer::getValue(const std::string& str, SynonymVector& returnValue) const
{
	std::map<std::string, std::pair<bool, SynonymVector> >::const_iterator pos = synonymMap.find(str);
	if (pos != this->synonymMap.end()) {
		returnValue.assign(pos->second.second.begin(), pos->second.second.end());
		return pos->second.first;
	}
	return false;
}

void SynonymContainer::loadSynonymContainer (boost::archive::binary_iarchive& ia) {
	ia >> synonymMap;
	ia >> prefixMap;
}

void SynonymContainer::saveSynonymContainer (boost::archive::binary_oarchive& oa) {
	oa << synonymMap;
	oa << prefixMap;
}


StemmerContainer *StemmerContainer::getInstance(const std::string &filePath)
{
    StemmerContainer *stemmerContainer = NULL;
    Map_t::iterator iterator = containers.find(string("StemmerContainer:") + filePath);
    if (iterator == containers.end()) {
        stemmerContainer = new StemmerContainer();
        stemmerContainer->filePath = filePath;
        containers[string("StemmerContainer:") + filePath] = stemmerContainer;
    } else {
        stemmerContainer = dynamic_cast<StemmerContainer *> (iterator->second);
        // in case the same path was used to create a different type of container
        if (stemmerContainer == NULL) {
            Logger::warn("AnalyzerContainer for %s found but of type %s instead of StemmerContainer",
                         filePath.c_str(), typeid(*(iterator->second)).name());
        }
        ASSERT(stemmerContainer != NULL);
    }
    return stemmerContainer;
}

void StemmerContainer::init()
{
	std::ifstream input(filePath.c_str());
	//  If the file path is OK, it will be passed, else this if will run and the error will be shown
	if (input.fail()) {
        Logger::warn("The stemmer file = \"%s\" could not be opened.", filePath.c_str());
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

bool StemmerContainer::contains(const std::string& str) const
{
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


StopWordContainer *StopWordContainer::getInstance(const std::string &filePath)
{
    StopWordContainer *stopWordContainer = NULL;
    Map_t::iterator iterator = containers.find(string("StopWordContainer:") + filePath);
    if (iterator == containers.end()) {
        stopWordContainer = new StopWordContainer();
        stopWordContainer->filePath = filePath;
        containers[string("StopWordContainer:") + filePath] = stopWordContainer;
    } else {
        stopWordContainer = dynamic_cast<StopWordContainer *> (iterator->second);
        // in case the same path was used to create a different type of container
        if (stopWordContainer == NULL) {
            Logger::warn("AnalyzerContainer for %s found but of type %s instead of StopWordContainer",
                         filePath.c_str(), typeid(*(iterator->second)).name());
        }
        ASSERT(stopWordContainer != NULL);
    }
    return stopWordContainer;
}

void StopWordContainer::init()
{
	std::string str;
	//  using file path to create an ifstream object
	std::ifstream input(filePath.c_str());
		//  If the file path is OK, it will be passed, else this if will run and the error will be shown
	if (input.fail()) {
	    Logger::warn("The stop words file = \"%s\" could not be opened.", filePath.c_str());
		return;
	}
	//	Reads the stop word files line by line and fills the vector
	this->stopWordsSet.clear();
	while (getline(input, str)) {
		boost::algorithm::trim(str);
		this->stopWordsSet.insert(str);
	}
}

bool StopWordContainer::contains(const std::string& str) const
{
	return (this->stopWordsSet.find(str) != this->stopWordsSet.end());
}
void StopWordContainer::loadStopWordContainer( boost::archive::binary_iarchive& ia) {
	ia >> this->stopWordsSet;
}

void StopWordContainer::saveStopWordContainer(boost::archive::binary_oarchive& oa) {
	oa << this->stopWordsSet;
}


ProtectedWordsContainer *ProtectedWordsContainer::getInstance(const std::string &filePath)
{
    ProtectedWordsContainer *protectedWordsContainer = NULL;
    Map_t::iterator iterator = containers.find(string("ProtectedWordsContainer:") + filePath);
    if (iterator == containers.end()) {
        protectedWordsContainer = new ProtectedWordsContainer();
        protectedWordsContainer->filePath = filePath;
        containers[string("ProtectedWordsContainer:") + filePath] = protectedWordsContainer;
    } else {
        protectedWordsContainer = dynamic_cast<ProtectedWordsContainer *> (iterator->second);
        // in case the same path was used to create a different type of container
        if (protectedWordsContainer == NULL) {
            Logger::warn("AnalyzerContainer for %s found but of type %s instead of ProtectedWordsContainer",
                         filePath.c_str(), typeid(*(iterator->second)).name());
        }
        ASSERT(protectedWordsContainer != NULL);
    }
    return protectedWordsContainer;
}

void ProtectedWordsContainer::init()
{
	std::string str;
	//  using file path to create an ifstream object
	std::ifstream input(filePath.c_str());
		//  If the file path is OK, it will be passed, else this if will run and the error will be shown
	if (input.fail()) {
	    Logger::warn("The protected words file = \"%s\" could not be opened.", filePath.c_str());
		return;
	}
	//	Reads the stop word files line by line and fills the vector
	this->protectedWords.clear();
	while (getline(input, str)) {
		boost::algorithm::trim(str);
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		this->protectedWords.insert(str);
	}
}

bool ProtectedWordsContainer::isProtected(const string& val) const
{
    return protectedWords.count(val) > 0; 
}

} // instantsearch
} // namespace srch2
