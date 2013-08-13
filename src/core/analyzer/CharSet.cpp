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
/// The Chinese oriented code point is refereed to 
///  http://grepcode.com/file/repo1.maven.org/maven2/org.apache.lucene/lucene-analyzers-smartcn/4.3.1/org/apache/lucene/analysis/cn/smart/Utility.java#Utility.getCharType%28char%29
unsigned CharSet::getCharacterType(CharType c)
{
    if((c>= 65 && c <= 90) ||(c>= 97 && c <= 122)||(c >= 48 && c <= 57)||(128 <= c && c <= 591)
    		||std::find(recordAllowedSpecialCharacters.begin(), recordAllowedSpecialCharacters.end(), (char)c) != recordAllowedSpecialCharacters.end())
		return LATIN_TYPE;
	else if(c < 128)
		return DELIMITER_TYPE;
	else if(c >= 12549 && c <= 12588)
		return BOPOMOFO_TYPE;
    else if((c >= 0x0021 && c <= 0x00BB) || (c >= 0x2010 && c <= 0x2642)    // Some more punctuations
        || (c >= 0x3001 && c <= 0x301E) || (c >= 0xFE30 && c <= 0xFF63))
        return DELIMITER_TYPE;
    else if((c >= 0xFF21 && c <= 0xFF3A) || (c >= 0xFF41 && c <= 0xFF5A)    // FULLWIDTH_LETTER
            ||(c >= 0xFF10 && c <= 0xFF19))                                 // FULLWIDTH_DIGIT
        return LATIN_TYPE;                //We treat these full-width characters the same as as half-width latin characters.
    else if (c >= 0x4E00 && c <= 0x9FA5)  //The common Chinese character
        return HANZI_TYPE;
    else
		return OTHER_TYPE;
}

}}
