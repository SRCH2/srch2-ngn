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
