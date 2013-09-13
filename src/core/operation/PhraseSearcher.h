//$Id$
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
    bool proximityMatch(const vector<vector<unsigned> >& positionListVector,
                        const vector<unsigned>& offsetsInPhrase, unsigned inputSlop,
                        vector<unsigned>& matchedPosition);
    signed  getPhraseSlop(const vector<unsigned>& query,
    		const vector<unsigned>& record);
    // CODE NOT IN USE
    unsigned geteditDistance_L(const vector<string>& keywords, const vector<string>& recordToMatch);
    // CODE NOT IN USE
    unsigned getEditDistance_DL(const vector<string>& src, const vector<string>& target);
private:
    // CODE NOT IN USE. Helper function for debugging
    void printCostTable(short *t, int r, int c);
    // CODE NOT IN USE. Helper function for debugging
    void printMap(const map<string, short>& m);

    unsigned slopThreshold;

};

struct comparator{
    bool operator() (const pair<unsigned, unsigned>& l, const pair<unsigned, unsigned>& r){
        return l.first > r.first;
    }
};

}
}
#endif /* __CORE_OPERATION_PHRASESEARCH_H__ */
