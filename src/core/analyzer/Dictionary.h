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
//$Id$

#ifndef __CORE_ANALYZER_DICTIONARY_H__
#define __CORE_ANALYZER_DICTIONARY_H__

#include <map>
#include <vector>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include "util/encoding.h"

namespace srch2{
namespace instantsearch{

class Dictionary{
public:
    static const short INVALID_WORD_FREQ = -1;

    Dictionary();

    bool saveDict(const std::string &dictFilePath);
    int loadDict(const std::string &dictFilePath);

    short getFreq(const std::vector<CharType> &buffer, unsigned istart, unsigned length) const;
    short getFreq(const std::string &str) const;

    bool insert(const std::string &str, short freq);
    
    int getMaxWordLength() const {
        return mMaxWordLength;
    }
private:
    typedef std::map<std::string, short> WordFrequencyMap;

    int mMaxWordLength;
    WordFrequencyMap mWordFrequencyMap;

#define SCRAMBLE_CHAR_MASK (0x65) // mask used to scramble bytes
#define SCRAMBLE_SHORT_MASK (0x710F) // mask used to scramble shorts
#define DICTIONARY_LEADING_KEY (1894704) // unique identifier for SRCH2 dictionaries
 
    char scrambleChar(char oldChar);
    char unscrambleChar(char oldChar);
    short scrambleShort(short oldShort);
    short unscrambleShort(short oldShort);

protected:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & mMaxWordLength;
		ar & mWordFrequencyMap;
	}
};


}
}

#endif
