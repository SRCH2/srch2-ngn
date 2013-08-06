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
StandardTokenizer::StandardTokenizer() {
    tokenStreamContainer.reset(new TokenStreamContainer());
}

bool StandardTokenizer::incrementToken() {
    (tokenStreamContainer->currentToken).clear();
    CharType previousChar = (CharType) ' ';
    //originally, set the previous character is ' ';
    while (true) {
        ///check whether the scanning is over.
        if ((tokenStreamContainer->offset)
                >= (tokenStreamContainer->completeCharVector).size()) {
            return (!(tokenStreamContainer->currentToken).empty()) ?
                    true : false;
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
        case DELIMITER_TYPE:
            if (!(tokenStreamContainer->currentToken).empty()) {
                return true;
            }
            break;
        case LATIN_TYPE:
        case BOPOMOFO_TYPE:
            //check if the types of previous character and  current character are the same
            if (previousCharacterType == currentCharacterType) {
                (tokenStreamContainer->currentToken).push_back(currentChar);
            } else {
                if (!(tokenStreamContainer->currentToken).empty()) //if the currentToken is not null, we need produce the token
                {
                    (tokenStreamContainer->offset)--;
                    return (!(tokenStreamContainer->currentToken).empty()) ?
                            true : false;
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
            return (!(tokenStreamContainer->currentToken).empty()) ?
                    true : false;
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

