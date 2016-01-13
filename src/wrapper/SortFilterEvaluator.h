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
