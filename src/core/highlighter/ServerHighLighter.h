/*
 * ServerHighLighter.h
 *
 *  Created on: Jan 27, 2014
 *      Author: srch2
 */

#ifndef SERVERHIGHLIGHTER_H_
#define SERVERHIGHLIGHTER_H_

#include <vector>
#include "highlighter/Highlighter.h"
#include "../sharding/configuration/CoreInfo.h"

namespace srch2 {
namespace instantsearch {
class QueryResults;
}}

namespace srch2 {
namespace util {
class RecordSerializer;
} }

using namespace srch2::instantsearch;
using namespace std;
using namespace srch2::util;

namespace srch2 {
namespace httpwrapper {

class Srch2Server;
class ParsedParameterContainer;

class ServerHighLighter {
public:
	ServerHighLighter(QueryResults * queryResults, const Indexer *indexer, const CoreInfo_t * coreInfo,
			const vector<unsigned>& accessibleAttrs);
	virtual ~ServerHighLighter();
	void generateSnippets();
private:
	void genSnippetsForSingleRecord(const QueryResults *qr, unsigned idx, RecordSnippet& recordSnippets);
	QueryResults * queryResults;
	HighlightAlgorithm* highlightAlgorithms;
	const CoreInfo_t *coreInfo;
	const Indexer *indexer;
	RecordSerializer *compactRecDeserializer;
	Schema * storedAttrSchema;
	std::string uncompressedInMemoryRecordString;
    std::map<string, vector<unsigned> *> prefixToCompleteStore;
    vector<unsigned> accessibleAttributes;
    std::map<string, PhraseInfo> phraseKeyWordsInfoMap;
};

} /* namespace httpwrapper */
} /* namespace srch2 */
#endif /* SERVERHIGHLIGHTER_H_ */
