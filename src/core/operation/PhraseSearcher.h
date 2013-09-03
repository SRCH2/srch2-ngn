//$Id: $
/*
 * PhraseSearch.h
 *
 *  Created on: Sep 2, 2013
 *      Author: sbisht
 */

#ifndef __CORE_OPERATION_PHRASESEARCH_H__
#define __CORE_OPERATION_PHRASESEARCH_H__
#include <vector>
#include <string>
#include <map>

using namespace std;

namespace srch2 {
namespace instantsearch {

class PhraseSearcher {
public:
    PhraseSearcher();
    bool exactMatch (const vector<vector<unsigned> > &positionListVector,
    				 const vector<unsigned>& keyWordPositionsInPhrase,
                      vector<unsigned>& matchedPositions);
    bool proximityMatch(const vector<vector<unsigned> > &positionListVector,
                        const vector<string>& keywords, unsigned editDistance,
                        vector<unsigned>& matchedPosition);
    unsigned geteditDistance(const vector<string>& keywords, const vector<string>& recordToMatch);
    unsigned getEditDistance_DL(const vector<string>& src, const vector<string>& target);
private:
    void getFuzzyRecordSnippet(const vector<vector<unsigned> > &positionListVector,
                               vector<string>& record, vector<unsigned>& matchedPosition, int fuzzy);
    void printCostTable(short *t, int r, int c);
    void printMap(const map<string, short>& m);

    unsigned editDistanceThreshold;

};

struct comparator{
    bool operator() (const pair<unsigned, unsigned>& l, const pair<unsigned, unsigned>& r){
        return l.first > r.first;
    }
};

}
}
#endif /* __CORE_OPERATION_PHRASESEARCH_H__ */
