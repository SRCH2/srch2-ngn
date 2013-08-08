/*
 * TokenFilter.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __CORE_ANALYZER__TOKENFILTER_H__
#define __CORE_ANALYZER__TOKENFILTER_H__

#include "TokenStream.h"
namespace srch2 {
namespace instantsearch {
/*
 * TokenFilter: filter a token
 */
class TokenFilter: public TokenStream {
public:
    TokenFilter(TokenStream* tokenStream) {
        this->tokenStream = tokenStream;
    }
    virtual ~TokenFilter() {
        delete tokenStream;
    }
protected:
    // a linker to a TokenFilter or a Tokenizer
    TokenStream* tokenStream;
};
}
}
#endif /* __CORE_ANALYZER__TOKENFILTER_H__ */
