/*
 * Tokenizer.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include "TokenOperator.h"

namespace bimaple
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
#endif /* __TOKENIZER_H__*/
