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

    void fillInCharacters(const std::vector<CharType> &charVector){
        tokenStreamContainer->fillInCharacters(charVector);
    }

    void fillInCharacters(const std::string &str){
        std::vector<CharType> charVector;
        utf8StringToCharTypeVector(str, charVector); 
        tokenStreamContainer->fillInCharacters(charVector);
    }

    std::vector<CharType> & getProcessedToken() {
        return tokenStreamContainer->currentToken;
    }

    unsigned getProcessedTokenPosition() {
            return tokenStreamContainer->currentTokenPosition;
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

};
}
}
#endif /* __CORE_ANALYZER__TOKEN_OPERATOR_H__ */
