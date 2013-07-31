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
#include <instantsearch/SortFilter.h>
#include <instantsearch/Score.h>
#include <map>
#include <string>

using srch2::instantsearch::SortEvaluator;
using srch2::instantsearch::Score;

#ifndef _WRAPPER_SORTFILTEREVALUATOR_H_

#define _WRAPPER_SORTFILTEREVALUATOR_H_


namespace srch2
{
namespace httpwrapper
{

class SortFilterEvaluator : public SortEvaluator
{
public:
	int compare(const std::map<std::string, Score> & left,const std::map<std::string, Score> & right) const{

		for(std::vector<std::string>::const_iterator f = field.begin() ; f != field.end() ; ++f){
			if(compare(left.at(*f) , right.at(*f)) != 0){ // if left and right are equal on this attribute go to the next
				return compare(left.at(*f) , right.at(*f));
			}
		}
		return 0;

	}

	const std::vector<std::string> * getParticipatingAttributes() const {
		return &field;
	}
	~SortFilterEvaluator(){

	}

	std::vector<std::string> field;
	srch2::instantsearch::SortOrder order;

private:

	int compare(const Score & left , const Score & right) const{
		if(left == right ) return 0;
		if(order == srch2::instantsearch::Ascending){ // TODO should be checked in test to see if this function is returning proper result
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
