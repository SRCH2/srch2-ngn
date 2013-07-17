/*
 * TokenFilter.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __CORE_ANALYZER__TOKENFILTER_H__
#define __CORE_ANALYZER__TOKENFILTER_H__

#include "TokenOperator.h"
namespace srch2
{
namespace instantsearch
{
/*
 * TokenFilter: filter a token
 */
class TokenFilter:public TokenOperator {
public:
	TokenFilter(TokenOperator* tokenOperator)
	{
		this->tokenOperator = tokenOperator;
	}
	virtual ~TokenFilter()
	{
		delete tokenOperator;
	}
protected:
	// a linker to a TokenFilter or a Tokenizer
	TokenOperator* tokenOperator;
};
}}
#endif /* __CORE_ANALYZER__TOKENFILTER_H__ */
