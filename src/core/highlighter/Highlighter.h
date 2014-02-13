//$Id: $
/*
 * Highlighter.h
 *
 *  Created on: Jan 9, 2014
 *      Author: Surendra Bisht
 */

#ifndef HIGHLIGHTER_H_
#define HIGHLIGHTER_H_

#include <instantsearch/QueryResults.h>
#include <instantsearch/ResultsPostProcessor.h>
#include "boost/unordered_map.hpp"
#include "boost/unordered_set.hpp"

using namespace std;

namespace srch2 {
namespace instantsearch {

class Analyzer;
struct AttributeSnippet{
	string FieldId;
	// vector is for multiple snippets per field ..although we do not support it yet.
	vector<string> snippet;
};
struct RecordSnippet {
	unsigned recordId;
	vector<AttributeSnippet> fieldSnippets;
};
struct AttributeHighlights {
	vector<char *> snippets;
};

struct keywordHighlightInfo{
	uint8_t flag;  // prefix = 0, complete = 1, unverified phraseOnly = 2, Hybrid = 3
	vector<CharType> key;
	keywordHighlightInfo(){
		flag = 0;
	}
};

struct PhraseTermInfo{
	vector<unsigned>* recordPosition;
	unsigned queryPosition;
	PhraseTermInfo() {
		recordPosition = NULL;
		queryPosition = 0;
	}
};

struct PhraseInfoForHighLight{
	// term is identified by its index in this vector
	vector<PhraseTermInfo> phraseKeyWords;
	unsigned slop;
	PhraseInfoForHighLight() { slop = 0; }
};

struct HighlightConfig{
	std::string highlightMarkerPre;
	std::string highlightMarkerPost;
	unsigned snippetSize;
};

const unsigned MIN_SNIPPET_SIZE = 100;

class HighlightAlgorithm {
public:
	struct matchedTermInfo{
			uint8_t flag;
			unsigned id;
			unsigned offset;
			unsigned len;
		};
	HighlightAlgorithm(vector<keywordHighlightInfo>& keywordStrToHighlight,
			std::map<string, PhraseInfo>& phrasesInfoMap, const HighlightConfig& hconfig);
	HighlightAlgorithm(vector<keywordHighlightInfo>& keywordStrToHighlight,
			vector<PhraseInfoForHighLight>& phrasesInfoList, const HighlightConfig& hconfig);
	virtual void getSnippet(unsigned recordId, unsigned attributeId, const string& dataIn, vector<string>& snippets, bool isMultiValued) = 0;

	void _genSnippet(const vector<CharType>& dataIn, vector<CharType>& snippets, unsigned snippetUpperEnd,
			unsigned snippetLowerEnd, const vector<matchedTermInfo>& highlightPositions);
	void insertHighlightMarkerIntoSnippets(vector<CharType>& snippets,
			const vector<CharType>& dataIn, unsigned lowerOffset, unsigned upperOffset,
			const vector<matchedTermInfo>& highlightPositions,
			unsigned snippetLowerEnd, unsigned snippetUpperEnd);
	void removeInvalidPositionInPlace(vector<matchedTermInfo>& highlightPositions);
	virtual ~HighlightAlgorithm() {}
private:
	unsigned snippetSize;
	string highlightMarkerPre;
	string highlightMarkerPost;
protected:
	vector<keywordHighlightInfo>& keywordStrToHighlight;
	vector<PhraseInfoForHighLight> phrasesInfoList;
	boost::unordered_map<unsigned, unsigned>  positionToOffsetMap;
};

class AnalyzerBasedAlgorithm : public HighlightAlgorithm {
public:
	AnalyzerBasedAlgorithm(Analyzer *analyzer, QueryResults *queryResults,
			std::map<string, PhraseInfo>& phrasesInfoMap, const HighlightConfig& hconf);
	AnalyzerBasedAlgorithm(Analyzer *analyzer, vector<keywordHighlightInfo>& keywordStrToHighlight,
			vector<PhraseInfoForHighLight>& phrasesInfoList, const HighlightConfig& hconf);
	void getSnippet(unsigned recordId, unsigned attributeId, const string& dataIn, vector<string>& snippets,
			bool isMultiValued);
private:
	Analyzer *analyzer;
};

class Indexer;
class ForwardIndex;

class TermOffsetAlgorithm : public HighlightAlgorithm {
public:
	TermOffsetAlgorithm(const Indexer * indexer, QueryResults * queryResults,
			std::map<string, PhraseInfo>& phrasesInfoMap, const HighlightConfig& hconf);
	void getSnippet(unsigned recordId, unsigned attributeId, const string& dataIn, vector<string>& snippets, bool isMultiValued);
private:
	ForwardIndex* fwdIndex;
	typedef std::map<unsigned, vector<unsigned> *>::iterator PrefixToCompleteMapIter;
	std::map<unsigned, vector<unsigned> *>& keywordPrefixToCompleteMap;
	boost::unordered_map<unsigned, vector<unsigned> > keywordIdToOffsetsMap;
};

} /* namespace instanstsearch */
} /* namespace srch2 */
#endif /* HIGHLIGHTER_H_ */
