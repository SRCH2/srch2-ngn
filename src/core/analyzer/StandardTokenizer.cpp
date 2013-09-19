/*
 * StandardTokenizer.cpp
 *
 *  Created on: 2013-5-17
 */

#include <iostream>
#include "StandardTokenizer.h"
#include "util/Assert.h"

namespace srch2 {
namespace instantsearch {
StandardTokenizer::StandardTokenizer()
    :Tokenizer()
{}

bool StandardTokenizer::incrementToken() {
    (tokenStreamContainer->currentToken).clear();
    CharType previousChar = (CharType) ' ';
    //originally, set the previous character is ' ';
    while (true) {
        ///check whether the scanning is over.
        if ((tokenStreamContainer->offset)
                >= (tokenStreamContainer->completeCharVector).size()) {

            if (tokenStreamContainer->currentToken.empty()) {
            	return false;
            } else {
            	tokenStreamContainer->currentTokenPosition++;
            	return true;
            }
        }
        CharType currentChar =
                (tokenStreamContainer->completeCharVector)[tokenStreamContainer->offset];
        if ((tokenStreamContainer->offset) - 1 >= 0) //check whether the previous character exists.
                {
            previousChar =
                    (tokenStreamContainer->completeCharVector)[(tokenStreamContainer->offset)
                            - 1];
        }
        (tokenStreamContainer->offset)++;
        ///we need combine previous character and current character to decide a word
        unsigned previousCharacterType = CharSet::getCharacterType(
                previousChar);
        unsigned currentCharacterType = CharSet::getCharacterType(currentChar);

        switch (currentCharacterType) {
        case CharSet::DELIMITER_TYPE:
            if (!(tokenStreamContainer->currentToken).empty()) {
            	tokenStreamContainer->currentTokenPosition++;
                return true;
            }
            break;
        case CharSet::LATIN_TYPE:
        case CharSet::BOPOMOFO_TYPE:
            //check if the types of previous character and  current character are the same
            if (previousCharacterType == currentCharacterType) {
                (tokenStreamContainer->currentToken).push_back(currentChar);
            } else {
                if (!(tokenStreamContainer->currentToken).empty()) //if the currentToken is not null, we need produce the token
                {
                    (tokenStreamContainer->offset)--;
                    tokenStreamContainer->currentTokenPosition++;
                    return true;
                } else
                    (tokenStreamContainer->currentToken).push_back(currentChar);
            }
            break;
        default: //other character type
            if (!(tokenStreamContainer->currentToken).empty()) {
                (tokenStreamContainer->offset)--;
            } else {
                (tokenStreamContainer->currentToken).push_back(currentChar);
            }
            tokenStreamContainer->currentTokenPosition++;
            if (tokenStreamContainer->currentToken.empty()) {
            	return false;
            } else {
            	tokenStreamContainer->currentTokenPosition++;
            	return true;
            }
        }
    }
    ASSERT(false);
    return false;
}

bool StandardTokenizer::processToken() {
    return this->incrementToken();
}

StandardTokenizer::~StandardTokenizer() {
    // TODO Auto-generated destructor stub
}
}
}

