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

#define OTHER_TYPE 0
#define DELIMITER_TYPE 1  //the character whose unicode is <128 but is not latin and is a delimiter, like (  > + etc..
#define LATIN_TYPE 2
#define BOPOMOFO_TYPE 3 // zhuyin is refereed to http://www.ssec.wisc.edu/~tomw/java/unicode.html.

namespace srch2
{
namespace instantsearch
{
///get the character type of one character

class CharSet
{
public:
	static void setRecordAllowedSpecialCharacters(const std::string &recordAllowedSpecialCharacters);
	static const std::string & getRecordAllowedSpecialCharacters();
	static unsigned getCharacterType(CharType c);

private:
	static std::string recordAllowedSpecialCharacters;
};

}}
#endif /* __CORE_ANALYZER__CHARSET_H__*/
