/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CoreInfo.h"
#include "sharding/sharding/metadata_manager/Shard.h"
#include "ConfigManager.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {


CoreInfo_t::CoreInfo_t(const CoreInfo_t &src) {
    name = src.name;

    configManager = src.configManager;

    dataDir = src.dataDir;
    dataSourceType = src.dataSourceType;
    dataFile = src.dataFile;
    dataFilePath = src.dataFilePath;

    dbParameters = src.dbParameters;
    dbSharedLibraryName = src.dbSharedLibraryName;
    dbSharedLibraryPath = src.dbSharedLibraryPath;

    isPrimSearchable = src.isPrimSearchable;

    primaryKey = src.primaryKey;

    fieldLatitude = src.fieldLatitude;
    fieldLongitude = src.fieldLongitude;
    indexType = src.indexType;

    searchableAttributesInfo = src.searchableAttributesInfo;
    refiningAttributesInfo = src.refiningAttributesInfo;

    supportSwapInEditDistance = src.supportSwapInEditDistance;

    enableWordPositionIndex = src.enableWordPositionIndex;
    enableCharOffsetIndex = src.enableCharOffsetIndex;

    recordBoostFieldFlag = src.recordBoostFieldFlag;
    recordBoostField = src.recordBoostField;
    queryTermBoost = src.queryTermBoost;
    indexCreateOrLoad = src.indexCreateOrLoad;

    searchType = src.searchType;

    supportAttributeBasedSearch = src.supportAttributeBasedSearch;

    facetEnabled = src.facetEnabled;
    facetTypes = src.facetTypes;
    facetAttributes = src.facetAttributes;
    facetStarts = src.facetStarts;
    facetEnds = src.facetEnds;
    facetGaps = src.facetGaps;

    stemmerFlag = src.stemmerFlag;
    stemmerFile = src.stemmerFile;
    synonymFilterFilePath = src.synonymFilterFilePath;
    synonymKeepOrigFlag = src.synonymKeepOrigFlag;
    stopFilterFilePath = src.stopFilterFilePath;
    protectedWordsFilePath = src.protectedWordsFilePath;
    analyzerType = src.analyzerType;
    chineseDictionaryFilePath = src.chineseDictionaryFilePath;
    allowedRecordTokenizerCharacters = src.allowedRecordTokenizerCharacters;
    ports = src.ports;
    aclCoreFlag = src.aclCoreFlag;
    userFeedbackEnabledFlag = src.userFeedbackEnabledFlag;
    maxFeedbackRecordsPerQuery = src.maxFeedbackRecordsPerQuery;
    maxFeedbackQueriesCount = src.maxFeedbackQueriesCount;

}

ClusterShardId CoreInfo_t::getPrimaryShardId(unsigned partitionId) const{
	ClusterShardId rtn ;
	rtn.coreId = this->coreId;
	rtn.partitionId = partitionId;
	rtn.replicaId = 0;
	return rtn;
}

uint32_t CoreInfo_t::getDocumentLimit() const {
    return documentLimit;
}

uint64_t CoreInfo_t::getMemoryLimit() const {
    return memoryLimit;
}

uint32_t CoreInfo_t::getMergeEveryNSeconds() const {
    return mergeEveryNSeconds;
}

uint32_t CoreInfo_t::getMergeEveryMWrites() const {
    return mergeEveryMWrites;
}

uint32_t CoreInfo_t::getUpdateHistogramEveryPMerges() const {
    return updateHistogramEveryPMerges;
}

uint32_t CoreInfo_t::getUpdateHistogramEveryQWrites() const {
    return updateHistogramEveryQWrites;
}

int CoreInfo_t::getSearchType(const string &coreName) const {
	return configManager->getSearchType(coreName);
}

const std::string& CoreInfo_t::getScoringExpressionString() const
{
    return scoringExpressionString;
}

int CoreInfo_t::getSearchResponseJSONFormat() const {
    return searchResponseJsonFormat;
}

bool CoreInfo_t::getIsFuzzyTermsQuery() const
{
    return exactFuzzy;
}

const string CoreInfo_t::getSrch2Home() const {
	return configManager->getSrch2Home();
}
const string CoreInfo_t::getLicenseKeyFileName() const {
	return configManager->getLicenseKeyFileName();
}

const string CoreInfo_t::getHTTPServerAccessLogFile() const {
	return configManager->getHTTPServerAccessLogFile();
}

const string CoreInfo_t::getNewHTTPServerAccessLogFile(const string & newFile) const {
	return configManager->getNewHTTPServerAccessLogFile(newFile);
}

const Logger::LogLevel& CoreInfo_t::getHTTPServerLogLevel() const{
	return configManager->getHTTPServerLogLevel();
}

float CoreInfo_t::getDefaultSpatialQueryBoundingBox() const{
	return configManager->getDefaultSpatialQueryBoundingBox();
}

unsigned int CoreInfo_t::getKeywordPopularityThreshold() const{
	return configManager->getKeywordPopularityThreshold();
}
const unsigned CoreInfo_t::getGetAllResultsNumberOfResultsThreshold() const{
	return configManager->getGetAllResultsNumberOfResultsThreshold();
}
const unsigned CoreInfo_t::getGetAllResultsNumberOfResultsToFindInEstimationMode() const{
	return configManager->getGetAllResultsNumberOfResultsToFindInEstimationMode();
}

unsigned int CoreInfo_t::getNumberOfThreads() const {
	return configManager->getNumberOfThreads();
}

bool CoreInfo_t::getQueryTermPrefixType() const
{
    return queryTermPrefixType;
}


float CoreInfo_t::getFuzzyMatchPenalty() const
{
    return fuzzyMatchPenalty;
}

float CoreInfo_t::getQueryTermSimilarityThreshold() const
{
    return queryTermSimilarityThreshold;
}

float CoreInfo_t::getQueryTermLengthBoost() const
{
    return queryTermLengthBoost;
}

float CoreInfo_t::getPrefixMatchPenalty() const
{
    return prefixMatchPenalty;
}

ResponseType CoreInfo_t::getSearchResponseFormat() const
{
    return searchResponseContent;
}


int CoreInfo_t::getDefaultResultsToRetrieve() const
{
    return resultsToRetrieve;
}

int CoreInfo_t::getOrdering() const {
	return configManager->getOrdering();
}

int CoreInfo_t::getAttributeToSort() const
{
    return attributeToSort;
}

unsigned short CoreInfo_t::getPort(PortType_t portType) const
{

	if(portType == GlobalPortsStart){
		return 0;
	}
    if (static_cast<unsigned int> (portType) >= ports.size()) {
        return 0;
    }

    return ports[portType];
}

void CoreInfo_t::setPort(PortType_t portType, unsigned short portNumber)
{
    if (static_cast<unsigned int> (portType) >= ports.size()) {
        ports.resize(static_cast<unsigned int> (EndOfPortType), 0);
    }

    ports[portType] = portNumber;
}

// JUST FOR Wrapper TEST
void CoreInfo_t::setDataFilePath(const string& path) {
    dataFilePath = path;
}

unsigned CoreInfo_t::getCacheSizeInBytes() const
{
    return cacheSizeInBytes;
}

}
}
