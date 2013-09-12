// $Id: $
/*
 * PhraseSearch.cpp
 *
 *  Created on: Sep 2, 2013
 *      Author: sbisht
 */

#include "PhraseSearcher.h"
#include "util/Logger.h"
#include <iostream>
#include <climits>
#include <queue>

using srch2::util::Logger;
namespace srch2 {
namespace instantsearch {

template <class T>
void printVector(const vector<T>& v);

PhraseSearcher::PhraseSearcher() {
    editDistanceThreshold = 100;
}
/*
 *  The function determines whether it is an exact match for the given phrase. The function
 *  expect as an input a vector of positionlists for each keyword of the phrase in same attribute.
 *  matchedPosition returns the first occurrence of exact match.
 *
 *  for a example record
 *  rec = {
 *          title : "The shining" ,
 *          description: "The Shining is a 1980 psychological horror film produced
 *                        and directed by Stanley Kubrick"
 *        }
 *  query
 *  q1 = "psychological horror"  is an exact match
 *  q2 = "psychological film" is not an exact match
 *  q3 = "Shining 1980" is not an exact match even if analyzer drops stop words (is, a)
 */
bool PhraseSearcher::exactMatch(const vector<vector<unsigned> > &positionListVector,
                                const vector<unsigned>& keyWordPositionsInPhrase,
                                vector<unsigned>& matchedPosition) {
    bool searchDone = false;
    bool matchFound = false;
    unsigned prevKeyWordPosition = 0;

    if (positionListVector.size() != keyWordPositionsInPhrase.size()){
    	Logger::debug("exactMatch: Position lists not provided for all keywords");
    	return false;
    }

    if (positionListVector.size() == 0){
    	Logger::debug("exactMatch: Position lists vector size is 0");
        return false;
    }
    // make sure none of the position list is not empty
    for (int i = 0; i < positionListVector.size(); ++i){
        if (positionListVector[i].size() == 0){
        	Logger::debug("exactMatch: Position list is empty for one of the keywords");
            return false;
        }
    }

    if (positionListVector.size() == 1 ) { // only 1 keyword in phrase!
        return true;
    }

    vector<unsigned> cursorPosition(positionListVector.size());

    while(!searchDone) {

        prevKeyWordPosition = positionListVector[0][cursorPosition[0]];
        unsigned i;
        for (i = 1; i < positionListVector.size(); ++i) {

            const vector<unsigned>& currList =  positionListVector[i];
            unsigned currKeyWordPosition = 0;
            unsigned j = 0;
            for (j = cursorPosition[i]; j < currList.size(); ++j)
            {
                if (currList[j] > prevKeyWordPosition)
                {
                    currKeyWordPosition = currList[j];
                    break;
                }
            }
            cursorPosition[i] = j;

            // we have reached at end of one of the list. Hence this should be
            // the final while loop.
            if (j == currList.size())
                searchDone = true;
            unsigned relativeDistance = keyWordPositionsInPhrase[i] - keyWordPositionsInPhrase[i-1];
            if (currKeyWordPosition - prevKeyWordPosition  != relativeDistance)
                break;
            else
                prevKeyWordPosition = currKeyWordPosition;
        }

        if (i == positionListVector.size()){
            // we found the match, break from outer while loop
            matchFound = true;
            break;
        }

        while(true) {
            ++cursorPosition[0];
            if (cursorPosition[0] >= positionListVector[0].size())
            {
                searchDone = true;
                break;
            }
            signed diff = positionListVector[0][cursorPosition[0]] -
                          positionListVector[1][cursorPosition[1]];
            signed relativeDistance = keyWordPositionsInPhrase[0] - keyWordPositionsInPhrase[1];
            if (diff == relativeDistance || diff > 0)
                break;
        }
    }

    if(matchFound)
    {
        for (unsigned i =0; i < cursorPosition.size(); ++i)
        {
            matchedPosition.push_back(positionListVector[i][cursorPosition[i]]);
        }
    }
    return matchFound;
}
/*
 *  The function determines whether it is proximity match for a given keyword
 *  and edit distance (slop) in a positionlistVector. MatchedPosition return
 *  first occurrence of proximity match.
 *
 *  See getPhraseSlop function below for more detail.
 *
 */
bool PhraseSearcher::proximityMatch(const vector<vector<unsigned> >& positionListVector,
                    const vector<unsigned>& offsetsInPhrase, unsigned inputSlop,
                    vector<unsigned>& matchedPosition)
{
    // pre-conditions

    if (positionListVector.size() == 0) {
    	Logger::debug("proximityMatch: Position list vector size is 0");
        return false;
    }

    if (positionListVector.size() != offsetsInPhrase.size()) {
    	Logger::debug("proximityMatch: Position list vector size did not match" \
    				  "with query keywords");
        return false;
    }

    // make sure none of the position list is not empty
    for (int i = 0; i < positionListVector.size(); ++i){
    	if (positionListVector[i].size() == 0){
    		Logger::debug("proximityMatch: Position list is empty for one of the keywords");
    		return false;
    	}
    }

    if (inputSlop == 0){
    	Logger::debug("proximityMatch: Edit distance cannot be 0 for proximity match");
        return false;
    }

    if (inputSlop > editDistanceThreshold) {
    	Logger::debug("proximityMatch: Edit distance %d is more than threshold %d", inputSlop,
    				editDistanceThreshold);
        return true;
    }

    // Now go over the position list and get the the vector string that could
    // possible match with keyword list for a give edit distance

    std::priority_queue<std::pair<unsigned, unsigned>,
                        vector<std::pair<unsigned, unsigned> >,
                        comparator > minHeap;
   // unsigned max = 0;
    unsigned min = 0;
    unsigned totalKeyWords = positionListVector.size();
    vector<unsigned> cursors(totalKeyWords);
    // initialize cursors
    for (unsigned i = 0; i < totalKeyWords; ++i) {
        minHeap.push(make_pair(positionListVector[i].front(),i));
        //if (positionListVector[i].front() > max)
        //    max = positionListVector[i].front();
    }
    while(1) {
        min = minHeap.top().first;
        matchedPosition.clear();
        //if (max - min < totalKeyWords + editDistance){
        for (unsigned i =0; i < positionListVector.size(); ++i){
        	unsigned pos = positionListVector[i][cursors[i]];
        	matchedPosition.push_back(pos);
        }
        if ((signed)inputSlop >= getPhraseSlop(offsetsInPhrase, matchedPosition)) {
        	return true;
        }
            //cout << "record : " ; printVector(record);
            /*if (editDistance >= getEditDistance_DL(keywords, record)) {
                return true;
            }*/
        //}
        unsigned currentListIndex = minHeap.top().second;
        const vector<unsigned>& currList = positionListVector[currentListIndex];
        ++cursors[currentListIndex];
        if (cursors[currentListIndex] >= currList.size())
            break;
        unsigned next = currList[cursors[currentListIndex]];
        minHeap.pop();
        minHeap.push(make_pair( next, currentListIndex));
    }

    return false;
}

/*
 * The function calculates difference in position of keywords in record and
 * query. Then max(difference) - min(difference) is considered as "slop".
 *
 * Original record =>    "new york time square hudson river"
 * position in record =>  1   2    3    4      5      6
 *
 * phrase 1 = "york^1 new^2 square^3 time^4",
 *
 * diff("york") = 1
 * diff("new") = -1
 * diff("time") = -1
 * diff("square") = 1
 *
 * max(diff) - min(diff) = 1 - (-1) =  2. Hence slop >= 2 should match this record.
 *
 */
signed PhraseSearcher::getPhraseSlop(const vector<unsigned>& query,
		const vector<unsigned>& record){

	signed maxDiff = INT_MIN;
	signed minDiff = INT_MAX;
	for (int i=0; i < query.size(); ++i){
		signed diff = record[i] - query[i];
		if (diff > maxDiff)
			maxDiff = diff;
		if (diff < minDiff)
			minDiff = diff;
	}
	return maxDiff - minDiff;
}
/*
 * The function calculates Levenshtein distance. Also known as edit distance
 * See link below for more details
 * http://en.wikipedia.org/wiki/Levenshtein_distance
 *
 * Original record => "new york time square"
 *
 * phrase 1 = "york new square time", edit distance should be 3
 * phrase 2 = "york square new time", edit distance should be 4
 *
 * Original record => "new york is famous for time square"
 *
 * phrase 1 = "york new square time", edit distance should be 5
 * phrase 2 = "york square new time", edit distance should be 5
 *
 */
unsigned PhraseSearcher::geteditDistance_L(const vector<string>& keywords,
                                         const vector<string>& recordToMatch) {

    unsigned cost;
    vector<unsigned> v1(keywords.size() + 1);
    vector<unsigned> v2(keywords.size() + 1);

    for (unsigned i = 0; i < keywords.size(); ++i)
        v1[i] = i;

    for (unsigned i =0; i < recordToMatch.size(); ++i) {
        v2[0] = i + 1;
        for (unsigned j = 0; j < keywords.size(); ++j) {
            cost = ( recordToMatch[i] ==  keywords[j]) ? 0 : 1;
            v2[j + 1] = std::min (std::min( v2[j] + 1, v1[j + 1] + 1), v1[j] + cost);
        }
        for (unsigned k =0; k < v1.size(); ++k)
            v1[k] = v2[k];
    }
    return v2[keywords.size()];
}

/*
 * The function calculates Damerau–Levenshtein distance.The algorithm improves
 * the regular edit distance by also taking swaps into account. Each swap is
 * considered as one edit. See link below for more details
 * http://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance
 *
 * Original record => "new york time square"
 *
 * phrase 1 = "york new square time", edit distance should be 2
 * phrase 2 = "york square new time", edit distance should be 3
 *
 * Original record => "new york is famous for time square"
 *
 * phrase 1 = "york new square time", edit distance should be 5
 * phrase 2 = "york square new time", edit distance should be 5
 *
 */

unsigned PhraseSearcher::getEditDistance_DL(const vector<string>& src,
                                            const vector<string>& target) {

    std::map<string, short> lastRowMap;
    unsigned lastColMatched = 0;
    unsigned columnSize = target.size() + 2;
    unsigned rowSize    = src.size() + 2;

    short* costTable = new short [ columnSize * rowSize];

    *costTable = columnSize + rowSize + 1;

    for (unsigned i = 1; i < rowSize; ++i) {
        costTable[i * columnSize + 0] = columnSize + rowSize + 1;
        costTable[i * columnSize + 1] = i - 1;
    }

    for (unsigned i = 1; i < columnSize; ++i) {
        costTable[i] = columnSize + rowSize + 1;
        costTable[columnSize + i] = i - 1;
    }
    //printCostTable(costTable, rowSize, columnSize);
    for (unsigned i = 2; i < rowSize; ++i) {
        lastColMatched = 0;
        for (unsigned j = 2; j < columnSize; ++j) {

            short costSubstitute = costTable[(i - 1) * columnSize + (j - 1)];
            if (src[i - 2] == target[j - 2]) {
                lastColMatched = j;
            } else {
                costSubstitute += 1;
            }

            short costSwap = *costTable;
            std::map<string, short>::iterator iter = lastRowMap.find(target[j - 2]);
            if (iter != lastRowMap.end()) {
                unsigned lastRow = iter->second;
                costSwap = costTable[(lastRow - 1) * columnSize + lastColMatched -1] +
                           (i - lastRow - 1) +
                           (j - lastColMatched - 1) + 1;
            }

            short costInsert = costTable[i * columnSize + (j - 1)] + 1;
            short costDelete = costTable[(i - 1) * columnSize + j] + 1;

            costTable[i * columnSize + j] = std::min( std::min(costSwap,
                                            std::min(costInsert, costDelete)), costSubstitute);
        }
        lastRowMap[src[i - 2]] = (short)i;
        //printMap(lastRowMap);
    }
    //printCostTable(costTable, rowSize, columnSize);
    unsigned editDistance = costTable[(rowSize - 1) * columnSize + (columnSize - 1)];
    delete costTable;
    return editDistance;
}
void PhraseSearcher::printMap(const map<string, short>& m){
    cout << "-----------------------" << endl;
    for (map<string, short>::const_iterator i = m.begin(); i != m.end(); ++i) {
        cout << i->first << " - " << i->second << endl;
    }
    cout << "-----------------------" << endl;
}
void PhraseSearcher::printCostTable(short *t, int r, int c){
    cout << endl;
    for (int i =0; i < r; ++i){
        for (int j =0 ; j < c; ++j)
            cout /*<< std::setw(4) */ <<  t[ i * c + j] << ",";
        cout << endl;
    }
}
template <class T>
void printVector(const vector<T>& v){
    for (unsigned i = 0; i < v.size(); ++i)
        cout << "'"<< v[i] <<  "' " ;
    cout << endl;
}

}
}

