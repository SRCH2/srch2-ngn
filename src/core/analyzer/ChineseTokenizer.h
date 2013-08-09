/*
 * =====================================================================================
 *
 *       Filename:  ChineseTokenizer.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  08/06/2013 06:29:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */

#ifndef __CORE_ANALYZER__CHINESETOKENIZER_H__
#define __CORE_ANALYZER__CHINESETOKENIZER_H__

#include <string>
#include <utility>
#include <vector>
#include "Tokenizer.h"
#include "Dictionary.h"

namespace srch2
{
namespace instantsearch
{

class ChineseTokenizer: public Tokenizer
{
public:
    ChineseTokenizer(const std::string &dict);
    bool processToken();
    virtual ~ChineseTokenizer() {};
protected:
    bool incrementToken();

    void feedCurrentToken(std::pair<short, short> range);
    bool standardIncrement(unsigned previousType, unsigned currentType, const CharType &currentChar);
    int tokenize(const std::vector<CharType> &sentence, int istart, int istop);
private:
    typedef std::pair<short, short> TokenSpan;
    typedef std::vector<TokenSpan>  TokenBuffer;

    Dictionary  mDict;
    TokenBuffer mBuffer;
    vector<CharType>  *pCurrentToken;
};

}
}
#endif
