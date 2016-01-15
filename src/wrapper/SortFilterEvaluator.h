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
#include <instantsearch/ResultsPostProcessor.h>
#include <instantsearch/TypedValue.h>
#include <map>
#include <string>
#include <sstream>

using srch2::instantsearch::SortEvaluator;
using srch2::instantsearch::TypedValue;

#ifndef _WRAPPER_SORTFILTEREVALUATOR_H_

#define _WRAPPER_SORTFILTEREVALUATOR_H_


namespace srch2
{
namespace httpwrapper
{

class SortFilterEvaluator : public SortEvaluator
{
public:
	int compare(const std::map<std::string, TypedValue> & left, unsigned leftTieBreaker,const std::map<std::string, TypedValue> & right, unsigned rightTieBreaker) const{

		for(std::vector<std::string>::const_iterator attributeIndex = field.begin() ; attributeIndex != field.end() ; ++attributeIndex){
		    int comparisonResultOnThisAttribute = compareOneAttribute(left.at(*attributeIndex) , right.at(*attributeIndex));
			if( comparisonResultOnThisAttribute != 0){ // if left and right are equal on this attribute go to the next
				return comparisonResultOnThisAttribute;
			}
		}
		// if we reach here then left value is equal to right value. Hence we use tiebreaker ( internal record ids)
		// to determine the order. It helps in achieving deterministic order.
		if(order == srch2::instantsearch::SortOrderAscending){
			if(leftTieBreaker < rightTieBreaker) return 1;
			else return -1;
		}else{
			if(leftTieBreaker < rightTieBreaker) return -1;
			else return 1;
		}
	}

	const std::vector<std::string> * getParticipatingAttributes() const {
		return &field;
	}
	~SortFilterEvaluator(){

	}

	string toString() const {
		stringstream ss;
		for(unsigned i=0 ; i<field.size() ; ++i){
			ss << field[i].c_str();
		}
		ss << order ;
		return ss.str();
	}
	std::vector<std::string> field;

private:

	int compareOneAttribute(const TypedValue & left , const TypedValue & right) const{
		if(left == right ) return 0;
		if(order == srch2::instantsearch::SortOrderAscending){ // TODO should be checked in test to see if this function is returning proper result
			if(left < right) return 1;
			else return -1;
		}else{
			if(left < right) return -1;
			else return 1;
		}
	}


};



}

}



#endif // _WRAPPER_SORTFILTEREVALUATOR_H_
