/*
 * CharSet.h
 *
 *  Created on: 2013-5-19
 */

#ifndef __CORE_ANALYZER__CHARSET_H__
#define __CORE_ANALYZER__CHARSET_H__
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
        DELIMITER_TYPE, //the character whose unicode is <128 
                        //but is not latin and is a delimiter, 
                        //like (  > + etc..
        LATIN_TYPE,     
        BOPOMOFO_TYPE,  //zhuyin:http://www.ssec.wisc.edu/~tomw/java/unicode.html
        HANZI_TYPE
    };

	static void setRecordAllowedSpecialCharacters(const std::string &recordAllowedSpecialCharacters);
	static const std::string & getRecordAllowedSpecialCharacters();
	static unsigned getCharacterType(CharType c);

private:
	static std::string recordAllowedSpecialCharacters;
};

}}
#endif /* __CORE_ANALYZER__CHARSET_H__*/
