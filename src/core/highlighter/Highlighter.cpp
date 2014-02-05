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

HighlightAlgorithm::HighlightAlgorithm(vector<keywordHighlightInfo>& keywordStrToHighlight,
									std::map<string, PhraseInfo>& phrasesInfoMap,
									const HighlightConfig& hconf) :
				keywordStrToHighlight(keywordStrToHighlight){
	std::map<string, PhraseInfo>::iterator iter = phrasesInfoMap.begin();
	// The code below populates the phrasesInfoList vector which is used to find valid phrase
	// positions and offsets.
	while(iter != phrasesInfoMap.end()) {
		PhraseInfoForHighLight pifh;
		pifh.phraseKeyWords.resize(this->keywordStrToHighlight.size());
		for (unsigned i = 0; i < iter->second.phraseKeyWords.size(); ++i) {
			vector<CharType> temp;
			utf8StringToCharTypeVector(iter->second.phraseKeyWords[i], temp);
			for(unsigned k = 0; k < this->keywordStrToHighlight.size(); ++k) {
				if (this->keywordStrToHighlight[k].flag == 1 &&
					compareVectors(this->keywordStrToHighlight[k].key, temp)){
					this->keywordStrToHighlight[k].flag = 2;
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
	this->snippetSize = (hconf.snippetSize > MIN_SNIPPET_SIZE) ? hconf.snippetSize : MIN_SNIPPET_SIZE;
	this->highlightMarkerPre = hconf.highlightMarkerPre;
	this->highlightMarkerPost = hconf.highlightMarkerPost;
}

HighlightAlgorithm::HighlightAlgorithm(vector<keywordHighlightInfo>& keywordStrToHighlight,
		vector<PhraseInfoForHighLight>& phrasesInfoList, const HighlightConfig& hconf):
		keywordStrToHighlight(keywordStrToHighlight), phrasesInfoList(phrasesInfoList){
	this->snippetSize = (hconf.snippetSize > MIN_SNIPPET_SIZE) ? hconf.snippetSize : MIN_SNIPPET_SIZE;
	this->highlightMarkerPre = hconf.highlightMarkerPre;
	this->highlightMarkerPost = hconf.highlightMarkerPost;
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
		if ( writeIdx == 0 || (highlightPositions[currIdx].flag != 2 &&
				highlightPositions[currIdx].offset != highlightPositions[writeIdx-1].offset)) {
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
		QueryResults *queryResults, std::map<string, PhraseInfo>& phrasesInfoMap,
		const HighlightConfig& hconf):
		HighlightAlgorithm(queryResults->impl->keywordStrToHighlight, phrasesInfoMap, hconf){
		this->analyzer = analyzer;
}
AnalyzerBasedAlgorithm::AnalyzerBasedAlgorithm(Analyzer *analyzer,
		vector<keywordHighlightInfo>& keywordStrToHighlight, vector<PhraseInfoForHighLight>& phrasesInfoList,
		const HighlightConfig& hconf):
		HighlightAlgorithm(keywordStrToHighlight, phrasesInfoList, hconf){
	this->analyzer = analyzer;
}

void AnalyzerBasedAlgorithm::getSnippet(unsigned recordId, unsigned /*not used*/, const char *dataIn, vector<string>& snippets) {

	vector<matchedTermInfo> highlightPositions;
	set<unsigned> actualHighlightedSet;
	vector<CharType> ctsnippet;
	vector<CharType> ctv;
	utf8StringToCharTypeVector(dataIn, ctv);
	short mask = (1 << keywordStrToHighlight.size()) - 1;

	/*
	 *   1. Feed the attribute value to the analyzer
	 *   2. Loop while we get the tokens from the analyzer with position and offset information.
	 *   3. Compare each token with the candidate prefixes stored in keywordStrToHighlight vector.
	 *   	The vector contains 4 kinds of terms :
	 *   	prefix 0, Complete 1, Phrase 2, Hybrid (occurs in both phrase and regular query)  3
	 *   4.
	 */
	this->analyzer->fillInCharacters(dataIn);
	while (this->analyzer->processToken()) {
		vector<CharType>& charVector = this->analyzer->getProcessedToken();
		unsigned offset = this->analyzer->getProcessedTokenOffset();
		unsigned position = this->analyzer->getProcessedTokenPosition();
		bool result = false;

		if (mask == 0 && phrasesInfoList.size() == 0)  // for phrase we cannot assume much.
			break;

		for (unsigned i =0; i < keywordStrToHighlight.size(); ++i) {
			switch (keywordStrToHighlight[i].flag) {
			case 0:  // prefix
			{
				result = isPrefix(keywordStrToHighlight[i].key, charVector);
				break;
			}
			case 1:
			case 2:
			case 3:
			{
				result = compareVectors(keywordStrToHighlight[i].key, charVector);
				break;
			}
			}
			if (result) {
				matchedTermInfo info = {keywordStrToHighlight[i].flag, i, offset,
						keywordStrToHighlight[i].key.size()};
			 	highlightPositions.push_back(info);
			 	if (keywordStrToHighlight[i].flag == 2 || keywordStrToHighlight[i].flag == 3) {
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

	/*
	 *   Now for the phrases, find the valid positions.
	 */
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
					highlightPositions[iter->second].flag = 4;
			}
		}
	}

	// sweep out invalid positions
	removeInvalidPositionInPlace(highlightPositions);

	if (highlightPositions.size() == 0) {
		Logger::debug("could not generate a snippet because keywords could not be found in attribute.");
		return;
	}

	unsigned snippetLowerEnd = 0, snippetUpperEnd = highlightPositions.size() - 1;
	/* no phrase for highlighting */
	if (phrasesInfoList.size() == 0 ) {
		unsigned j = snippetUpperEnd;
		while(j > 0){
			unsigned _id = highlightPositions[j].id;
			actualHighlightedSet.erase(_id);
			if (actualHighlightedSet.empty())
				break;
			--j;
		};
		snippetLowerEnd = j;
	}

	_genSnippet(ctv, ctsnippet, snippetUpperEnd, snippetLowerEnd, highlightPositions);
	string snippet;
	charTypeVectorToUtf8String(ctsnippet, snippet);
	//cout << "DEBUG: snippet size is " << snippet.size() << endl;
	snippets.push_back(snippet);
}

void HighlightAlgorithm::_genSnippet(const vector<CharType>& dataIn, vector<CharType>& snippets,
		unsigned snippetUpperEnd, unsigned snippetLowerEnd,
		const vector<matchedTermInfo>& highlightPositions) {

	snippets.reserve(this->snippetSize);
	unsigned maxSnippetLen = this->snippetSize;

	vector<CharType> filler;
	utf8StringToCharTypeVector("...", filler);

	ASSERT(highlightPositions.size() > 0);

	if (highlightPositions[0].offset != 0) {
		snippets.insert(snippets.end(), filler.begin(), filler.end());
	}
	unsigned quotaRemaining = maxSnippetLen - snippets.size() - filler.size() /*end filler*/;
	quotaRemaining -= (snippetUpperEnd - snippetLowerEnd) *
					  (this->highlightMarkerPre.size() + this->highlightMarkerPost.size());

	unsigned lowerOffset = highlightPositions[snippetLowerEnd].offset;
	unsigned upperOffset = highlightPositions[snippetUpperEnd].offset +
			highlightPositions[snippetUpperEnd].len;
	while(upperOffset < dataIn.size() && !isWhiteSpace(dataIn[upperOffset - 1])) {
		upperOffset++;
	}
	unsigned gap = upperOffset - lowerOffset;;

	if (gap < quotaRemaining) {
		unsigned leftPad, rightPad;
		leftPad = rightPad = (quotaRemaining - gap) / 2;
		if (upperOffset + rightPad >= dataIn.size()) {
			/*branch1*/
			leftPad += rightPad - (dataIn.size() - upperOffset);  // add remaining to leftPad
			upperOffset = dataIn.size() - 1;  // set the upper offset to the size of data
		} else {
			/*branch2*/
			upperOffset = upperOffset + rightPad;
			while(upperOffset > 0 /*defensive check*/ && !isWhiteSpace(dataIn[upperOffset - 1])){
				upperOffset--;
			}
		}
		if (lowerOffset - leftPad - 1 > lowerOffset /*unsigned value will wrap around*/) {
			/*branch3*/
			rightPad = leftPad - lowerOffset;
		    lowerOffset = 1;
		} else {
			/*branch4*/
			rightPad = 0;
			lowerOffset = lowerOffset - leftPad;
			while(lowerOffset < dataIn.size() /*defensive check*/
					&& !isWhiteSpace(dataIn[lowerOffset - 1])){
				lowerOffset++;
			}
		}
		// following branch will get executed only if branch 2 and branch 3 were taken above,
		if (upperOffset < dataIn.size() - 1 && rightPad > 0) {
			if (upperOffset + rightPad >= dataIn.size()) {
				upperOffset = dataIn.size() - 1;  // set the upper offset to the size of data
			} else {
				upperOffset = upperOffset + rightPad;
				while(upperOffset > 0 /*defensive check*/ && !isWhiteSpace(dataIn[upperOffset - 1])){
					upperOffset--;
				}
			}
		}
		insertHighlightMarkerIntoSnippets(snippets, dataIn, lowerOffset, upperOffset,
				highlightPositions, snippetLowerEnd, snippetUpperEnd);
	} else {
		// find maximum interval between the offsets of matched keywords.
		unsigned index = snippetLowerEnd;
		signed intervalGap = 0;  // keep this signed to check for negative value.
		unsigned extraChars = gap - quotaRemaining + 3;
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

		if (!snippetShortened){
			index = snippetLowerEnd;
			sort(intervalVect.begin(), intervalVect.end());
			unsigned sum = 0;
			unsigned idx = intervalVect.size() - 1;
			while(sum  < extraChars && idx < intervalVect.size()) {
				sum += intervalVect[idx].first;
				idx--;
			}
			unsigned cushion = (sum - extraChars) / (intervalVect.size() - idx - 1);
			unsigned cutOffinterval = intervalVect[idx+1].first;
			unsigned startOffset = lowerOffset;
			unsigned endOffset = 0;
			while(index < snippetUpperEnd) {
				intervalGap = highlightPositions[index + 1].offset -
									(highlightPositions[index].offset + highlightPositions[index].len);
				startOffset = highlightPositions[index].offset;
				if (intervalGap >= (signed)cutOffinterval){
					endOffset = highlightPositions[index].offset + highlightPositions[index].len +
								cushion / 2;
					while(endOffset > startOffset + highlightPositions[index].len
							&& !isWhiteSpace(dataIn[endOffset - 1])){
						endOffset--;
					}
					insertHighlightMarkerIntoSnippets(snippets, dataIn, startOffset, endOffset,
													highlightPositions, snippetLowerEnd, snippetUpperEnd);

					snippets.insert(snippets.end(), filler.begin(), filler.end()); // add "..."


					startOffset = highlightPositions[index + 1].offset - (cushion / 2);
					endOffset = highlightPositions[index + 1].offset - 1;
					while(startOffset < endOffset
							&& !isWhiteSpace(dataIn[startOffset - 1])){
						startOffset++;
					}
					insertHighlightMarkerIntoSnippets(snippets, dataIn, startOffset, endOffset,
													highlightPositions, snippetLowerEnd, snippetUpperEnd);
				} else {
					endOffset = highlightPositions[index + 1].offset - 1;
					if (startOffset < endOffset)
					insertHighlightMarkerIntoSnippets(snippets, dataIn, startOffset, endOffset,
												highlightPositions, snippetLowerEnd, snippetUpperEnd);
				}
				++index;
			}
			startOffset = highlightPositions[index].offset;
			insertHighlightMarkerIntoSnippets(snippets, dataIn, startOffset, upperOffset,
					highlightPositions, snippetLowerEnd, snippetUpperEnd);
		}
	}
	snippets.insert(snippets.end(), filler.begin(), filler.end());
}

void HighlightAlgorithm::insertHighlightMarkerIntoSnippets(vector<CharType>& snippets,
		const vector<CharType>& dataIn, unsigned lowerOffset, unsigned upperOffset,
		const vector<matchedTermInfo>& highlightPositions,
		unsigned snippetLowerEnd, unsigned snippetUpperEnd) {

	unsigned copyStartOffset = lowerOffset - 1;
	for (unsigned i =  snippetLowerEnd; i <= snippetUpperEnd; ++i) {
		if (highlightPositions[i].offset < lowerOffset)
			continue;
		if (highlightPositions[i].offset > upperOffset)
			break;
		snippets.insert(snippets.end(), dataIn.begin() + copyStartOffset, dataIn.begin() + highlightPositions[i].offset - 1);
		copyStartOffset = highlightPositions[i].offset - 1;
		snippets.insert(snippets.end(), this->highlightMarkerPre.begin(), this->highlightMarkerPre.end());
		snippets.insert(snippets.end(), dataIn.begin() + copyStartOffset, dataIn.begin() + copyStartOffset + highlightPositions[i].len);
		snippets.insert(snippets.end(), this->highlightMarkerPost.begin(), this->highlightMarkerPost.end());
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

TermOffsetAlgorithm::TermOffsetAlgorithm(const Indexer * indexer,
		QueryResults *queryResults,
		std::map<string, PhraseInfo>& phrasesInfoMap,const HighlightConfig& hconf):
				HighlightAlgorithm(queryResults->impl->keywordStrToHighlight, phrasesInfoMap, hconf),
				keywordPrefixToCompleteMap(queryResults->impl->prefixToCompleteMap){

	const IndexReaderWriter* rwIndexer =  dynamic_cast<const IndexReaderWriter *>(indexer);
	fwdIndex = rwIndexer->getForwardIndex();
}
void TermOffsetAlgorithm::getSnippet(unsigned recordId, unsigned attributeId, const char *dataIn,
		vector<string>& snippets) {
	bool valid = false;
	const ForwardList * fwdList = fwdIndex->getForwardList(recordId, valid);
	if (!valid) {
		Logger::error("Invalid forward list for record id = %d", recordId);
		return;
	}
	if (fwdList->getKeywordAttributeBitmaps() == 0){
		Logger::warn("Attribute info not found in forward List!!");
		return;
	}

	vector<matchedTermInfo> highlightPositions;
	vector<CharType> ctsnippet;
	vector<CharType> ctv;
	utf8StringToCharTypeVector(dataIn, ctv);

	const unsigned *keywordIdsPtr = fwdList->getKeywordIds();
	unsigned keywordsInRec =  fwdList->getNumberOfKeywords();

	for(PrefixToCompleteMapIter iter = keywordPrefixToCompleteMap.begin();
			iter != keywordPrefixToCompleteMap.end(); ++iter) {
		unsigned i = 0, j = 0;
		vector<unsigned>& keywordIds = *(iter->second);
		while(i < keywordIds.size() && j < keywordsInRec) {
			if (keywordIds[i] > keywordIdsPtr[j])
				++j;
			else if (keywordIds[i] < keywordIdsPtr[j])
				++i;
			else {
				unsigned attributeBitMap =	fwdList->getKeywordAttributeBitmap(j);
				if (attributeBitMap && (1 << attributeId)) {
					vector<unsigned> offsetPosition;
					vector<unsigned> wordPosition;
					fwdList->getKeyWordOffsetInRecordField(j, attributeId, attributeBitMap, offsetPosition);
					for (unsigned _idx = 0; _idx < offsetPosition.size(); ++_idx){
						// todo replace iter->first with keywordID
						matchedTermInfo mti = {keywordStrToHighlight[iter->first].flag, iter->first, offsetPosition[_idx],
							keywordStrToHighlight[iter->first].key.size()};
						highlightPositions.push_back(mti);
					}
					if (phrasesInfoList.size() > 0) {
						fwdList->getKeyWordPostionsInRecordField(j, attributeId, attributeBitMap, wordPosition);
						for(unsigned pidx = 0 ; pidx < phrasesInfoList.size(); ++pidx) {
							if (phrasesInfoList[pidx].phraseKeyWords[iter->first].recordPosition) {
								phrasesInfoList[pidx].phraseKeyWords[iter->first].recordPosition->
								assign(wordPosition.begin(), wordPosition.end());
							}
						}
						for (unsigned _idx = 0; _idx < wordPosition.size(); ++_idx){
							positionToOffsetMap.insert(make_pair(wordPosition[_idx], highlightPositions.size() - offsetPosition.size() + _idx));
						}
					}
				}
				++i; ++j;
			}
		}
	}

	/*
	 *   Now for the phrases, find the valid positions.
	 */
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
					highlightPositions[iter->second].flag = 4;
			}
		}
	}
	std::sort(highlightPositions.begin(), highlightPositions.end());

	// sweep out invalid positions
	removeInvalidPositionInPlace(highlightPositions);

	if (highlightPositions.size() == 0) {
		Logger::debug("could not generate a snippet because keywords could not be found in attribute.");
		return;
	}

	unsigned snippetLowerEnd = 0, snippetUpperEnd = highlightPositions.size() - 1;

	_genSnippet(ctv, ctsnippet, snippetUpperEnd, snippetLowerEnd, highlightPositions);
	string snippet;
	charTypeVectorToUtf8String(ctsnippet, snippet);
	snippets.push_back(snippet);
}

} /* namespace instanstsearch */
} /* namespace srch2 */
