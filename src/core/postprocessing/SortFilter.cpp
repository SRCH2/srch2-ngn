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

namespace srch2 {
namespace instantsearch {

class ResultNonSearchableAttributeComparator {
private:
    ForwardIndex* forwardIndex;
    Schema * schema;
    const Query * query;
    SortFilter * filter;

public:

    ResultNonSearchableAttributeComparator(SortFilter * filter,
            ForwardIndex* forwardIndex, Schema * schema, const Query * query) {
        this->filter = filter;
        this->forwardIndex = forwardIndex;
        this->schema = schema;
        this->query = query;
    }

    // this operator should be consistent with two others in TermVirtualList.h and QueryResultsInternal.h
    bool operator()(const QueryResult * lhs, const QueryResult * rhs) const {

        // do the comparison
        if (filter->evaluator->compare(lhs->valuesOfParticipatingNonSearchableAttributes,
                rhs->valuesOfParticipatingNonSearchableAttributes) > 0) {
            return true;
        } else {
            return false;
        }

    }
};

SortFilter::~SortFilter() {
    delete evaluator; // this object is allocated in plan Generator
}

// TODO : we don't need query in new design
void SortFilter::doFilter(IndexSearcher * indexSearcher, const Query * query,
        QueryResults * input, QueryResults * output) {

    ASSERT(evaluator != NULL);
    if(evaluator == NULL) return;

    IndexSearcherInternal * indexSearcherInternal =
            dynamic_cast<IndexSearcherInternal *>(indexSearcher);
    Schema * schema = indexSearcherInternal->getSchema();
    ForwardIndex * forwardIndex = indexSearcherInternal->getForwardIndex();

    // first copy all input results to output
    output->copyForPostProcessing(input);

    // extract all the information from forward index
    // 1. find the participating attributes
    /*
     * Example : for example if the query contains "name,age,bdate ASC" , the participating attributes are
     *           name, age and bdate.
     */
    const vector<string> * attributes =
            evaluator->getParticipatingAttributes();
    vector<unsigned> attributeIds;
    for (vector<string>::const_iterator attributeName = attributes->begin();
            attributeName != attributes->end(); ++attributeName) {
        unsigned id = schema->getNonSearchableAttributeId(*attributeName);
        attributeIds.push_back(id);
    }
    // 2. extract the data from forward index.
    for(std::vector<QueryResult *>::iterator queryResultIterator = output->impl->sortedFinalResults.begin() ;
            queryResultIterator != output->impl->sortedFinalResults.end() ; ++queryResultIterator){
        QueryResult * queryResult = *queryResultIterator;
        bool isValid = false;
        const ForwardList * list = forwardIndex->getForwardList(
                queryResult->internalRecordId, isValid);
        ASSERT(isValid);
        const VariableLengthAttributeContainer * nonSearchableAttributeContainer =
                list->getNonSearchableAttributeContainer();
        // now get the values from the container
        vector<Score> scores;
        nonSearchableAttributeContainer->getBatchOfAttributes(attributeIds, schema,
                &scores);
        // save the values in QueryResult objects
        for(std::vector<string>::const_iterator attributesIterator = attributes->begin() ;
                attributesIterator != attributes->end() ; ++attributesIterator){
            queryResult->valuesOfParticipatingNonSearchableAttributes[*attributesIterator] =
                    scores.at(std::distance(attributes->begin() , attributesIterator));
        }


    }

    // 3. now sort the results based on the comparator
    std::sort(output->impl->sortedFinalResults.begin(),
            output->impl->sortedFinalResults.end(),
            ResultNonSearchableAttributeComparator(this, forwardIndex, schema,
                    query));

}

}
}

