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
#include "util/RecordSerializerUtil.h"

using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {
/*
 *   The function loops over all the query results and calls genSnippetsForSingleRecord for
 *   each query result.
 */

void ServerHighLighter::generateSnippets(map<string,std::pair<string, RecordSnippet> >& highlightInfo){

	unsigned upperLimit = queryResults->getNumberOfResults();
	if (upperLimit > HighlightRecOffset + HighlightRecCount)
		upperLimit = HighlightRecOffset + HighlightRecCount;
	unsigned lowerLimit = HighlightRecOffset;
	for (unsigned i = lowerLimit; i < upperLimit; ++i) {
		string key = queryResults->getRecordId(i);
		map<string,std::pair<string, RecordSnippet> >::iterator iter = highlightInfo.find(key);
		if (iter == highlightInfo.end())
			continue;
		RecordSnippet& recordSnippets = iter->second.second;
		unsigned recordId = queryResults->getInternalRecordId(i);
		genSnippetsForSingleRecord(queryResults, i, recordSnippets);
		recordSnippets.recordId =recordId;
		//highlightInfo.push_back(recordSnippets);
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
	vector<vector<unsigned> > matchedAttributeIdsList;
	qr->getMatchedAttributes(recIdx, matchedAttributeIdsList);
	for (unsigned i = 0; i <  matchingKeywords.size(); ++i) {
		keywordHighlightInfo keywordInfo;
		if(termTypes.at(i) == TERM_TYPE_COMPLETE)
			keywordInfo.flag = HIGHLIGHT_KEYWORD_IS_COMPLETE;
		else if (termTypes.at(i) == TERM_TYPE_PHRASE)
			keywordInfo.flag = HIGHLIGHT_KEYWORD_IS_PHRASE;
		else
			keywordInfo.flag = HIGHLIGHT_KEYWORD_IS_PERFIX;
		utf8StringToCharTypeVector(matchingKeywords[i], keywordInfo.key);
		keywordInfo.editDistance = editDistances.at(i);
		keywordInfo.attributeIdsList = matchedAttributeIdsList.at(i);
		keywordStrToHighlight.push_back(keywordInfo);
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

		vector<keywordHighlightInfo> keywordStrToHighlight;
		buildKeywordHighlightInfo(qr, recIdx, keywordStrToHighlight);

        StoredRecordBuffer buffer =  server->getIndexer()->getInMemoryData(recordId);
        if (buffer.start.get() == NULL)
        	return;
        const vector<std::pair<unsigned, string> >&highlightAttributes = server->getCoreInfo()->getHighlightAttributeIdsVector();
        for (unsigned i = 0 ; i < highlightAttributes.size(); ++i) {
    		AttributeSnippet attrSnippet;
        	unsigned id = highlightAttributes[i].first;
        	// check whether the searchable attribute is accessible for current role-id.
        	// snippet is generated for accessible searchable attributes only.
        	bool isFieldAccessible = server->indexer->getAttributeAcl().isSearchableFieldAccessibleForRole(
        			aclRoleValue, highlightAttributes[i].second);
        	if (!isFieldAccessible)
        		continue;  // ignore unaccessible attributes. Do not generate snippet.
        	unsigned lenOffset = compactRecDeserializer->getSearchableOffset(id);
        	const char *attrdata = buffer.start.get() + *((unsigned *)(buffer.start.get() + lenOffset));
        	unsigned len = *(((unsigned *)(buffer.start.get() + lenOffset)) + 1) -
        			*((unsigned *)(buffer.start.get() + lenOffset));
        	snappy::Uncompress(attrdata,len, &uncompressedInMemoryRecordString);
        	try{
				this->highlightAlgorithms->getSnippet(qr, recIdx, highlightAttributes[i].first,
						uncompressedInMemoryRecordString, attrSnippet.snippet,
						storedAttrSchema->isSearchableAttributeMultiValued(id), keywordStrToHighlight);
        	}catch(const exception& ex) {
        		Logger::debug("could not generate a snippet for an record/attr %d/%d", recordId, id);
        	}
        	attrSnippet.FieldId = highlightAttributes[i].second;
        	if (attrSnippet.snippet.size() > 0)
        		recordSnippets.fieldSnippets.push_back(attrSnippet);
        }
        if (recordSnippets.fieldSnippets.size() == 0)
        	Logger::warn("could not generate a snippet because search keywords could not be found in any attribute of record!!");
}

ServerHighLighter::ServerHighLighter(QueryResults * queryResults,Srch2Server *server,
		ParsedParameterContainer& param, unsigned offset, unsigned count) {

	this->queryResults = queryResults;

	HighlightConfig hconf;
	string pre, post;
	server->getCoreInfo()->getExactHighLightMarkerPre(pre);
	server->getCoreInfo()->getExactHighLightMarkerPost(post);
	hconf.highlightMarkers.push_back(make_pair(pre, post));
	server->getCoreInfo()->getFuzzyHighLightMarkerPre(pre);
	server->getCoreInfo()->getFuzzyHighLightMarkerPost(post);
	hconf.highlightMarkers.push_back(make_pair(pre, post));
	server->getCoreInfo()->getHighLightSnippetSize(hconf.snippetSize);
	this->aclRoleValue = param.roleId;
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

	//  Below code is disabled for V0. Without this code phrase search highlighting will not work.
//	if (isEnabledCharPositionIndex(server->indexer->getSchema()->getPositionIndexType())) {
//		this->highlightAlgorithms  = new TermOffsetAlgorithm(server->indexer,
//				 param.PhraseKeyWordsInfoMap, hconf);
//	} else {
//		Analyzer *currentAnalyzer = AnalyzerFactory::getCurrentThreadAnalyzer(server->indexDataConfig);
//		this->highlightAlgorithms  = new AnalyzerBasedAlgorithm(currentAnalyzer,
//				 param.PhraseKeyWordsInfoMap, hconf);
//	}
	if (isEnabledCharPositionIndex(server->getIndexer()->getSchema()->getPositionIndexType())) {
		this->highlightAlgorithms  = new TermOffsetAlgorithm(server->getIndexer(),
				PhraseKeyWordsInfoMap, hconf);
	} else {
		Analyzer *currentAnalyzer = AnalyzerFactory::getCurrentThreadAnalyzerWithSynonyms(server->getCoreInfo());
		this->highlightAlgorithms  = new AnalyzerBasedAlgorithm(currentAnalyzer,
				PhraseKeyWordsInfoMap, hconf);
	}

	this->server = server;
	storedAttrSchema = Schema::create();
	RecordSerializerUtil::populateStoredSchema(storedAttrSchema, server->getIndexer()->getSchema());
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
