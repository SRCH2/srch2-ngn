/*
 * =====================================================================================
 *
 *       Filename:  Dictionary.cpp
 *
 *    Description:  Implementation of Dictionary
 *
 *        Version:  1.0
 *        Created:  08/09/2013 02:32:31 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jianfeng (), jianfengjia@srch2.com
 *   Organization:  SRCH2.com
 *
 * =====================================================================================
 */

#include "Dictionary.h"
#include <utility>
#include <fstream>
#include "util/encoding.h"
#include "util/Logger.h"

using namespace std;
using namespace srch2::util;

namespace srch2{
namespace instantsearch{

Dictionary::Dictionary():mMaxWordLength(0),mHashMap(){
}

Dictionary::Dictionary(const string &dictPath)
    :mMaxWordLength(0),mHashMap(){
    int icount = loadDict(dictPath);
    Logger::debug("loadDict count is %d", icount);
}

int Dictionary::loadDict(const string &dictPath){
    ifstream ifs(dictPath.c_str(), ios::binary);
    if( !ifs.is_open()){
        Logger::error("Dictionary path: %s can not open", dictPath.c_str());
        return -1;
    }
    boost::archive::binary_iarchive ia(ifs);
    ia >> *this;
    ifs.close();
    return mHashMap.size();
}

bool Dictionary::saveDict(const string &dictPath){
    ofstream ofs(dictPath.c_str(), ios::binary);
    if ( !ofs.is_open()){
        return false;
    }
    boost::archive::binary_oarchive oa(ofs);
    oa << *this;
    ofs.close();
    return true;
}

short Dictionary::getFreq(const vector<CharType> &buffer, unsigned istart, unsigned length){
    if (length < 1 || istart + length > buffer.size()){
        return INVALID_FREQ;
    }
    string utf8String; 
    charTypeVectorToUtf8String(buffer, istart, istart+length, utf8String);
    return getFreq(utf8String);
}

short Dictionary::getFreq(const string &str){
    HashMap::const_iterator it = mHashMap.find(str);
    if ( it == mHashMap.end()){
        return INVALID_FREQ;
    } else {
        return it->second;
    }
}

bool Dictionary::insert(const string &str, short freq){
    pair<HashMap::iterator, bool> result = mHashMap.insert( make_pair<string, short>(str,freq));
    if(result.second){
        if (str.length() > mMaxWordLength){
            mMaxWordLength = str.length();
        }
    }
    return result.second;
}

}
}
