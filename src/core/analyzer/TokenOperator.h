/*
 * TokenOperator.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __TOKEN_OPERATOR_H__
#define __TOKEN_OPERATOR_H__

#include <boost/shared_ptr.hpp>
#include "SharedToken.h"

namespace bimaple
{
namespace instantsearch
{

/*
 * TokenOperator can be a Tokenizer or TokenFilter
 */
 class TokenOperator {
public:
	boost::shared_ptr<SharedToken> sharedToken;
	TokenOperator() {}
	virtual bool incrementToken() = 0;
	void getCurrentToken(std::vector<CharType> &charVector)
	{
		charVector = sharedToken->currentToken;
	}
	virtual ~TokenOperator() {}

};
}}
#endif /* __TOKEN_OPERATOR_H__ */
