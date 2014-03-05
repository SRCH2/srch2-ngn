//$Id: $
/*
 * Highlighter.cpp
 *
 *  Created on: Jan 9, 2014
 *      Author: Surendra Bisht
 */

#include "Highlighter.h"
#include "operation/PhraseSearcher.h"
#include <set>
#include <boost/algorithm/string.hpp>
#include <instantsearch/Analyzer.h>
#include "util/Logger.h"
#include "operation/IndexerInternal.h"
#include "query/QueryResultsInternal.h"

using namespace srch2::util;

namespace srch2 {
namespace instantsearch {

template<class T>
bool compareVectors(vector<T> left, vector<T> right) {
	if (left.size() != right.size()) {
		return false;
	}
	return std::equal(left.begin(), left.end(), right.begin());
}

template<class T>
bool isPrefix(vector<T> left, vector<T> right) {
	typedef std::pair<typename vector<T>::iterator, typename vector<T>::iterator> MisMatchPositonType;
	if (left.size() > right.size()) {
		return false;
	}
	MisMatchPositonType mismatch = std::mismatch(left.begin(), left.end(), right.begin());
	return  mismatch.first == left.end();
}

bool isWhiteSpace(CharType c) {
	if(c == 32 /*whitespace*/ || c == 9 /*tab*/) {
		return true;
	}
	return false;
}

HighlightAlgorithm::HighlightAlgorithm(std::map<string, PhraseInfo>& phrasesInfoMap,
									const HighlightConfig& hconf) {
	this->phrasesInfoMap.swap(phrasesInfoMap);
	this->snippetSize = (hconf.snippetSize > MIN_SNIPPET_SIZE) ? hconf.snippetSize : MIN_SNIPPET_SIZE;
	this->highlightMarkers = hconf.highlightMarkers;
}

HighlightAlgorithm::HighlightAlgorithm(vector<PhraseInfoForHighLight>& phrasesInfoList,
		const HighlightConfig& hconf): phrasesInfoMap(std::map<string, PhraseInfo>()),
		 phrasesInfoList(phrasesInfoList){
	this->snippetSize = (hconf.snippetSize > MIN_SNIPPET_SIZE) ? hconf.snippetSize : MIN_SNIPPET_SIZE;
	this->highlightMarkers = hconf.highlightMarkers;
}

void HighlightAlgorithm::setupPhrasePositionList(vector<keywordHighlightInfo>& keywordStrToHighlight) {
	this->phrasesInfoList.clear();
	std::map<string, PhraseInfo>::iterator iter = phrasesInfoMap.begin();
	// The code below populates the phrasesInfoList vector which is used to find valid phrase
	// positions and offsets.
	while(iter != phrasesInfoMap.end()) {
		PhraseInfoForHighLight pifh;
		pifh.phraseKeyWords.resize(keywordStrToHighlight.size());
		for (unsigned i = 0; i < iter->second.phraseKeyWords.size(); ++i) {
			vector<CharType> temp;
			utf8StringToCharTypeVector(iter->second.phraseKeyWords[i], temp);
			for(unsigned k = 0; k < keywordStrToHighlight.size(); ++k) {
				if (keywordStrToHighlight[k].flag != HIGHLIGHT_KEYWORD_IS_PERFIX &&
						compareVectors(keywordStrToHighlight[k].key, temp)){
					if (keywordStrToHighlight[k].flag == HIGHLIGHT_KEYWORD_IS_COMPLETE)
						keywordStrToHighlight[k].flag = HIGHLIGHT_KEYWORD_IS_HYBRID; // word present in both phrase and normal query
					PhraseTermInfo pti;
					pti.recordPosition = new vector<unsigned>();  // to be filled later
					pti.queryPosition = iter->second.phraseKeywordPositionIndex[i];
					pifh.phraseKeyWords[k] = pti;
				}
			}
		}
		pifh.slop = iter->second.proximitySlop;
		this->phrasesInfoList.push_back(pifh);
		++iter;
	}
}
void _genDefaultSnippet(const string& dataIn, vector<string>& snippets, unsigned snippetSize);
void _genDefaultSnippet(const string& dataIn, vector<string>& snippets, unsigned snippetSize) {
	const char * snippetStart = dataIn.c_str();
	unsigned snippetOffset = snippetSize > dataIn.size() ? dataIn.size() - 1 : snippetSize;
	const char * snippetEnd = snippetStart + snippetOffset;
	if (snippetOffset == snippetSize)
		while(snippetEnd - snippetStart > snippetOffset - 10 && !isWhiteSpace(*snippetEnd))
			snippetEnd--;
	string buf;
	buf.reserve(snippetEnd - snippetStart + strlen("...") + 1);
	buf.insert(0, snippetStart, snippetEnd - snippetStart + 1);
	if (snippetOffset == snippetSize)
		buf.insert(buf.size(), "...");
	snippets.push_back(buf);
}
void HighlightAlgorithm::genDefaultSnippet(const string& dataIn, vector<string>& snippets, bool isMultivalued){
	if (isMultivalued) {
		const char * attrStartPos = dataIn.c_str();
		const char * lastPos = dataIn.c_str();
		while(1) {
			const char * attrEndPos = strstr(lastPos, " $$ ");
			if (attrEndPos == 0)
				break;
			_genDefaultSnippet(string(lastPos,attrEndPos), snippets, snippetSize);
			lastPos = attrEndPos + strlen(" $$ ");;
		}
		if (*lastPos != 0)
			_genDefaultSnippet(string(lastPos), snippets, snippetSize);
	} else {
		_genDefaultSnippet(dataIn, snippets, snippetSize);
	}
}

void HighlightAlgorithm::removeInvalidPositionInPlace(vector<matchedTermInfo>& highlightPositions){
	/*
	 *  highlightPositions vector contains sorted offset of valid prefix/complete matches. The below
	 *  logic does two things.
	 *  1. removes invalid position of phrase terms
	 *  	e.g given a phrase = "apple pie" and a document = "apple pie ......pie.....apple ...."
	 *  	later occurrences of apple/pie are invalid positions.
	 *  2. removes duplicate entries.
	 *  	For term offset based algorithm, there are instances where duplicate offset information
	 *  	creeps in. e.g. for matching prefixes= foo and food, leaf nodes of food are also leaf nodes
	 *  	of foo.
	 */
	unsigned writeIdx = 0;
	unsigned currIdx = 0;
	while (currIdx < highlightPositions.size()) {
		if (highlightPositions[currIdx].flag != HIGHLIGHT_KEYWORD_IS_PHRASE &&
			(writeIdx == 0 || highlightPositions[currIdx].offset != highlightPositions[writeIdx-1].offset)) {
			if (currIdx - writeIdx > 0) {
				highlightPositions[writeIdx] = highlightPositions[currIdx];
			}
			writeIdx++;
		}
		currIdx++;
	}
	highlightPositions.resize(writeIdx);
}

AnalyzerBasedAlgorithm::AnalyzerBasedAlgorithm(Analyzer *analyzer,
		std::map<string, PhraseInfo>& phrasesInfoMap,
		const HighlightConfig& hconf):
		HighlightAlgorithm(phrasesInfoMap, hconf){
		this->analyzer = analyzer;
}
AnalyzerBasedAlgorithm::AnalyzerBasedAlgorithm(Analyzer *analyzer,
		vector<PhraseInfoForHighLight>& phrasesInfoList,
		const HighlightConfig& hconf):
		HighlightAlgorithm(phrasesInfoList, hconf){
	this->analyzer = analyzer;
}

void AnalyzerBasedAlgorithm::getSnippet(const QueryResults* /*not used*/, unsigned /* not used*/,
		unsigned /*not used*/, const string& dataIn,
		vector<string>& snippets, bool isMultiValued, vector<keywordHighlightInfo>& keywordStrToHighlight) {

	if (dataIn.length() == 0)
		return;
	// One of the constructors of this class allows to pass phraseInfoList directly (used in ctest).
	// If the phraseInfoList is already present then do not re-calculate.
	if(phrasesInfoList.size() == 0)
		setupPhrasePositionList(keywordStrToHighlight);

	vector<matchedTermInfo> highlightPositions;
	set<unsigned> actualHighlightedSet;
	vector<CharType> ctsnippet;
	short mask = (1 << keywordStrToHighlight.size()) - 1;

	/*
	 *   1. Feed the attribute value to the analyzer
	 *   2. Loop while we get the tokens from the analyzer with position and offset information.
	 *   3. Compare each token with the candidate prefixes stored in keywordStrToHighlight vector.
	 *   	The vector contains 4 kinds of terms :
	 *   	prefix 0, Complete 1, Phrase 2, Hybrid (occurs in both phrase and regular query)  3
	 *   4. Store the term char offset in a vector. If it is a phrase term then also store the term
	 *      positions for phrase processing later.
	 */
	this->analyzer->fillInCharacters(dataIn.c_str());
	while (this->analyzer->processToken()) {
		vector<CharType>& charVector = this->analyzer->getProcessedToken();
		unsigned charOffset = this->analyzer->getProcessedTokenCharOffset();
		unsigned position = this->analyzer->getProcessedTokenPosition();
		bool matchFound = false;

		if (mask == 0 && phrasesInfoList.size() == 0)  // for phrase we cannot assume much.
			break;

		for (unsigned i =0; i < keywordStrToHighlight.size(); ++i) {
			switch (keywordStrToHighlight[i].flag) {
			case HIGHLIGHT_KEYWORD_IS_PERFIX:  // prefix
			{
				matchFound = isPrefix(keywordStrToHighlight[i].key, charVector);
				break;
			}
			case HIGHLIGHT_KEYWORD_IS_COMPLETE:
			case HIGHLIGHT_KEYWORD_IS_PHRASE:
			// added just to ignore compiler warning. This flag is computed later.
			case HIGHLIGHT_KEYWORD_IS_VERIFIED_PHRASE:
			case HIGHLIGHT_KEYWORD_IS_HYBRID:
			{
				matchFound = compareVectors(keywordStrToHighlight[i].key, charVector);
				break;
			}
			}
			if (matchFound) {
				matchedTermInfo info = {keywordStrToHighlight[i].flag, i, charOffset,
						keywordStrToHighlight[i].key.size(), 0};
				if (keywordStrToHighlight[i].editDistance > 0)
					info.tagIndex = 1;
			 	highlightPositions.push_back(info);
			 	if (keywordStrToHighlight[i].flag == HIGHLIGHT_KEYWORD_IS_PHRASE
			 			|| keywordStrToHighlight[i].flag == HIGHLIGHT_KEYWORD_IS_HYBRID) {
			 		/*
			 		 *   Go over all phrases and add position info for the matched keyword
			 		 */
			 		for (unsigned j = 0; j < phrasesInfoList.size(); ++j) {
			 			if (phrasesInfoList[j].phraseKeyWords[i].recordPosition)
			 				phrasesInfoList[j].phraseKeyWords[i].recordPosition->push_back(position);
			 		}
			 		positionToOffsetMap.insert(std::make_pair(position, highlightPositions.size()- 1));
			 	}
			 	actualHighlightedSet.insert(i);
			 	mask &= ~(1 << i);   // helps in early termination if all keywords are found
			 	break;
			}
		}
	}

	validatePhrasePositions(highlightPositions);
	// sweep out invalid positions
	removeInvalidPositionInPlace(highlightPositions);

	if (highlightPositions.size() == 0) {
		genDefaultSnippet(dataIn, snippets, isMultiValued);
		Logger::debug("could not generate a snippet because keywords could not be found in attribute.");
		return;
	}

	buildSnippetUsingHighlightPositions(dataIn, highlightPositions, actualHighlightedSet,
			ctsnippet, snippets, isMultiValued);

	// Clear the list so that the phrase info list is recalculated for next attribute/record.
	phrasesInfoList.clear();
}

/*
 *   The function first compute the valid phrase positions. Then it goes over the highlight positions
 *   computed earlier without the phrase consideration, and mark the correct phrase positions of
 *   the phrase terms only.
 *
 *   e.g  phrase "A B"
 *   highlight positions (term, position, flag):[ (A, 13, 2) (A, 18, 2) (B, 19, 2) (D, 33, 1)]
 *
 *   flag meaning : (see enum KeywordHighlightInfoFlag in header file)
 *   flag 1 --> exact term ( not in phrase)
 *   flag 2 --> phrase term position ( not validated)
 *   flag 4 --> phrase term position (validated)
 *
 *   validated positions: [ (A, 13, 2) (A, 18, 4) (B, 19, 4) (D, 33, 1)]
 *
 *   Now we know that the term A at position 13 does not make a valid phrase.
 */
void HighlightAlgorithm::validatePhrasePositions(vector<matchedTermInfo>& highlightPositions) {

	PhraseSearcher phraseSearcher;
	vector<vector<vector<unsigned> > > allPhrasesMatchedPositions;
	for (unsigned j = 0; j < phrasesInfoList.size(); ++j) {
		PhraseInfoForHighLight &phraseInfo = phrasesInfoList[j];
		vector<vector<unsigned> > positionListVector;
		vector<vector<unsigned> > matchedPositions;
		vector<unsigned> keyWordPosInPhrase;
		for ( unsigned k = 0 ; k < phraseInfo.phraseKeyWords.size(); ++k) {
			if (phraseInfo.phraseKeyWords[k].recordPosition) {
				positionListVector.push_back(*(phraseInfo.phraseKeyWords[k].recordPosition));
				keyWordPosInPhrase.push_back(phraseInfo.phraseKeyWords[k].queryPosition);
			}
		}
		if (phraseInfo.slop > 0) {
			phraseSearcher.proximityMatch(positionListVector, keyWordPosInPhrase, phraseInfo.slop,
					matchedPositions, false);
		} else {
			phraseSearcher.exactMatch(positionListVector, keyWordPosInPhrase,
					matchedPositions, false);
		}
		allPhrasesMatchedPositions.push_back(matchedPositions);
	}

	// mark the valid position in highlightPositions vector
	vector<unsigned> allPhrasesOffsetInData;
	for(unsigned i = 0; i < allPhrasesMatchedPositions.size(); ++i) {
		vector<vector<unsigned> >&  currPhraseMatchedPositions = allPhrasesMatchedPositions[i];
		for (unsigned j = 0; j < currPhraseMatchedPositions.size(); ++j) {
			vector<unsigned> &currPhraseMatchedPosition = currPhraseMatchedPositions[j];
			for (unsigned k = 0; k < currPhraseMatchedPosition.size(); ++k) {
				boost::unordered_map<unsigned, unsigned>::iterator iter =
						positionToOffsetMap.find(currPhraseMatchedPosition[k]);
				if (iter != positionToOffsetMap.end())
					highlightPositions[iter->second].flag = HIGHLIGHT_KEYWORD_IS_VERIFIED_PHRASE;
			}
		}
	}
}
/*
 *  Now that the highlight positions are computed, start snippet generation. For multi-value attributes
 *  break, generate snippet for each individual attributes.
 */
void HighlightAlgorithm::buildSnippetUsingHighlightPositions(const string& dataIn,
		vector<matchedTermInfo>& highlightPositions, set<unsigned>& visitedKeyword,
		vector<CharType>& ctsnippet, vector<string>& snippets, bool isMultiValued) {

	unsigned snippetLowerEnd = 0, snippetUpperEnd = highlightPositions.size() - 1;
	if (phrasesInfoList.size() == 0 && !isMultiValued) {
		unsigned j = snippetUpperEnd;
		while(j > 0){
			unsigned _id = highlightPositions[j].id;
			visitedKeyword.erase(_id);
			if (visitedKeyword.empty())
				break;
			--j;
		};
		snippetLowerEnd = j;
	}

	if (isMultiValued) {
			const char * attrStartPos = dataIn.c_str();
			const char * lastPos = dataIn.c_str();
			snippetLowerEnd = 0; snippetUpperEnd = 0;
			while(1) {
				ctsnippet.clear();
				const char * attrEndPos = strstr(lastPos, " $$ ");
				if (attrEndPos == 0)
					break;
				unsigned matchCntInAttr = 0;
				vector<matchedTermInfo> partHighlightPositions;
				while(snippetUpperEnd <  highlightPositions.size() &&
						highlightPositions[snippetUpperEnd].offset < (attrEndPos - attrStartPos)) {
					partHighlightPositions.push_back(highlightPositions[snippetUpperEnd]);
					partHighlightPositions.back().offset -=  (lastPos - attrStartPos);
					snippetUpperEnd++;
					matchCntInAttr++;
				}
				string attrPartVal= dataIn.substr(lastPos - attrStartPos /*offset*/, attrEndPos - lastPos /*len*/);
				if (matchCntInAttr) {
					vector<CharType> ctv;
					utf8StringToCharTypeVector(attrPartVal, ctv);
					_genSnippet(ctv, ctsnippet, partHighlightPositions.size() - 1, 0, partHighlightPositions);
					string snippet;
					charTypeVectorToUtf8String(ctsnippet, snippet);
					snippets.push_back(snippet);
				} else {
					genDefaultSnippet(attrPartVal, snippets, false);
				}

				lastPos = attrEndPos + strlen(" $$ ");
				snippetLowerEnd = snippetUpperEnd;
			}
			if (*lastPos != 0) {
				ctsnippet.clear();
				snippetUpperEnd = highlightPositions.size() - 1;
				string attrPartVal= dataIn.substr(lastPos - attrStartPos /*offset*/, string::npos /*len*/);
				if (snippetLowerEnd <  highlightPositions.size() &&
						highlightPositions[snippetLowerEnd].offset > (lastPos - attrStartPos)) {
					vector<CharType> ctv;
					utf8StringToCharTypeVector(attrPartVal, ctv);
					vector<matchedTermInfo> partHighlightPositions;
					vector<matchedTermInfo>::iterator phpIter = highlightPositions.begin() + snippetLowerEnd;
					while(phpIter != highlightPositions.end()){
						partHighlightPositions.push_back(*phpIter);
						partHighlightPositions.back().offset -=  (lastPos - attrStartPos);
						++phpIter;
					}
					_genSnippet(ctv, ctsnippet, partHighlightPositions.size() - 1, 0, partHighlightPositions);
					string snippet;
					charTypeVectorToUtf8String(ctsnippet, snippet);
					snippets.push_back(snippet);
				} else {
					genDefaultSnippet(attrPartVal, snippets, false);
				}
			}
		}else {
			vector<CharType> ctv;
			utf8StringToCharTypeVector(dataIn, ctv);
			if (ctv.size() == 0)
				return;
			_genSnippet(ctv, ctsnippet, snippetUpperEnd, snippetLowerEnd, highlightPositions);
			string snippet;
			charTypeVectorToUtf8String(ctsnippet, snippet);
			//cout << "DEBUG: snippet size is " << snippet.size() << endl;
			snippets.push_back(snippet);
		}
}

void HighlightAlgorithm::_genSnippet(const vector<CharType>& dataIn, vector<CharType>& snippets,
		unsigned snippetUpperEnd, unsigned snippetLowerEnd,
		const vector<matchedTermInfo>& highlightPositions) {

	ASSERT(highlightPositions.size() > 0);

	unsigned markerCharSize = 1.5 * (snippetUpperEnd - snippetLowerEnd) *
						  (this->highlightMarkers[0].first.size() + this->highlightMarkers[0].first.size());
	snippets.reserve(this->snippetSize + markerCharSize + 6);
	unsigned maxSnippetLen = this->snippetSize;

	vector<CharType> filler;
	utf8StringToCharTypeVector("...", filler);

	unsigned lowerOffset = highlightPositions[snippetLowerEnd].offset - 1;
	unsigned upperOffset = highlightPositions[snippetUpperEnd].offset +
			highlightPositions[snippetUpperEnd].len - 1;
	if (upperOffset > dataIn.size() || lowerOffset > upperOffset) {
		return;
	}
	/*
	 * Move upper offset to find the word boundary. If no word boundary is found within 20 characters
	 * then we stop
	 */
	unsigned tempOffset = upperOffset;
	while(tempOffset < (upperOffset + 20) && tempOffset < dataIn.size()  &&
			!isWhiteSpace(dataIn[tempOffset])) {
		tempOffset++;
	}
	upperOffset = tempOffset;

	unsigned quotaRemaining = maxSnippetLen;
	unsigned gap = upperOffset - lowerOffset;

	if (gap < quotaRemaining) {
		unsigned leftPad, rightPad;
		// Because we have all highlighting position with snippet limit
		// TODO: try to look for sentence boundary to generate more meaningful snippets.

		leftPad = rightPad = (quotaRemaining - gap) / 2;
		if (upperOffset + rightPad >= dataIn.size()) {
			/*branch1*/
			leftPad += rightPad - (dataIn.size() - upperOffset);  // add remaining to leftPad
			upperOffset = dataIn.size();  // set the upper offset to the size of data
		} else {
			/*branch2*/
			tempOffset = upperOffset + rightPad;
			while(tempOffset > upperOffset && !isWhiteSpace(dataIn[tempOffset - 1])){
				tempOffset--;
			}
			upperOffset = tempOffset;
		}
		if (lowerOffset - leftPad > lowerOffset /*unsigned value will wrap around*/) {
			/*branch3*/
			rightPad = leftPad - lowerOffset;
		    lowerOffset = 0;
		} else {
			/*branch4*/
			rightPad = 0;
			tempOffset = lowerOffset - leftPad;
			while(tempOffset < lowerOffset && !isWhiteSpace(dataIn[tempOffset])){
				tempOffset++;
			}
			lowerOffset = tempOffset;
		}
		// following branch will get executed only if branch 2 and branch 3 were taken above,
		if (upperOffset < dataIn.size() && rightPad > 0) {
			if (upperOffset + rightPad >= dataIn.size()) {
				upperOffset = dataIn.size();  // set the upper offset to the size of data
			} else {
				tempOffset = upperOffset + rightPad;
				while(tempOffset > upperOffset && !isWhiteSpace(dataIn[tempOffset - 1])){
					tempOffset--;
				}
				upperOffset = tempOffset;
			}
		}
		if (lowerOffset != 0) {
			snippets.insert(snippets.end(), filler.begin(), filler.end());
		}
		insertHighlightMarkerIntoSnippets(snippets, dataIn, lowerOffset, upperOffset,
				highlightPositions, snippetLowerEnd, snippetUpperEnd);
	} else {
		// find maximum interval between the offsets of matched keywords.
		unsigned index = snippetLowerEnd;
		signed intervalGap = 0;  // keep this signed to check for negative value.
		unsigned extraChars = gap - quotaRemaining;
		bool snippetShortened = false;
		vector<std::pair<unsigned, unsigned> > intervalVect;
		while(index < snippetUpperEnd) {
			intervalGap = highlightPositions[index + 1].offset -
					(highlightPositions[index].offset + highlightPositions[index].len);
			if (intervalGap > (signed)extraChars) {
				unsigned currentIndexOffset =
						highlightPositions[index].offset + highlightPositions[index].len;
				unsigned intermediateOffset =
						currentIndexOffset  + ((intervalGap - extraChars) / 2);
				while(intermediateOffset > currentIndexOffset
						&& !isWhiteSpace(dataIn[intermediateOffset - 1])){
					intermediateOffset--;
				}
				insertHighlightMarkerIntoSnippets(snippets, dataIn, lowerOffset, intermediateOffset,
								highlightPositions, snippetLowerEnd, snippetUpperEnd);
				snippets.insert(snippets.end(), filler.begin(), filler.end()); // add "..."

				unsigned nextIndexOffset = highlightPositions[index + 1].offset;
						//+ highlightPositions[index + 1].len;
				intermediateOffset = nextIndexOffset  - ((intervalGap - extraChars) / 2);
				while(intermediateOffset < nextIndexOffset
						&& !isWhiteSpace(dataIn[intermediateOffset - 1])){
					intermediateOffset++;
				}

				insertHighlightMarkerIntoSnippets(snippets, dataIn, intermediateOffset, upperOffset,
												highlightPositions, snippetLowerEnd, snippetUpperEnd);
				snippetShortened = true;
				break;
			}
			else {
				if (intervalGap < 0) intervalGap = 0; // negative interval does not make much sense
				intervalVect.push_back(std::make_pair(intervalGap, index));
			}
			++index;
		}

		if (!snippetShortened && intervalVect.size() > 0){
			index = snippetLowerEnd;
			sort(intervalVect.begin(), intervalVect.end());
			unsigned sum = 0;
			unsigned idx = intervalVect.size() - 1;
			while(sum  < extraChars && idx < intervalVect.size()) {
				sum += intervalVect[idx].first;
				idx--;
			}
			if (sum > extraChars) {
				unsigned cushion = (sum - extraChars) / ((signed)intervalVect.size() - (signed)idx - 1);
				unsigned cutOffinterval = intervalVect[(signed)idx+1].first;
				unsigned startOffset = 0;
				unsigned endOffset = 0;

				while(index < snippetUpperEnd) {
					intervalGap = highlightPositions[index + 1].offset -
							(highlightPositions[index].offset + highlightPositions[index].len);
					startOffset = highlightPositions[index].offset - 1;
					if (intervalGap >= (signed)cutOffinterval){
						endOffset =  highlightPositions[index].offset + highlightPositions[index].len;
						tempOffset = highlightPositions[index].offset + highlightPositions[index].len +
								cushion / 2;
						while(tempOffset  > endOffset && !isWhiteSpace(dataIn[tempOffset - 1])){
							tempOffset--;
						}
						endOffset = tempOffset;
						insertHighlightMarkerIntoSnippets(snippets, dataIn, startOffset, endOffset,
								highlightPositions, snippetLowerEnd, snippetUpperEnd);

						snippets.insert(snippets.end(), filler.begin(), filler.end()); // add "..."


						startOffset = highlightPositions[index + 1].offset - 1 - (cushion / 2);
						endOffset = highlightPositions[index + 1].offset - 1;
						while(startOffset < endOffset
								&& !isWhiteSpace(dataIn[startOffset])){
							startOffset++;
						}
						insertHighlightMarkerIntoSnippets(snippets, dataIn, startOffset, endOffset,
								highlightPositions, snippetLowerEnd, snippetUpperEnd);
					} else {
						endOffset = highlightPositions[index + 1].offset - 1;
						insertHighlightMarkerIntoSnippets(snippets, dataIn, startOffset, endOffset,
								highlightPositions, snippetLowerEnd, snippetUpperEnd);
					}
					++index;
				}
				startOffset = highlightPositions[index].offset - 1;
				insertHighlightMarkerIntoSnippets(snippets, dataIn, startOffset, upperOffset,
						highlightPositions, snippetLowerEnd, snippetUpperEnd);
			}
		}
	}
	if (upperOffset < dataIn.size() - 1) {
		snippets.insert(snippets.end(), filler.begin(), filler.end());
	}
}

void HighlightAlgorithm::insertHighlightMarkerIntoSnippets(vector<CharType>& snippets,
		const vector<CharType>& dataIn, unsigned lowerOffset, unsigned upperOffset,
		const vector<matchedTermInfo>& highlightPositions,
		unsigned snippetLowerEnd, unsigned snippetUpperEnd) {

	unsigned copyStartOffset = lowerOffset;
	for (unsigned i =  snippetLowerEnd; i <= snippetUpperEnd; ++i) {
		if (highlightPositions[i].offset < lowerOffset)
			continue;
		if (highlightPositions[i].offset > upperOffset)
			break;
		snippets.insert(snippets.end(), dataIn.begin() + copyStartOffset, dataIn.begin() + highlightPositions[i].offset - 1);
		copyStartOffset = highlightPositions[i].offset - 1;
		unsigned index = highlightPositions[i].tagIndex;
		snippets.insert(snippets.end(), this->highlightMarkers[index].first.begin(), this->highlightMarkers[index].first.end());
		snippets.insert(snippets.end(), dataIn.begin() + copyStartOffset, dataIn.begin() + copyStartOffset + highlightPositions[i].len);
		snippets.insert(snippets.end(), this->highlightMarkers[index].second.begin(), this->highlightMarkers[index].second.end());
		copyStartOffset += highlightPositions[i].len;
	}
	if (copyStartOffset < upperOffset)
		snippets.insert(snippets.end(), dataIn.begin() + copyStartOffset, dataIn.begin() + upperOffset);
}

bool operator < (HighlightAlgorithm::matchedTermInfo l, HighlightAlgorithm::matchedTermInfo r){
	if (l.offset < r.offset)
		return true;
	else
		return false;
}

TermOffsetAlgorithm::TermOffsetAlgorithm (const Indexer * indexer,
		std::map<string, PhraseInfo>& phrasesInfoMap, const HighlightConfig& hconf):
				HighlightAlgorithm(phrasesInfoMap, hconf){

	const IndexReaderWriter* rwIndexer =  dynamic_cast<const IndexReaderWriter *>(indexer);
	fwdIndex = rwIndexer->getForwardIndex();
	fwdIndex->getForwardListDirectory_ReadView(readView);

}


/*
 *  Function:  findMatchingKeywordsFromPrefixNode
 *  This function uses the leftmost and rightmost descendants' ids of a node in the trie rooted at
 *  prefixNode to probe the sorted keyword ids in the forward list. The algorithm traverses the trie
 *  in a breadth first search manner. Each Id is probed using a binary search.
 */
void TermOffsetAlgorithm::findMatchingKeywordsFromPrefixNode(const TrieNode* prefixNode, unsigned prefixIndex,
		vector<CandidateKeywordInfo>& completeKeywordsId,
		const unsigned *keywordIdsPtr, unsigned keywordsInRec){

	unsigned low = 0, high = keywordsInRec;

	vector<vector<const TrieNode*> > workingSets= vector<vector<const TrieNode*> >(2);
	unsigned currSetIdx = 0;
	unsigned nextSetIdx =  1;

	workingSets[currSetIdx].push_back(prefixNode);
	while(workingSets[currSetIdx].size()) {
		vector<const TrieNode*>& currentSet = workingSets[currSetIdx];
		vector<const TrieNode*>& nextSet = workingSets[nextSetIdx];
		unsigned lowerOffset = 0, higherOffset = 0;
		for(unsigned i = 0; i < currentSet.size(); ++i) {
			const TrieNode* currNode = currentSet[i];
			// Check leftmost descendant first.
			const unsigned * iter = std::lower_bound(keywordIdsPtr + low , keywordIdsPtr + high, currNode->leftMostDescendant->id);

			// if below criterion is met then neither current node's children nor its sibling will
			// be in the keyword list. So break from here.
			if (iter >= keywordIdsPtr + high){
				break;
			}
			lowerOffset = std::distance(keywordIdsPtr, iter);

			if (*iter == currNode->leftMostDescendant->id) {
				completeKeywordsId.push_back(CandidateKeywordInfo(prefixIndex, lowerOffset));
				lowerOffset++;
			}

			// only move the lower bound for the left most node in the working set
			if ( i == 0)
				low = lowerOffset;

			if (lowerOffset >= high)
				break;

			iter = std::lower_bound(keywordIdsPtr + lowerOffset , keywordIdsPtr + high, currNode->rightMostDescendant->id);
			higherOffset = std::distance(keywordIdsPtr, iter);

			// If the below condition is not met then the current sub-trie node cannot have any matching
			// keywords. Just skip the sub-trie.
			if (higherOffset > lowerOffset) {
				// store the children to next working set.
				for (unsigned j = 0; j < currNode->getChildrenCount(); ++j){
					nextSet.push_back(currNode->getChild(j));
				}
			}

			// If all the keywords are less than the rightmost descendant node then do not go further
			// on current working list. Skip the siblings and their children.
			if (higherOffset >= high)
				break;

			// shrink high end mark for the right most node.
			if (i == currentSet.size() - 1)
				high = higherOffset;

			if (*iter == currNode->rightMostDescendant->id) {
				completeKeywordsId.push_back(CandidateKeywordInfo(prefixIndex, higherOffset));
			}
		}
		if (low >= high)
			return;
		currentSet.clear();
		std::swap(nextSetIdx, currSetIdx);
	}
}

void TermOffsetAlgorithm::getSnippet(const QueryResults* qr, unsigned recidx, unsigned attributeId,
		const string& dataIn, vector<string>& snippets, bool isMultiValued,
		vector<keywordHighlightInfo>& keywordStrToHighlight) {

	if (dataIn.length() == 0)
		return;
	unsigned recordId = qr->getInternalRecordId(recidx);
	bool valid = false;
	const ForwardList * fwdList = fwdIndex->getForwardList(readView, recordId, valid);
	if (!valid) {
		Logger::error("Invalid forward list for record id = %d", recordId);
		return;
	}
	if (fwdList->getKeywordAttributeBitmaps() == 0){
		Logger::warn("Attribute info not found in forward List!!");
		return;
	}
	setupPhrasePositionList(keywordStrToHighlight);
	vector<matchedTermInfo> highlightPositions;
	vector<CharType> ctsnippet;

	set<unsigned> visitedKeyword;
	const unsigned *keywordIdsPtr = fwdList->getKeywordIds();
	unsigned keywordsInRec =  fwdList->getNumberOfKeywords();
	vector<CandidateKeywordInfo> *candidateKeywordsId;
	boost::unordered_map<unsigned, vector<CandidateKeywordInfo>*>::iterator iter = cache.find(recidx);
	/*
	 *  The get snippet code is called x number of times per record. Where x is number of attributes
	 *  to highlight. We compute the candidate kewordIds once for each record and then use it for all the
	 *  attributes.
	 */
	if (iter == cache.end()) {
		candidateKeywordsId = new vector<CandidateKeywordInfo>;
		candidateKeywordsId->reserve(1000);
		vector< const TrieNode* > & prefixNodes = qr->impl->sortedFinalResults[recidx]->matchingKeywordTrieNodes;
		for(unsigned indx = 0; indx < prefixNodes.size(); ++indx) {
			if (qr->impl->sortedFinalResults[recidx]->termTypes.at(indx) != TERM_TYPE_PREFIX &&
			    qr->impl->sortedFinalResults[recidx]->matchingKeywordTrieNodes[indx]->isTerminalNode()) {
				unsigned id = qr->impl->sortedFinalResults[recidx]->matchingKeywordTrieNodes[indx]->id;
				const unsigned * iter = lower_bound(keywordIdsPtr, keywordIdsPtr + keywordsInRec - 1, id);
				unsigned matchOffset = std::distance(keywordIdsPtr, iter);
				if (matchOffset < keywordsInRec) {
					candidateKeywordsId->push_back(CandidateKeywordInfo(indx, matchOffset));
				}
				else{
					ASSERT(true);
				}
			} else {
				findMatchingKeywordsFromPrefixNode(prefixNodes[indx], indx, *candidateKeywordsId, keywordIdsPtr, keywordsInRec);
			}
		}
		cache.insert(make_pair(recidx, candidateKeywordsId));
	} else {
		candidateKeywordsId = iter->second;
	}
	for (unsigned i = 0; i < candidateKeywordsId->size(); ++i) {
		CandidateKeywordInfo info = (*candidateKeywordsId)[i];
		unsigned attributeBitMap =	fwdList->getKeywordAttributeBitmap(info.keywordOffset);
		if (attributeBitMap & (1 << attributeId)) {
			vector<unsigned> offsetPosition;
			vector<unsigned> wordPosition;
			fwdList->getKeyWordOffsetInRecordField(info.keywordOffset, attributeId, attributeBitMap, offsetPosition);
			visitedKeyword.insert(info.prefixKeyIdx);
			for (unsigned _idx = 0; _idx < offsetPosition.size(); ++_idx){
				matchedTermInfo mti = {keywordStrToHighlight[info.prefixKeyIdx].flag,
						info.prefixKeyIdx, offsetPosition[_idx],
						keywordStrToHighlight[info.prefixKeyIdx].key.size(), 0};
				if (keywordStrToHighlight[info.prefixKeyIdx].editDistance > 0)
					mti.tagIndex = 1;
				highlightPositions.push_back(mti);
			}
			if (phrasesInfoList.size() > 0) {
				fwdList->getKeyWordPostionsInRecordField(info.keywordOffset, attributeId, attributeBitMap, wordPosition);
				for(unsigned pidx = 0 ; pidx < phrasesInfoList.size(); ++pidx) {
					if (phrasesInfoList[pidx].phraseKeyWords[info.prefixKeyIdx].recordPosition) {
						phrasesInfoList[pidx].phraseKeyWords[info.prefixKeyIdx].recordPosition->
						assign(wordPosition.begin(), wordPosition.end());
					}
				}
				for (unsigned _idx = 0; _idx < wordPosition.size(); ++_idx){
					positionToOffsetMap.insert(make_pair(wordPosition[_idx], highlightPositions.size() - offsetPosition.size() + _idx));
				}
			}
		}
	}

	validatePhrasePositions(highlightPositions);

	std::sort(highlightPositions.begin(), highlightPositions.end());

	// sweep out invalid positions
	removeInvalidPositionInPlace(highlightPositions);

	if (highlightPositions.size() == 0) {
		genDefaultSnippet(dataIn, snippets, isMultiValued);
		Logger::debug("could not generate a snippet because keywords could not be found in attribute.");
		return;
	}

	buildSnippetUsingHighlightPositions(dataIn, highlightPositions, visitedKeyword,
				ctsnippet, snippets, isMultiValued);

}

} /* namespace instanstsearch */
} /* namespace srch2 */
