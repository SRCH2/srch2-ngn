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
//$Id$
/*
 * PhraseSearch.h
 *
 *  Created on: Sep 2, 2013
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
            vector<vector<unsigned> >& matchedPositions,vector<unsigned>& listOfSlops, bool stopAtFirstMatch);
    bool proximityMatch(const vector<vector<unsigned> >& positionListVector,
            const vector<unsigned>& offsetsInPhrase, unsigned inputSlop,
            vector< vector<unsigned> >& matchedPosition,vector<unsigned>& listOfSlops, bool stopAtFirstMatch);
    signed  getPhraseSlopDistance(const vector<unsigned>& query,
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
