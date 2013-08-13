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

    int loadDict(const std::string &dictFilePath);
    bool saveDict(const std::string &dictFilePath);

    short getFreq(const std::vector<CharType> &buffer, unsigned istart, unsigned length);
    short getFreq(const std::string &str);

    bool insert(const std::string &str, short freq);
    
    int getMaxLength()const{
        return mMaxWordLength;
    }
private:
    typedef std::map<std::string, short> WordFrequencyMap;

    int mMaxWordLength;
    WordFrequencyMap mWordFrequencyMap;
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
