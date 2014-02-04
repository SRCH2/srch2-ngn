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

namespace srch2 {
namespace instantsearch {
class QueryResults;
}}

namespace srch2 {
namespace util {
class RecordSerializer;
} }

using namespace srch2::instantsearch;;
using namespace std;
using namespace srch2::util;

namespace srch2 {
namespace httpwrapper {

class Srch2Server;
class ParsedParameterContainer;

class ServerHighLighter {
public:
	ServerHighLighter(QueryResults * queryResults,Srch2Server *server,
			ParsedParameterContainer& param);
	virtual ~ServerHighLighter();
	void generateSnippets(vector<RecordSnippet>& highlightInfo);
private:
	void genSnippetsForSingleRecord(unsigned recordId, RecordSnippet& recordSnippets);
	QueryResults * queryResults;
	HighlightAlgorithm* highlightAlgorithms;
	Srch2Server *server;
	RecordSerializer *compactRecDeserializer;
	Schema * storedAttrSchema;
};

} /* namespace httpwrapper */
} /* namespace srch2 */
#endif /* SERVERHIGHLIGHTER_H_ */
