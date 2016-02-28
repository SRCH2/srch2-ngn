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
#ifndef __CORE_ANALYZER_CHINESETOKENIZER_H__
#define __CORE_ANALYZER_CHINESETOKENIZER_H__

#include <string>
#include <utility>
#include <vector>
#include "Tokenizer.h"
#include "Dictionary.h"

namespace srch2
{
namespace instantsearch
{

class ChineseDictionaryContainer;

class ChineseTokenizer: public Tokenizer
{
public:
    ChineseTokenizer(const ChineseDictionaryContainer* container);
    bool processToken();
    virtual ~ChineseTokenizer() {};
    virtual void clearState() {     
        mCurrentChineseTokens.clear();
    }

protected:
    bool incrementToken();

private:
    void feedCurrentToken(const std::pair<short, short> &range);
    void tokenize(const std::vector<CharType> &sentence, int istart, int istop);
    bool chineseIncrement(int iChineseStart);
    int  identifyEndOfChineseSequence();
    bool nonChineseIncrement(unsigned currentType, CharType currentChar);

    typedef std::pair<short, short> TokenSpan;  //The [begin,end) position interval of 
                                                // a ChineseToken in the original vector 
    typedef std::vector<TokenSpan>  TokenBuffer;

    const ChineseDictionaryContainer* mChineseDictionaryContainer;
    TokenBuffer mCurrentChineseTokens;
};

}
}
#endif
