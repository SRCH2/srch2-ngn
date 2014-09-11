//$Id$

#include "Dictionary.h"
#include <utility>
#include <fstream>
#include "util/Logger.h"

using namespace std;
using namespace srch2::util;

namespace srch2{
namespace instantsearch{

Dictionary::Dictionary():mMaxWordLength(0),mWordFrequencyMap(){
}

int Dictionary::loadDict(const string &dictFilePath){
    ifstream ifs(dictFilePath.c_str(), ios::binary);
    if( !ifs.is_open()){
        Logger::error("Dictionary path: %s can not open", dictFilePath.c_str());
        return -1;
    }
    boost::archive::binary_iarchive ia(ifs);
    ia >> *this;
    ifs.close();
    return mWordFrequencyMap.size();
}

bool Dictionary::saveDict(const string &dictFilePath){
    ofstream ofs(dictFilePath.c_str(), ios::binary);
    if ( !ofs.is_open()){
        return false;
    }
    boost::archive::binary_oarchive oa(ofs);
    oa << *this;
    ofs.close();
    return true;
}

short Dictionary::getFreq(const vector<CharType> &buffer, unsigned istart, unsigned length) const{
    if (length < 1 || istart + length > buffer.size()){
        return INVALID_WORD_FREQ;
    }
    string utf8String; 
    charTypeVectorToUtf8String(buffer, istart, istart+length, utf8String);
    return getFreq(utf8String);
}

short Dictionary::getFreq(const string &utf8String) const{
    WordFrequencyMap::const_iterator it = mWordFrequencyMap.find(utf8String);
    if ( it == mWordFrequencyMap.end()){
        return INVALID_WORD_FREQ;
    } else {
        return it->second;
    }
}

bool Dictionary::insert(const string &utf8String, short freq){
    pair<WordFrequencyMap::iterator, bool> result = mWordFrequencyMap.insert( make_pair<string, short>(utf8String,freq));
    if(result.second){
        int length = getUtf8StringCharacterNumber(utf8String);
        if (length > mMaxWordLength){
            mMaxWordLength = length;
        }
    }
    return result.second;
}

}
}
