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

class PhraseQueryFilter : public ResultsPostProcessorFilter {
public:
    virtual void doFilter(QueryEvaluator *queryEvaluator, const Query * query,
                 QueryResults * input , QueryResults * output);


    PhraseQueryFilter(const PhraseQueryFilter & filter){
    	this->phraseInfoVector = filter.phraseInfoVector;
    }

	virtual ResultsPostProcessorFilter * getNewCopy() const{
		PhraseQueryFilter * newCopy = new PhraseQueryFilter(*this);
		return newCopy;
	}
private:

    bool matchPhrase(const ForwardList * forwardListPtr, const PhraseInfo& phraseInfo);
    vector<PhraseInfo> phraseInfoVector;
};

}
}
#endif /* __CORE_POSTPROCESSING_PHRASESEARCHFILTER_H__ */
