/*
 * TokenOperator.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __CORE_ANALYZER_TOKEN_OPERATOR_H__
#define __CORE_ANALYZER_TOKEN_OPERATOR_H__

#include <boost/shared_ptr.hpp>
#include "TokenStreamContainer.h"

namespace srch2 {
namespace instantsearch {

/*
 * TokenOperator can be a Tokenizer or TokenFilter
 */
class TokenStream {
public:
    boost::shared_ptr<TokenStreamContainer> tokenStreamContainer;
    TokenStream():tokenStreamContainer() {
        tokenStreamContainer.reset(new TokenStreamContainer());
    }
    virtual bool processToken() = 0;

    // by default, no state needs to be cleared
    virtual void clearState() {};

    void fillInCharacters(const std::vector<CharType> &charVector){
        tokenStreamContainer->fillInCharacters(charVector);
    }

    /*
     *  isPrefix is a way to inform the analyzer that stop filter should not be applied
     *  the passed string.
     */
    void fillInCharacters(const std::string &str, bool isPrefix = false){
        std::vector<CharType> charVector;
        utf8StringToCharTypeVector(str, charVector); 
        tokenStreamContainer->fillInCharacters(charVector, isPrefix);
    }

    std::vector<CharType> & getProcessedToken() {
        return tokenStreamContainer->currentToken;
    }

    unsigned getProcessedTokenPosition() {
            return tokenStreamContainer->currentTokenPosition;
    }

    unsigned getProcessedTokenCharOffset() {
    	return tokenStreamContainer->currentTokenOffset;
    }

    AnalyzedTokenType getProcessedTokentype() {
    	return tokenStreamContainer->type;
    }

    unsigned getProcessedTokenLen() {
    	return tokenStreamContainer->currentTokenLen;
    }

    bool isEnd() const {
        return tokenStreamContainer->offset >= 
            (tokenStreamContainer->completeCharVector).size();
    }

    const CharType& getCurrentChar() const {
        return (tokenStreamContainer->completeCharVector).at(
                tokenStreamContainer->offset);
    }

    virtual ~TokenStream() {
    }

    // Set of possible (valid) characters for this token stream.  Used to identify "words" (tokens) and where the words end.
    CharSet characterSet;
};
}
}
#endif /* __CORE_ANALYZER__TOKEN_OPERATOR_H__ */
