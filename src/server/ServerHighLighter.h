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
/*
 * ServerHighLighter.h
 *
 *  Created on: Jan 27, 2014
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

using namespace srch2::instantsearch;
using namespace std;
using namespace srch2::util;

namespace srch2 {
namespace httpwrapper {

class Srch2Server;
class ParsedParameterContainer;

class ServerHighLighter {
public:
	ServerHighLighter(QueryResults * queryResults,Srch2Server *server,
			ParsedParameterContainer& param, unsigned offset, unsigned count);
	virtual ~ServerHighLighter();
	void generateSnippets(vector<RecordSnippet>& highlightInfo);
private:
	void genSnippetsForSingleRecord(const QueryResults *qr, unsigned idx, RecordSnippet& recordSnippets);
	QueryResults * queryResults;
	HighlightAlgorithm* highlightAlgorithms;
	Srch2Server *server;
	RecordSerializer *compactRecDeserializer;
	Schema * storedAttrSchema;
	unsigned HighlightRecOffset;
	unsigned HighlightRecCount;
	std::string uncompressedInMemoryRecordString;
    std::map<string, vector<unsigned> *> prefixToCompleteStore;
    string aclRoleValue;
};

} /* namespace httpwrapper */
} /* namespace srch2 */
#endif /* SERVERHIGHLIGHTER_H_ */
