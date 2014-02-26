/*
 * ServerHighLighter.cpp
 *
 *  Created on: Jan 27, 2014
 *      Author: srch2
 */

#include "ServerHighLighter.h"
#include "Srch2Server.h"
#include "AnalyzerFactory.h"
#include <instantsearch/QueryResults.h>
#include "ParsedParameterContainer.h"
#include "util/RecordSerializer.h"
#include "query/QueryResultsInternal.h"

using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {
/*
 *   The function loops over all the query results and calls genSnippetsForSingleRecord for
 *   each query result.
 */
void ServerHighLighter::generateSnippets(vector<RecordSnippet>& highlightInfo){

	unsigned upperLimit = queryResults->getNumberOfResults();
	if (upperLimit > HighlightRecOffset + HighlightRecCount)
		upperLimit = HighlightRecOffset + HighlightRecCount;
	unsigned lowerLimit = HighlightRecOffset;
	for (unsigned i = lowerLimit; i < upperLimit; ++i) {
		RecordSnippet recordSnippets;
		unsigned recordId = queryResults->getInternalRecordId(i);
		genSnippetsForSingleRecord(queryResults, i, recordSnippets);
		recordSnippets.recordId =recordId;
		highlightInfo.push_back(recordSnippets);
	}
}
void findChildNodesForPrefixNode(TrieNodePointer prefixNode, vector<unsigned>& completeKeywordsId);
void findChildNodesForPrefixNode(TrieNodePointer prefixNode, vector<unsigned>& completeKeywordsId){
	vector<TrieNodePointer> buffer;
	buffer.reserve(1000);  // reserve ~4KB to avoid frequent resizing
	TrieNodePointer currNode = prefixNode;
	buffer.push_back(prefixNode);
	while(buffer.size() > 0) {
		TrieNodePointer currNode = buffer.back(); buffer.pop_back();
		if (currNode->isTerminalNode()) {
			completeKeywordsId.push_back(currNode->id);
		}
		for(signed i = currNode->getChildrenCount() - 1; i >= 0; --i) {
			buffer.push_back(currNode->getChild(i));
		}
	}
}
void buildKeywordHighlightInfo(const QueryResults * qr, unsigned recIdx,
		vector<keywordHighlightInfo>& keywordStrToHighlight);
void buildKeywordHighlightInfo(const QueryResults * qr, unsigned recIdx,
		vector<keywordHighlightInfo>& keywordStrToHighlight){
	vector<string> matchingKeywords;
	qr->getMatchingKeywords(recIdx, matchingKeywords);
	vector<TermType> termTypes;
	qr->getTermTypes(recIdx, termTypes);
	vector<unsigned> editDistances;
	qr->getEditDistances(recIdx, editDistances);
	for (unsigned i = 0; i <  matchingKeywords.size(); ++i) {
		keywordHighlightInfo keyInfo;
		if(termTypes.at(i) == TERM_TYPE_COMPLETE)
			keyInfo.flag = 1;
		else if (termTypes.at(i) == TERM_TYPE_PHRASE)
			keyInfo.flag = 2;
		else
			keyInfo.flag = 0;
		utf8StringToCharTypeVector(matchingKeywords[i], keyInfo.key);
		keyInfo.editDistance = editDistances.at(i);
		keywordStrToHighlight.push_back(keyInfo);
	}
}
/*
 *   The function generates snippet for all the highlight attributes of a given query result.
 *   Attribute values are fetched from a compact representation stored in forward index.
 */
void ServerHighLighter::genSnippetsForSingleRecord(const QueryResults *qr, unsigned recIdx, RecordSnippet& recordSnippets) {

		/*
		 *  Code below is a setup for highlighter module
		 */
		unsigned recordId = qr->getInternalRecordId(recIdx);

		bool termOffsetInfoPresent = isEnabledCharPositionIndex(server->indexer->getSchema()->getPositionIndexType());
		vector<keywordHighlightInfo> keywordStrToHighlight;
		buildKeywordHighlightInfo(qr, recIdx, keywordStrToHighlight);

        StoredRecordBuffer buffer =  server->indexer->getInMemoryData(recordId);
        const vector<std::pair<unsigned, string> >&highlightAttributes = server->indexDataConfig->getHighlightAttributeIdsVector();
        for (unsigned i = 0 ; i < highlightAttributes.size(); ++i) {
    		AttributeSnippet attrSnippet;
        	unsigned id = highlightAttributes[i].first;
        	unsigned lenOffset = compactRecDeserializer->getSearchableOffset(id);
        	const char *attrdata = buffer.start + *((unsigned *)(buffer.start + lenOffset));
        	unsigned len = *(((unsigned *)(buffer.start + lenOffset)) + 1) -
        			*((unsigned *)(buffer.start + lenOffset));
        	snappy::Uncompress(attrdata,len, &uncompressedInMemoryRecordString);
        	try{
				this->highlightAlgorithms->getSnippet(qr, recIdx, highlightAttributes[i].first,
						uncompressedInMemoryRecordString, attrSnippet.snippet,
						storedAttrSchema->isSearchableAttributeMultiValued(id), keywordStrToHighlight);
        	}catch(const exception& ex) {
        		Logger::warn("could not generate a snippet for an record/attr %d/%d", recordId, id);
        	}
        	attrSnippet.FieldId = highlightAttributes[i].second;
        	if (attrSnippet.snippet.size() > 0)
        		recordSnippets.fieldSnippets.push_back(attrSnippet);
        }
        if (recordSnippets.fieldSnippets.size() == 0)
        	Logger::error("could not generate a snippet because search keywords could not be found in any attribute of record!!");
}

ServerHighLighter::ServerHighLighter(QueryResults * queryResults,Srch2Server *server,
		ParsedParameterContainer& param, unsigned offset, unsigned count) {

	this->queryResults = queryResults;

	HighlightConfig hconf;
	string pre, post;
	server->indexDataConfig->getExactHighLightMarkerPre(pre);
	server->indexDataConfig->getExactHighLightMarkerPost(post);
	hconf.highlightMarkers.push_back(make_pair(pre, post));
	server->indexDataConfig->getFuzzyHighLightMarkerPre(pre);
	server->indexDataConfig->getFuzzyHighLightMarkerPost(post);
	hconf.highlightMarkers.push_back(make_pair(pre, post));
	server->indexDataConfig->getHighLightSnippetSize(hconf.snippetSize);

	if (!isEnabledWordPositionIndex(server->indexer->getSchema()->getPositionIndexType())){
		// we do not need phrase information because position index is not enabled.
		param.PhraseKeyWordsInfoMap.clear();
	}
	/*
	 *  We have two ways of generating snippets.
	 *  1. Using term offsets stored in the forward index.
	 *  OR
	 *  2. By generating offset information at runtime using analyzer.
	 *
	 *  If isEnabledCharPositionIndex returns true then the term offset information is available
	 *  in the forward index. In that case we use the term offset based logic.
	 */
	// Note: check for server schema not the configuration.
	if (isEnabledCharPositionIndex(server->indexer->getSchema()->getPositionIndexType())) {
		this->highlightAlgorithms  = new TermOffsetAlgorithm(server->indexer,
				 param.PhraseKeyWordsInfoMap, hconf);
	} else {
		Analyzer *currentAnalyzer = AnalyzerFactory::getCurrentThreadAnalyzer(server->indexDataConfig);
		this->highlightAlgorithms  = new AnalyzerBasedAlgorithm(currentAnalyzer,
				 param.PhraseKeyWordsInfoMap, hconf);
	}
	this->server = server;
	storedAttrSchema = Schema::create();
	JSONRecordParser::populateStoredSchema(storedAttrSchema, server->indexer->getSchema());
	compactRecDeserializer = new RecordSerializer(*storedAttrSchema);
	this->HighlightRecOffset = offset;
	this->HighlightRecCount = count;
	this->uncompressedInMemoryRecordString.reserve(4096);
}

ServerHighLighter::~ServerHighLighter() {
	delete this->compactRecDeserializer;
	delete this->highlightAlgorithms;
	delete storedAttrSchema;
    std::map<string, vector<unsigned> *>::iterator iter =
    						prefixToCompleteStore.begin();
    while(iter != prefixToCompleteStore.end()) {
    	delete iter->second;
    	++iter;
	}
}

} /* namespace httpwrapper */
} /* namespace srch2 */
