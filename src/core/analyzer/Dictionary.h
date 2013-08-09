/*
 * =====================================================================================
 *
 *       Filename:  Dictionary.h
 *
 *    Description:  The dictionary structure used by ChineseTokenizer
 *
 *        Version:  1.0
 *        Created:  08/09/2013 01:57:06 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jianfeng (), jianfengjia@srch2.com
 *   Organization:  SRCH2.com
 *
 * =====================================================================================
 */

#ifndef __CORE_ANALYZER__DICTIONARY_H__
#define __CORE_ANALYZER__DICTIONARY_H__

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
    static const short INVALID_FREQ = -1;

    Dictionary();
    Dictionary(const string &dictPath);

    int loadDict(const string &dictPath);
    bool saveDict(const string &dictPath);

    short getFreq(const std::vector<CharType> &buffer, unsigned istart, unsigned length);
    short getFreq(const std::string &str);

    bool insert(const std::string &str, short freq);
    
    int getMaxLength()const{
        return mMaxWordLength;
    }
private:
    typedef std::map<std::string, short> HashMap;

    int mMaxWordLength;
    HashMap mHashMap;
protected:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & mMaxWordLength;
		ar & mHashMap;
	}
};


}
}

#endif
