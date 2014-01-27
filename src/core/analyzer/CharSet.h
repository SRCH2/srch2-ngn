/*
 * CharSet.h
 *
 *  Created on: 2013-5-19
 */

#ifndef __CORE_ANALYZER_CHARSET_H__
#define __CORE_ANALYZER_CHARSET_H__
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "util/encoding.h"

namespace srch2
{
namespace instantsearch
{
///get the character type of one character

class CharSet
{
public:
    enum CharacterType{
        OTHER_TYPE,
        DELIMITER_TYPE, // In general a character whose unicode is < 128, but not a latin character, 
                        //  then it is a delimiter, such as "( > + ..
                        // In Chinese, there are more delimiters such as " ，", "。", and "！". 
                        //  They are also treated as a delimiter.
                        
        LATIN_TYPE,     // In general all the English alphabet and digits are LATIN_TYPE,
                        // In Chinese, there are FullWidth English alphabet and digit which are also
                        //  treated as LATIN_TYPE, such as ＡＺ１３
                        
        BOPOMOFO_TYPE,  // Used for the Bopomofo Zhuyin system in Taiwan, 
                        //  http://www.ssec.wisc.edu/~tomw/java/unicode.html
                        
        HANZI_TYPE,      // Chinese Hanzi character
        WHITESPACE        // simple whitespace, tab
    };
    unsigned getCharacterType(const CharType c) const;

    void setRecordAllowedSpecialCharacters(const std::string &recordAllowedSpecialCharacters);
    const std::string & getRecordAllowedSpecialCharacters();

    CharSet() : recordAllowedSpecialCharacters("") {};
    ~CharSet() {};

private:
    std::string recordAllowedSpecialCharacters;
};

}}
#endif /* __CORE_ANALYZER__CHARSET_H__*/
