//$ID$
/*
 * PhraseSearchFilter.h
 *
 *  Created on: Sep 9, 2013
 *      Author: sbisht
 */

#ifndef __CORE_POSTPROCESSING_PHRASESEARCHFILTER_H__
#define __CORE_POSTPROCESSING_PHRASESEARCHFILTER_H__
#include <vector>
#include <string>
#include <instantsearch/ResultsPostProcessor.h>
#include <instantsearch/Query.h>
#include "index/ForwardIndex.h"
#include "operation/TermVirtualList.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

class PhraseInfo{
    public:
        vector<string> phraseKeyWords;
        vector<unsigned> keywordIds;
        vector<unsigned> phraseKeywordPositionIndex;
        unsigned proximitySlop;
        unsigned attributeBitMap;
};

class PhraseQueryFilter : public ResultsPostProcessorFilter {
public:
    virtual void doFilter(IndexSearcher *indexSearcher, const Query * query,
                 QueryResults * input , QueryResults * output);
    void addPhrase(const vector<string>& keywordsInPhrase,
                   const vector<unsigned>& queryKeywordsPositionIndex,
                   unsigned slopValue, unsigned fieldVector);
private:

    bool matchPhrase(const ForwardList * forwardListPtr, const PhraseInfo& phraseInfo);
    vector<PhraseInfo> phraseInfoVector;
};

}
}
#endif /* __CORE_POSTPROCESSING_PHRASESEARCHFILTER_H__ */
