/*
 * LowerCaseFilter.h
 *
 *  Created on: 2013-5-17
 */
//This class is used to transform the English letters to lower case

#ifndef __LOWERCASEFILTER_H__
#define __LOWERCASEFILTER_H__

#include "TokenOperator.h"
#include "TokenFilter.h"

namespace bimaple
{
namespace instantsearch
{

/*
 *  LowerCaseFilter transform English letter in token to lower case
 *  for example: Token: "School" -> "school"
 */
class LowerCaseFilter:public TokenFilter {
public:
	LowerCaseFilter(TokenOperator* tokenOperator);
	bool incrementToken();
	virtual ~LowerCaseFilter();
private:
	void transformToLowerCase(std::vector<CharType> &token);
};
}}
#endif /* __LOWERCASEFILTER_H__ */
