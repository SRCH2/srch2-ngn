/*
 * LowerCaseFilter.h
 *
 *  Created on: 2013-5-17
 */
//This class is used to transform the English letters to lower case

#ifndef __CORE_ANALYZER__LOWERCASEFILTER_H__
#define __CORE_ANALYZER__LOWERCASEFILTER_H__

#include "TokenStream.h"
#include "TokenFilter.h"

namespace srch2
{
namespace instantsearch
{

/*
 *  LowerCaseFilter transform English letter in token to lower case
 *  for example: Token: "School" -> "school"
 */
class LowerCaseFilter:public TokenFilter {
public:
	LowerCaseFilter(TokenStream* tokenStream);
	bool processToken();
	virtual ~LowerCaseFilter();
private:
	void transformToLowerCase(std::vector<CharType> &token);
};
}}
#endif /* __CORE_ANALYZER__LOWERCASEFILTER_H__ */
