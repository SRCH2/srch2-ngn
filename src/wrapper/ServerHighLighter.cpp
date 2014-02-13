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

using namespace srch2::util;

namespace srch2 {
namespace httpwrapper {
/*
 *   The function loops over all the query results and calls genSnippetsForSingleRecord for
 *   each query result.
 */
void ServerHighLighter::generateSnippets(vector<RecordSnippet>& highlightInfo){

	for (unsigned i = 0; i < queryResults->getNumberOfResults(); ++i) {
		if(i < HighlightRecOffset)
			continue;
		if (i >= HighlightRecOffset + HighlightRecCount)
			break;
		RecordSnippet recordSnippets;
		unsigned recordId = queryResults->getInternalRecordId(i);
		genSnippetsForSingleRecord(recordId, recordSnippets);
		recordSnippets.recordId =recordId;
		highlightInfo.push_back(recordSnippets);
	}
}
/*
 *   The function generates snippet for all the highlight attributes of a given query result.
 *   Attribute values are fetched from a compact representation stored in forward index.
 */
void ServerHighLighter::genSnippetsForSingleRecord(unsigned recordId, RecordSnippet& recordSnippets) {

        StoredRecordBuffer buffer =  server->indexer->getInMemoryData(recordId);
        const vector<std::pair<unsigned, string> >&highlightAttributes = server->indexDataConfig->getHighlightAttributeIdsVector();
        for (unsigned i = 0 ; i < highlightAttributes.size(); ++i) {
    		AttributeSnippet attrSnippet;
        	unsigned id = highlightAttributes[i].first;
        	unsigned lenOffset = compactRecDeserializer->getSearchableOffset(id);
        	const char *attrdata = buffer.start + *((unsigned *)(buffer.start + lenOffset));
        	unsigned len = *(((unsigned *)(buffer.start + lenOffset)) + 1) -
        			*((unsigned *)(buffer.start + lenOffset));

        	std::string uncompressedInMemoryRecordString;
        	snappy::Uncompress(attrdata,len, &uncompressedInMemoryRecordString);
        	try{
				this->highlightAlgorithms->getSnippet(recordId, highlightAttributes[i].first,
						uncompressedInMemoryRecordString, attrSnippet.snippet,
						storedAttrSchema->isSearchableAttributeMultiValued(id));
        	}catch(exception ex) {
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
				queryResults, param.PhraseKeyWordsInfoMap, hconf);
	} else {
		Analyzer *currentAnalyzer = AnalyzerFactory::getCurrentThreadAnalyzer(server->indexDataConfig);
		this->highlightAlgorithms  = new AnalyzerBasedAlgorithm(currentAnalyzer,
				queryResults, param.PhraseKeyWordsInfoMap, hconf);
	}
	this->server = server;
	storedAttrSchema = Schema::create();
	JSONRecordParser::populateStoredSchema(storedAttrSchema, server->indexer->getSchema());
	compactRecDeserializer = new RecordSerializer(*storedAttrSchema);
	this->HighlightRecOffset = offset;
	this->HighlightRecCount = count;
}

ServerHighLighter::~ServerHighLighter() {
	delete this->compactRecDeserializer;
	delete this->highlightAlgorithms;
	delete storedAttrSchema;
}

} /* namespace httpwrapper */
} /* namespace srch2 */
