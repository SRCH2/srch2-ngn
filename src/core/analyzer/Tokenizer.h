/*
 * Tokenizer.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __CORE_ANALYZER__TOKENIZER_H__
#define __CORE_ANALYZER__TOKENIZER_H__

#include "TokenStream.h"

namespace srch2 {
namespace instantsearch {
/*
 * Tokenizer: tokenize a string
 */
class Tokenizer: public TokenStream {
public:
    Tokenizer() {
    }
protected:
    virtual bool incrementToken() = 0;
    virtual ~Tokenizer() {
    }
};
}
}
#endif /* __CORE_ANALYZER__TOKENIZER_H__*/
