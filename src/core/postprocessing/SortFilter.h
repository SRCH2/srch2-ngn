//$Id: NonSearchableAttributeExpressionFilter.h 3456 2013-07-10 02:11:13Z Jamshid $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */


#ifndef _CORE_POSTPROCESSING_SORTFILTER_H_
#define _CORE_POSTPROCESSING_SORTFILTER_H_


#include <vector>
#include <algorithm>


#include "ResultsPostProcessor.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "instantsearch/Score.h"

namespace srch2
{
namespace instantsearch
{




class SortFilter : public ResultsPostProcessorFilter
{

public:
	// TODO : we don't need query in new design
	void doFilter(Schema * schema, ForwardIndex * forwardIndex, const Query * query,
			 QueryResults * input, QueryResults * output){

		// first copy all input results to output
		input->impl->copyForPostProcessing(output->impl);

		// now sort the results based on the comparator
		std::sort(output->impl->sortedFinalResults.begin(), output->impl->sortedFinalResults.end() ,
				SortFilter::ResultNonSearchableAttributeComparator(this,forwardIndex,schema,query));


	}
	~SortFilter(){

	}


	// TODO : temperorrily we keep two simple variables to be filled up at query builder
	std::string attributeName;
	SortOrder order;

private:
	// evaluates expression object coming from query using result data to
	// calculate the score based on result's data.





	class ResultNonSearchableAttributeComparator{
    private:
        ForwardIndex* forwardIndex;
        Schema * schema;
        const Query * query;
        SortFilter * filter;

        // based on the expression given in query, a value is assigned to each result and then
        // results are sorted based on these values.
    	void getResultScoreForPostProcessingSort(const QueryResult * & result,Score * score) const{
    		// TODO: this must change to get score based on a expression from query


    		// getting attributeID from schema
    		unsigned attributeId = schema->getNonSearchableAttributeId(filter->attributeName);

    		// attribute type
    		FilterType attributeType = schema->getTypeOfNonSearchableAttribute(attributeId);

			bool isValid = false;
			const ForwardList * list = forwardIndex->getForwardList(result->internalRecordId , isValid);
			ASSERT(isValid);
			const VariableLengthAttributeContainer * nonSearchableAttributes = list->getNonSearchableAttributeContainer();

			nonSearchableAttributes->getAttribute(attributeId,schema, score);


    	}
    public:
        ResultNonSearchableAttributeComparator(SortFilter * filter , ForwardIndex* forwardIndex , Schema * schema,const Query * query) {
            this->filter = filter;
        	this->forwardIndex = forwardIndex;
            this->schema = schema;
            this->query = query;
        }

        // this operator should be consistent with two others in TermVirtualList.h and QueryResultsInternal.h
        bool operator() (const QueryResult * lhs, const QueryResult * rhs) const
        {
        	Score lhsScore,rhsScore;
        	getResultScoreForPostProcessingSort(lhs , &lhsScore);
        	getResultScoreForPostProcessingSort(rhs , &rhsScore);
        	return (filter->order==srch2::instantsearch::Ascending)?
        			lhsScore < rhsScore :
        			!(lhsScore < rhsScore);
        }
	};
};

}
}
#endif // _CORE_POSTPROCESSING_SORTFILTER_H_

