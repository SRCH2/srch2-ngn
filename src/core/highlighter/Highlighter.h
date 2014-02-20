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
	unsigned editDistance;
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
	vector<std::pair<string, string> > highlightMarkers;
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
			unsigned tagIndex;
		};
	HighlightAlgorithm ( std::map<string, PhraseInfo>& phrasesInfoMap, const HighlightConfig& hconfig);
	HighlightAlgorithm ( vector<PhraseInfoForHighLight>& phrasesInfoList, const HighlightConfig& hconfig);
	virtual void getSnippet(const QueryResults *qr,unsigned recIdx, unsigned attributeId, const string& dataIn,
			vector<string>& snippets, bool isMultiValued, vector<keywordHighlightInfo>& keywordStrToHighlight) = 0;

	void _genSnippet(const vector<CharType>& dataIn, vector<CharType>& snippets, unsigned snippetUpperEnd,
			unsigned snippetLowerEnd, const vector<matchedTermInfo>& highlightPositions);
	void insertHighlightMarkerIntoSnippets(vector<CharType>& snippets,
			const vector<CharType>& dataIn, unsigned lowerOffset, unsigned upperOffset,
			const vector<matchedTermInfo>& highlightPositions,
			unsigned snippetLowerEnd, unsigned snippetUpperEnd);
	void removeInvalidPositionInPlace(vector<matchedTermInfo>& highlightPositions);
	void genDefaultSnippet(const string& dataIn, vector<string>& snippets, bool isMultivalued);
	void setupPhrasePositionList(vector<keywordHighlightInfo>& keywordStrToHighlight);
	void buildKeywordHighlightInfo(const QueryResults * qr, unsigned recIdx,
			vector<keywordHighlightInfo>& keywordStrToHighlight);
	virtual ~HighlightAlgorithm() {}
private:
	unsigned snippetSize;
	vector<std::pair<string, string> > highlightMarkers; // holds marker for fuzzy or exact match
protected:
	std::map<string, PhraseInfo> phrasesInfoMap;
	vector<PhraseInfoForHighLight> phrasesInfoList;
	boost::unordered_map<unsigned, unsigned>  positionToOffsetMap;
};

class AnalyzerBasedAlgorithm : public HighlightAlgorithm {
public:
	AnalyzerBasedAlgorithm(Analyzer *analyzer,
			std::map<string, PhraseInfo>& phrasesInfoMap, const HighlightConfig& hconf);
	AnalyzerBasedAlgorithm(Analyzer *analyzer,
			vector<PhraseInfoForHighLight>& phrasesInfoList, const HighlightConfig& hconf);
	void getSnippet(const QueryResults *qr,unsigned recIdx, unsigned attributeId, const string& dataIn, vector<string>& snippets,
			bool isMultiValued, vector<keywordHighlightInfo>& keywordStrToHighlight);
private:
	Analyzer *analyzer;
};

class Indexer;
class ForwardIndex;

class TermOffsetAlgorithm : public HighlightAlgorithm {
	struct CandidateKeywordInfo{
			short prefixKeyIdx;
			unsigned keywordOffset;
			CandidateKeywordInfo(short arg1, unsigned arg2) {
				prefixKeyIdx = arg1; keywordOffset = arg2;
			}
		};
public:
	TermOffsetAlgorithm(const Indexer * indexer,
			std::map<string, PhraseInfo>& phrasesInfoMap, const HighlightConfig& hconf);
	void getSnippet(const QueryResults *qr,unsigned recIdx, unsigned attributeId, const string& dataIn,
			vector<string>& snippets, bool isMultiValued, vector<keywordHighlightInfo>& keywordStrToHighlight);
	~TermOffsetAlgorithm() {
		boost::unordered_map<unsigned, vector<CandidateKeywordInfo>*>::iterator iter = cache.begin();
		while (iter != cache.end()) {
			delete iter->second;
			++iter;
		}
	}
private:
	ForwardIndex* fwdIndex;
	boost::unordered_map<unsigned, vector<CandidateKeywordInfo>* > cache;
};

} /* namespace instanstsearch */
} /* namespace srch2 */
#endif /* HIGHLIGHTER_H_ */
