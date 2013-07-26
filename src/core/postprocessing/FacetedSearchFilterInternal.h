//$Id: ResultsPostProcessor.h 3456 2013-06-26 02:11:13Z Jamshid $

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


#ifndef _CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H_
#define _CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H_


#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include "instantsearch/Score.h"

#include "operation/IndexSearcherInternal.h"

namespace srch2
{
namespace instantsearch
{

class FacetedSearchFilterInternal

{

public:

	void facetByCountAggregation(const Score & attributeValue ,
			const std::vector<Score> & lowerBounds ,
			std::vector<pair<string, float> >  * counts ){

		if(lowerBounds.empty()){ // simple facet
			// move on results to see if this value is seen before (increment) or is new (add and initialize)
			for(std::vector<pair<string, float> >::iterator p = counts->begin() ; p != counts->end() ; ++p){
				if(p->first.compare(attributeValue.toString()) == 0){
					p->second = p->second + 1;
					return;
				}
			}
			counts->push_back(make_pair(attributeValue.toString() , 1));
			return;
		}else{
			unsigned i = 0;
			for(std::vector<Score>::const_iterator lowerBound = lowerBounds.begin() ; lowerBound!=lowerBounds.end() ; ++lowerBound){
				bool fallsInThisCategory = false;
				// It's assumed that lowerBound list is sorted so the
				// first category also accepts records things which are less than it.
				if(i == 0) {
					fallsInThisCategory = true;
				}else{ // in normal case if the value is greater than lowerBound it's potentially in this category.
					if(*lowerBound <= attributeValue){
						fallsInThisCategory = true;
					}
				}
				if(fallsInThisCategory){
					// if this is not the last category, the value should also be less than the next lowerBound
					if(i != lowerBounds.size()-1){
						if(*(lowerBound + 1) <= attributeValue){
							fallsInThisCategory = false;
						}
					}
				}
				if(fallsInThisCategory){
					counts->at(i).second = counts->at(i).second + 1;
					return;
				}
				//
				++i;
			}
			// normally we should never reach to this point because each record should fall in at least one of the categories.
			ASSERT(false);
		}


	}

	// this function prepares the lowebound structure from the paraller string vectors
	void prepareFacetInputs(IndexSearcher *indexSearcher){


		IndexSearcherInternal * indexSearcherInternal = dynamic_cast<IndexSearcherInternal *>(indexSearcher);
		Schema * schema = indexSearcherInternal->getSchema();
		ForwardIndex * forwardIndex = indexSearcherInternal->getForwardIndex();


		std::vector<Score> rangeStartScores;
		std::vector<Score> rangeEndScores;
		std::vector<Score> rangeGapScores;
		// 1. parse the values into Score.
		unsigned f = 0;
		for(std::vector<std::string>::iterator field = this->fields.begin() ;
				field != this->fields.end() ; ++field){
			FilterType type = schema->getTypeOfNonSearchableAttribute(schema->getNonSearchableAttributeId(*field));
			Score start;
			start.setScore(type , this->rangeStarts.at(f));
			rangeStartScores.push_back(start);

			Score end;
			end.setScore(type , this->rangeEnds.at(f));
			rangeStartScores.push_back(end);

			Score gap;
			gap.setScore(type , this->rangeGaps.at(f));
			rangeStartScores.push_back(gap);

			//
			f++;
		}


		// 2. create the lowebound vector for each attribute

		unsigned t = 0;
		for(std::vector<FacetType>::iterator type = types.begin() ; type != types.end() ; ++type){
			std::vector<Score> lowerBounds;

			switch (*type) {
				case Range:
					break;
				case Simple:
					Score & start = rangeStartScores.at(t);
					Score & end = rangeEndScores.at(t);
					Score & gap = rangeGapScores.at(t);


					Score temp = start;
					lowerBounds.push_back(temp.minimumValue()); // to collect data smaller than start
					while(temp < end){
						lowerBounds.push_back(temp); // data of normal categories
						temp = temp + gap;
					}
					lowerBounds.push_back(temp); // to collect data greater than end
					break;
			}
			lowerBoundsOfCategories[fields.at(t )] = lowerBounds;

			//
			t++;
		}



	}


	//
	std::vector<FacetType> types;
	std::vector<std::string> fields;
	std::vector<std::string> rangeStarts;
	std::vector<std::string> rangeEnds;
	std::vector<std::string> rangeGaps;




	std::map<std::string , std::vector<Score> > lowerBoundsOfCategories;

};

}
}

#endif // _CORE_POSTPROCESSING_FACETEDSEARCHFILTERINTERNAL_H_
