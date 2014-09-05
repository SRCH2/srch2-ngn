/*
 * TokenFilter.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __CORE_ANALYZER_TOKENFILTER_H__
#define __CORE_ANALYZER_TOKENFILTER_H__

#include "TokenStream.h"
namespace srch2 {
namespace instantsearch {
/*
 * TokenFilter: filter a token
 */
class TokenFilter: public TokenStream {
public:
    TokenFilter(TokenStream* tokenStream)
    {
        this->tokenStream = tokenStream;
    }
    virtual ~TokenFilter() {
        delete tokenStream;
    }

    virtual void clearState() {
      // clear the state of the filter in the upstream
      if (this->tokenStream != NULL)
        this->tokenStream->clearState();

      // clear our own states: nothing to do by default.
    }

protected:
    // a linker to a TokenFilter or a Tokenizer
    TokenStream* tokenStream;
};
}
}
#endif /* __CORE_ANALYZER__TOKENFILTER_H__ */
