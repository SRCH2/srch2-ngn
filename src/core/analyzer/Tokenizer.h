/*
 * Tokenizer.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __CORE_ANALYZER__TOKENIZER_H__
#define __CORE_ANALYZER__TOKENIZER_H__

#include "TokenOperator.h"

namespace srch2
{
namespace instantsearch
{
/*
 * Tokenizer: tokenize a string
 */
class Tokenizer:public TokenOperator {
public:
	Tokenizer()
	{}
	bool emitToken()
	{
		if(!(sharedToken->currentToken).empty())
		{
	        return true;
		}
		else
			return false;
	}
	virtual ~Tokenizer()
	{
	}
};
}}
#endif /* __CORE_ANALYZER__TOKENIZER_H__*/
