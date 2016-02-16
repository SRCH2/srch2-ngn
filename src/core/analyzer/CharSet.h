/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * CharSet.h
 *
 *  Created on: 2013-5-19
 */

#ifndef __CORE_ANALYZER_CHARSET_H__
#define __CORE_ANALYZER_CHARSET_H__
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "src/core/util/encoding.h"

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
