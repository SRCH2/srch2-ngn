/*
 * WhiteSpaceTokenizer.cpp
 *
 *  Created on: 2013-5-18
 */

#include "WhiteSpaceTokenizer.h"

namespace srch2
{
namespace instantsearch
{
WhiteSpaceTokenizer::WhiteSpaceTokenizer()
{
	sharedToken.reset(new SharedToken());
}

bool WhiteSpaceTokenizer::incrementToken()
{
	(sharedToken->currentToken).clear();
    while(true)
    {
    	if((sharedToken->offset) < (sharedToken->completeCharVector).size())
		{
			CharType c = (sharedToken->completeCharVector)[sharedToken->offset];
			(sharedToken->offset)++;
			switch(CharSet::getCharacterType(c))
			{
			case DELIMITER_TYPE:
				if(!(sharedToken->currentToken).empty())
				{
					return emitToken();
				}
				else
					(sharedToken->offset)++;
				break;
			default:
				(sharedToken->currentToken).push_back(c);
				break;
			}
		}
		else
		{
			return emitToken();
		}
    }
}
WhiteSpaceTokenizer::~WhiteSpaceTokenizer() {
	// TODO Auto-generated destructor stub
}
}}

