/*
 * WhiteSpaceTokenizer.h
 *
 *  Created on: 2013-5-18
 */
//This class will token the words according to whitespace character.

#ifndef __WHITESPACETOKENIZER_H__
#define __WHITESPACETOKENIZER_H__

#include "Tokenizer.h"
namespace bimaple
{
namespace instantsearch
{
/*
 *  WhiteSpaceTokenizer use DELIMITER_TYPE to separate the string and use LowerCaseFilter to filter the token
 *  For example: "We went to 学校" will be tokenized as "We" "went" "to" and "学校"
 */
class WhiteSpaceTokenizer: public Tokenizer {
public:
	WhiteSpaceTokenizer();
	bool incrementToken();
	virtual ~WhiteSpaceTokenizer();
};
}}
#endif /* __WHITESPACETOKENIZER_H__ */
