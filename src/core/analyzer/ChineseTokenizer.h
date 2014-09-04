//$Id$

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
