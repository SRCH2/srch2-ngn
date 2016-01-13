

#ifndef __CORE_POSTPROCESSING_RESULTSPOSTPROCESSORINTERNAL_H__
#define __CORE_POSTPROCESSING_RESULTSPOSTPROCESSORINTERNAL_H__

#include <vector>

#include <instantsearch/ResultsPostProcessor.h>
#include "util/Assert.h"

using namespace std;

namespace srch2
{
namespace instantsearch
{

class ResultsPostProcessorPlanInternal
{
public:
	vector<ResultsPostProcessorFilter *> filterVector;
	vector<ResultsPostProcessorFilter *>::iterator filterIterator;

	ResultsPostProcessorPlanInternal(){
	    filterIterator = filterVector.end();
	}

	~ResultsPostProcessorPlanInternal(){
		for(vector<ResultsPostProcessorFilter *>::iterator filter = filterVector.begin();
								filter != filterVector.end(); ++filter){
		    if (*filter != NULL){
                delete *filter;
		    }else{
		        ASSERT(false);
		    }
		}
	}
};

}
}

#endif // __CORE_POSTPROCESSING_RESULTSPOSTPROCESSORINTERNAL_H__
