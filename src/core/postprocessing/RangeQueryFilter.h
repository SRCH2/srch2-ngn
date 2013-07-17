//$Id: RangeQueryFilter.h 3456 2013-06-26 02:11:13Z Jamshid $

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


#ifndef _CORE_POSTPROCESSING_RANGEQUERYFILTER_H_
#define _CORE_POSTPROCESSING_RANGEQUERYFILTER_H_


#include <vector>
#include <map>
#include <string>


#include "ResultsPostProcessor.h"
#include "instantsearch/Schema.h"
#include "index/ForwardIndex.h"
#include "util/VariableLengthAttributeContainer.h"
#include "instantsearch/Score.h"

namespace srch2
{
namespace instantsearch
{




class RangeQueryFilter : public ResultsPostProcessorFilter
{

public:
	void doFilter(Schema * schema, ForwardIndex * forwardIndex, const Query * query,
			QueryResults * input, QueryResults * output){

		std::string attributeName = query->getNonSearchableAttributeName();
		// TODO : operation also needs to come from query, for now we assume it's always " attribute < value "

		// getting attributeID from schema
		unsigned attributeId = schema->getNonSearchableAttributeId(attributeName);

		// attribute type
		FilterType attributeType = schema->getTypeOfNonSearchableAttribute(attributeId);

		Score attributeValue; // get attribute value from query
		attributeValue.setScore(attributeType , query->getNonSearchableAttributeValue());

		// iterating on results and checking the criteria on each of them
		for(vector<QueryResult *>::iterator resultIter = input->impl->sortedFinalResults.begin(); resultIter != input->impl->sortedFinalResults.end() ; ++resultIter){
			QueryResult * result = *resultIter;

			bool isValid = false;
			const ForwardList * list = forwardIndex->getForwardList(result->internalRecordId , isValid);
			ASSERT(isValid);
			const VariableLengthAttributeContainer * nonSearchableAttributes = list->getNonSearchableAttributeContainer();
			Score attributeData;
			// get the data of this attribute from forward index
			switch (attributeType) {
				case srch2::instantsearch::UNSIGNED:
					attributeData.setScore(nonSearchableAttributes->getUnsignedAttribute(attributeId,schema));
					break;
				case srch2::instantsearch::FLOAT:
					attributeData.setScore(nonSearchableAttributes->getFloatAttribute(attributeId,schema));
					break;
				case srch2::instantsearch::TEXT:
					attributeData.setScore(nonSearchableAttributes->getTextAttribute(attributeId,schema));
					break;
				case srch2::instantsearch::TIME:
					attributeData.setScore(nonSearchableAttributes->getTimeAttribute(attributeId,schema));
					break;
			}

			// check the data and query value with the condition // TODO this must change to a good implementation of expression and criteria

			// <, >, <=, >=, ==, ... are overloaded for class Score
			bool pass = false;
			switch (query->getPostProcessingFilterOperation()) {
				case LESS_THAN:
					pass = (attributeData < attributeValue) ;
					break;
				case LESS_THAN_EQUALS:
					pass = (attributeData <= attributeValue) ;
					break;
				case EQUALS:
					pass = (attributeData == attributeValue) ;
					break;
				case GREATER_THAN:
					pass = (attributeData > attributeValue) ;
					break;
				case GREATER_THAN_EQUALS:
					pass = (attributeData >= attributeValue) ;
					break;
				case NOT_EQUALS:
					pass = (attributeData != attributeValue) ;
					break;
			}
			// if the result passes the filter criteria it's copied into the output.
			if(pass){
				output->impl->sortedFinalResults.push_back(result);
			}


		}


	}
	~RangeQueryFilter(){

	}
};

}
}
#endif // _CORE_POSTPROCESSING_RANGEQUERYFILTER_H_

