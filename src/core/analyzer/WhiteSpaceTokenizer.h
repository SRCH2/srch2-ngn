/*
 * WhiteSpaceTokenizer.h
 *
 *  Created on: 2013-5-18
 */
//This class will token the words according to whitespace character.
#ifndef __CORE_ANALYZER_WHITESPACETOKENIZER_H__
#define __CORE_ANALYZER_WHITESPACETOKENIZER_H__

#include "Tokenizer.h"
namespace srch2 {
namespace instantsearch {
/*
 *  WhiteSpaceTokenizer use DELIMITER_TYPE to separate the string and use LowerCaseFilter to filter the token
 *  For example: "We went to 学校" will be tokenized as "We" "went" "to" and "学校"
 */
class WhiteSpaceTokenizer: public Tokenizer {
public:
    WhiteSpaceTokenizer();
    void clearState() {};
    bool incrementToken();
    bool processToken();
    virtual ~WhiteSpaceTokenizer();
};
}
}
#endif /* __CORE_ANALYZER__WHITESPACETOKENIZER_H__ */
