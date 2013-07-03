/*
 * StandardTokenizer.cpp
 *
 *  Created on: 2013-5-17
 */

#include <iostream>
#include "StandardTokenizer.h"

namespace bimaple
{
namespace instantsearch
{
StandardTokenizer::StandardTokenizer()
{
	sharedToken.reset(new SharedToken());
}

bool StandardTokenizer::incrementToken()
{
	(sharedToken->currentToken).clear();
	CharType previousChar = (CharType)' ';
    //originally, set the previous character is ' ';
    while(true)
    {
    	///check whether the scanning is over.
    	if((sharedToken->offset) >= (sharedToken->completeCharVector).size())
    		return emitToken();
    	CharType currentChar = (sharedToken->completeCharVector)[sharedToken->offset];
        if((sharedToken->offset)-1 >= 0)//check whether the previous character exists.
        {
        	previousChar = (sharedToken->completeCharVector)[(sharedToken->offset)-1];
        }
        (sharedToken->offset)++;
        ///we need combine previous character and current character to decide a word
    	unsigned previousCharacterType = CharSet::getCharacterType(previousChar);
    	unsigned currentCharacterType = CharSet::getCharacterType(currentChar);

        switch(currentCharacterType)
        {
        case DELIMITER_TYPE:
        	if(!(sharedToken->currentToken).empty())
        	{
        		return emitToken();
        	}
            break;
        case LATIN_TYPE:
        case BOPOMOFO_TYPE:
        	//check if the types of previous character and  current character are the same
        	if(previousCharacterType == currentCharacterType)
        	{
            	(sharedToken->currentToken).push_back(currentChar);
        	}
        	else
        	{
				if(!(sharedToken->currentToken).empty())//if the currentToken is not null, we need produce the token
				{
					(sharedToken->offset)--;
					return emitToken();
				}
				else
					(sharedToken->currentToken).push_back(currentChar);
        	}
			break;
        default://other character type
        	if(!(sharedToken->currentToken).empty())
        	{
        		(sharedToken->offset)--;
        	}
        	else
        		(sharedToken->currentToken).push_back(currentChar);
        	return emitToken();
        }
    }
}

StandardTokenizer::~StandardTokenizer() {
	// TODO Auto-generated destructor stub
}
}}

