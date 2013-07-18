/*
 * CharSet.cpp
 *
 *  Created on: 2013-5-19
 */

#include "CharSet.h"

namespace srch2
{
namespace instantsearch
{

std::string CharSet::recordAllowedSpecialCharacters = "";

void CharSet::setRecordAllowedSpecialCharacters(const std::string &recordAllowedSpecialCharacters)
{
	CharSet::recordAllowedSpecialCharacters = recordAllowedSpecialCharacters;
}

const std::string& CharSet::getRecordAllowedSpecialCharacters()
{
	return recordAllowedSpecialCharacters;
}

/// the unicode  point of  591 is refereed to http://www.ssec.wisc.edu/~tomw/java/unicode.html.
unsigned CharSet::getCharacterType(CharType c)
{
    if((c>= 65 && c <= 90) ||(c>= 97 && c <= 122)||(c >= 48 && c <= 57)||(128 <= c && c <= 591)
    		||std::find(recordAllowedSpecialCharacters.begin(), recordAllowedSpecialCharacters.end(), (char)c) != recordAllowedSpecialCharacters.end())
		return LATIN_TYPE;
	else if(c < 128)
		return DELIMITER_TYPE;
	else if(c >= 12549 && c <= 12588)
		return BOPOMOFO_TYPE;
	else
		return OTHER_TYPE;
}

}}
