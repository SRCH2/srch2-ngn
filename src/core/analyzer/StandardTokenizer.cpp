/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
    // CharOffset starts from 1.
    tokenStreamContainer->currentTokenOffset = tokenStreamContainer->offset + 1;
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
        unsigned previousCharacterType = characterSet.getCharacterType(previousChar);
        unsigned currentCharacterType = characterSet.getCharacterType(currentChar);

        switch (currentCharacterType) {
        case CharSet::WHITESPACE:
            if (!(tokenStreamContainer->currentToken).empty()) {
            	tokenStreamContainer->currentTokenPosition++;
                return true;
            }
            tokenStreamContainer->currentTokenOffset++;
            break;
        case CharSet::LATIN_TYPE:
        case CharSet::BOPOMOFO_TYPE:
        case CharSet::DELIMITER_TYPE:
            //check if the types of previous character and  current character are the same
            if (previousCharacterType == currentCharacterType) {
                (tokenStreamContainer->currentToken).push_back(currentChar);
            } else if (previousCharacterType  == CharSet::DELIMITER_TYPE ||
            		currentCharacterType == CharSet::DELIMITER_TYPE) {
            	/*
            	 *  delimiters will go with both LATIN and BOPPMOFO types.
            	 *  e.g for C++  C is Latin type and + is Delimiter type. We do not want to split
            	 *  them into to C and ++.
            	 *
            	 *  We also do not want to tokenize "c+b" because NonalphaNumericFilter will tokenize
            	 *  it later.
            	 */
            	(tokenStreamContainer->currentToken).push_back(currentChar);
            }else {
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
    tokenStreamContainer->type = ANALYZED_ORIGINAL_TOKEN;
    return this->incrementToken();
}

StandardTokenizer::~StandardTokenizer() {
    // TODO Auto-generated destructor stub
}
}
}

