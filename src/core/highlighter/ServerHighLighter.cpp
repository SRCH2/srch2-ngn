/*
 * ServerHighLighter.cpp
 *
 *  Created on: Jan 27, 2014
 *      Author: srch2
 */

#include "ServerHighLighter.h"
//#include "Srch2Server.h"
#include "operation/IndexerInternal.h"
#include "analyzer/AnalyzerFactory.h"
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

void ServerHighLighter::generateSnippets(){
	for( unsigned i = 0 ; i < queryResults->impl->sortedFinalResults.size() ; i++){
		unsigned recordId = queryResults->getInternalRecordId(i);
		RecordSnippet& recordSnippet = queryResults->impl->sortedFinalResults.at(i)->recordSnippet;
		genSnippetsForSingleRecord(queryResults, i, recordSnippet);
		recordSnippet.recordId =recordId;
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

        StoredRecordBuffer buffer =  indexer->getInMemoryData(recordId);
        if (buffer.start.get() == NULL)
        	return;
        const vector<std::pair<unsigned, string> >&highlightAttributes = coreInfo->getHighlightAttributeIdsVector();
        for (unsigned i = 0 ; i < highlightAttributes.size(); ++i) {
    		AttributeSnippet attrSnippet;
        	unsigned id = highlightAttributes[i].first;
        	// check whether the searchable attribute is accessible for current role-id.
        	// snippet is generated for accessible searchable attributes only.

        	bool isFieldAccessible = AttributeAccessControl::isFieldAccessible(id,
        			accessibleAttributes, indexer->getSchema()->getNonAclSearchableAttrIdsList());
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

ServerHighLighter::ServerHighLighter(QueryResults * queryResults,
		const Indexer *indexer, const CoreInfo_t * coreInfo, const vector<unsigned>& accessibleAttrs) {

	this->queryResults = queryResults;
	this->indexer = indexer;
	this->coreInfo = coreInfo;
	HighlightConfig hconf;
	string pre, post;
	coreInfo->getExactHighLightMarkerPre(pre);
	coreInfo->getExactHighLightMarkerPost(post);
	hconf.highlightMarkers.push_back(make_pair(pre, post));
	coreInfo->getFuzzyHighLightMarkerPre(pre);
	coreInfo->getFuzzyHighLightMarkerPost(post);
	hconf.highlightMarkers.push_back(make_pair(pre, post));
	coreInfo->getHighLightSnippetSize(hconf.snippetSize);
	this->accessibleAttributes = accessibleAttrs;
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

	if (isEnabledCharPositionIndex(indexer->getSchema()->getPositionIndexType())) {
		this->highlightAlgorithms  = new TermOffsetAlgorithm(indexer,
				phraseKeyWordsInfoMap, hconf);
	} else {
		Analyzer *currentAnalyzer = AnalyzerFactory::getCurrentThreadAnalyzerWithSynonyms(coreInfo);
		this->highlightAlgorithms  = new AnalyzerBasedAlgorithm(currentAnalyzer,
				phraseKeyWordsInfoMap, hconf);
	}

	storedAttrSchema = Schema::create();
	RecordSerializerUtil::populateStoredSchema(storedAttrSchema, indexer->getSchema());
	compactRecDeserializer = new RecordSerializer(*storedAttrSchema);
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
