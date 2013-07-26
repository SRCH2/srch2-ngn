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



#include <vector>
#include <algorithm>


#include "instantsearch/ResultsPostProcessor.h"
#include "instantsearch/SortFilter.h"
#include "instantsearch/IndexSearcher.h"
#include "operation/IndexSearcherInternal.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "instantsearch/Score.h"

using namespace std;

namespace srch2
{
namespace instantsearch
{




class ResultNonSearchableAttributeComparator{
private:
	ForwardIndex* forwardIndex;
	Schema * schema;
	const Query * query;
	SortFilter * filter;


public:

	SortFilter::~SortFilter(){
		delete evaluator; // this object is allocated in plan Generator
	}


	ResultNonSearchableAttributeComparator(SortFilter * filter , ForwardIndex* forwardIndex , Schema * schema,const Query * query) {
		this->filter = filter;
		this->forwardIndex = forwardIndex;
		this->schema = schema;
		this->query = query;
	}

	// this operator should be consistent with two others in TermVirtualList.h and QueryResultsInternal.h
	bool operator() (const QueryResult * lhs, const QueryResult * rhs) const
	{
		//1. First get the value of non-searchable attributes for the participating attributes from forward index
		const vector<string> * attributes =  filter->evaluator->getParticipatingAttributes();


		bool isValid = false;
		const ForwardList * list = forwardIndex->getForwardList(lhs->internalRecordId , isValid);
		ASSERT(isValid);
		const VariableLengthAttributeContainer * lhsContainer = list->getNonSearchableAttributeContainer();

		bool isValid = false;
		list = forwardIndex->getForwardList(rhs->internalRecordId , isValid);
		ASSERT(isValid);
		const VariableLengthAttributeContainer * rhsContainer = list->getNonSearchableAttributeContainer();

		vector<unsigned> attributeIds;
		vector<unsigned> attributeIdsToSort;

		for(vector<string>::const_iterator attributeName = attributes->begin() ;
				attributeName != attributes->end() ; ++attributeName){
			unsigned id = schema->getNonSearchableAttributeId(*attributeName);
			attributeIds.push_back(id);
			attributeIdsToSort.push_back(id);
		}
		// sort because container expects the input ids to be ascending
		sort(attributeIdsToSort.begin() , attributeIdsToSort.end());

		// now get the values from the container
		vector<Score> lhsScoresAfterSort;
		lhsContainer->getBatchOfAttributes(attributeIdsToSort,schema, &lhsScoresAfterSort);

		vector<Score> rhsScoresAfterSort;
		rhsContainer->getBatchOfAttributes(attributeIdsToSort,schema, &rhsScoresAfterSort);

		// since sort has changed the places of items, we should "unsort"
		map<string,Score> lhsScores,rhsScores;

		unsigned a=0;
		for(vector<unsigned>::iterator id = attributeIdsToSort.begin();
				id != attributeIdsToSort.end() ; ++id){
			vector<unsigned>::iterator oldPlace = find(attributeIds.begin() , attributeIds.end() , *id);
			lhsScores[attributes->at(oldPlace - attributeIds.begin())] = lhsScoresAfterSort.at(a); // TODO : subtracting iterators might not be safe
			rhsScores[attributes->at(oldPlace - attributeIds.begin())] = rhsScoresAfterSort.at(a);

			//
			a++;
		}


		//2. then do the comparison
		if ( filter->evaluator->compare(lhsScores , rhsScores) >= 0 ){
			return true;
		}else{
			return false;
		}

	}
};


// TODO : we don't need query in new design
void SortFilter::doFilter(IndexSearcher * indexSearcher, const Query * query,
		QueryResults * input, QueryResults * output){

	IndexSearcherInternal * indexSearcherInternal = dynamic_cast<IndexSearcherInternal *>(indexSearcher);
	Schema * schema = indexSearcherInternal->getSchema();
	ForwardIndex * forwardIndex = indexSearcherInternal->getForwardIndex();

	// first copy all input results to output
	input->impl->copyForPostProcessing(output->impl);

	// now sort the results based on the comparator
	std::sort(output->impl->sortedFinalResults.begin(), output->impl->sortedFinalResults.end() ,
			ResultNonSearchableAttributeComparator(this,forwardIndex,schema,query));


}

}
}

