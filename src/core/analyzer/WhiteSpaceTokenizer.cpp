/*
 * WhiteSpaceTokenizer.cpp
 *
 *  Created on: 2013-5-18
 */

#include "WhiteSpaceTokenizer.h"
#include "util/Assert.h"

namespace srch2 {
namespace instantsearch {
WhiteSpaceTokenizer::WhiteSpaceTokenizer()
    :Tokenizer()
{}

bool WhiteSpaceTokenizer::incrementToken() {
    (tokenStreamContainer->currentToken).clear();
    while (true) {
        if ((tokenStreamContainer->offset)
                < (tokenStreamContainer->completeCharVector).size()) {
            CharType c =
                    (tokenStreamContainer->completeCharVector)[tokenStreamContainer->offset];
            (tokenStreamContainer->offset)++;
            switch (CharSet::getCharacterType(c)) {
            case CharSet::DELIMITER_TYPE:
                if (!(tokenStreamContainer->currentToken).empty()) {
                	tokenStreamContainer->currentTokenPosition++;
                    return true;
                } else {
                    (tokenStreamContainer->offset)++;
                }
                break;
            default:
                (tokenStreamContainer->currentToken).push_back(c);
                break;
            }
        } else {
        	tokenStreamContainer->currentTokenPosition++;
            return (!(tokenStreamContainer->currentToken).empty()) ? true : false;
        }
    }
    ASSERT(false);
    return false;
}

bool WhiteSpaceTokenizer::processToken() {
    return this->incrementToken();
}

WhiteSpaceTokenizer::~WhiteSpaceTokenizer() {
    // TODO Auto-generated destructor stub
}
}
}

