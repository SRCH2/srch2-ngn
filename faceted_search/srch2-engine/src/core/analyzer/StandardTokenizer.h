/*
 * StandardTokenizer.h
 *
 *  Created on: 2013-5-17
 */
//This class will token the words according to whitespace character and character which is >=256 .

#ifndef __STANDARDTOKENIZER_H__
#define __STANDARDTOKENIZER_H__

#include <vector>
#include <iostream>
#include "Tokenizer.h"

using std::vector;

namespace srch2
{
namespace instantsearch
{
/*
 *  StandardTokenizer use DELIMITER_TYPE to separate the LATIN_TYPE string and BOPOMOFO_TYPE.
 *  for OTHER_TYPE, each character is a token
 *  For example: "We went to 学校" will be tokenized as "We" "went" "to" "学" and "校"
 */
class StandardTokenizer:public Tokenizer {
public:
	StandardTokenizer();
	bool incrementToken();
	virtual ~StandardTokenizer();
};
}}
#endif /* __STANDARDTOKENIZER_H__ */
