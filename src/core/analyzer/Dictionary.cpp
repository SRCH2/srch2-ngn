//$Id$

#include "Dictionary.h"
#include <utility>
#include "util/Logger.h"
#include <stdio.h>

using namespace std;
using namespace srch2::util;

namespace srch2{
namespace instantsearch{

Dictionary::Dictionary():mMaxWordLength(0),mWordFrequencyMap(){
}

int Dictionary::loadDict(const string &dictFilePath){
  FILE *fp = fopen(dictFilePath.c_str(), "rb");
  if(fp == NULL){
    Logger::error("Dictionary path: %s can not open", dictFilePath.c_str());
    return -1;
  }  

  // load the leading key at the beginning of the file
  int leadingKey;
  if (fread(&leadingKey, 1, sizeof(leadingKey), fp) != sizeof(leadingKey)) {
    Logger::error("Error while reading the file %s ", dictFilePath.c_str());
    return -1;      
  }

  if (leadingKey != DICTIONARY_LEADING_KEY) {
    Logger::error("Wrong dictionary file ", dictFilePath.c_str());
    return -1;      
  }

  // load mMaxWordLength
  if (fread(&mMaxWordLength, 1, sizeof(mMaxWordLength), fp) 
      != sizeof(mMaxWordLength)) {
    Logger::error("Error while reading the file %s ", dictFilePath.c_str());
    return -1;      
  }
  
  // load the map of word frequencies
  mWordFrequencyMap.clear(); 
  
  int mapSize;
  if (fread(&mapSize, 1, sizeof(mapSize), fp) != sizeof(mapSize)) {
    Logger::error("Error while reading the file %s ", dictFilePath.c_str());
    return -1;      
  }

  int wordNum = 0;
  while (wordNum < mapSize) {
    // unscramble each byte in the string
    int len;
    if (fread(&len, 1, sizeof(len), fp) != sizeof(len)) {
      Logger::error("Error while reading the file %s ", dictFilePath.c_str());
      return -1;      
    }

    // serialize the scrambled bytes in the string
    string word;
    char ch;
    for (int i = 0; i < len; i ++) {
      if (fread(&ch, 1, sizeof(ch), fp) != sizeof(ch)) {
        Logger::error("Error while reading the file %s ", dictFilePath.c_str());
        return -1;      
      }
      word.append(1, unscrambleChar(ch));
    }

    short scrambledFreq;
    if (fread(&scrambledFreq, 1, sizeof(scrambledFreq), fp) != sizeof(scrambledFreq)) {
      Logger::error("Error while reading the file %s ", dictFilePath.c_str());
      return -1;      
    }

    // uncramble the frequency
    this->insert(word, unscrambleShort(scrambledFreq));

    wordNum ++;
  }
  
  fclose(fp);
  return mWordFrequencyMap.size();
}


char Dictionary::scrambleChar(char oldChar){
  return oldChar ^ SCRAMBLE_CHAR_MASK;
}

// since we use XOR for scrambing a byte,
// we can unscramble it using XOR with the same mask
char Dictionary::unscrambleChar(char oldChar){
  return oldChar ^ SCRAMBLE_CHAR_MASK;
}

short Dictionary::scrambleShort(short oldShort){
  return oldShort ^ SCRAMBLE_SHORT_MASK;
}

short Dictionary::unscrambleShort(short oldShort){
  return oldShort ^ SCRAMBLE_SHORT_MASK;
}

  // main idea: to protect these words and frequencies, we scramble
  // the strings and frequencies using an XOR operation with a mask.
  // The masks are known only by us.
bool Dictionary::saveDict(const string &dictFilePath){
    FILE *fp;
    fp = fopen(dictFilePath.c_str(), "w+b");
    if (fp == NULL)
        return false;

    // write the leading key at the beginning of the file
    int leadingKey = DICTIONARY_LEADING_KEY;
    fwrite(&leadingKey, 1, sizeof(leadingKey), fp);

    // save mMaxWordLength
    fwrite (&mMaxWordLength, 1, sizeof(mMaxWordLength), fp);

    // save the map of word frequencies
    int mapSize = mWordFrequencyMap.size();
    fwrite (&mapSize, 1, sizeof(mapSize), fp);

    for (WordFrequencyMap::const_iterator it = mWordFrequencyMap.begin(); 
        it != mWordFrequencyMap.end(); it++) {
      string word = it->first;
      short freq = it->second;

      // use a mask to scramble each byte in the string
      int len = word.length();
      fwrite (&len, 1, sizeof(len), fp);      

      // serialize the scrambled bytes in the string
      for (int i = 0; i < len; i ++) {
        char ch = scrambleChar(word[i]);
        fwrite(&ch, 1, sizeof(ch), fp);
      }

      short scrambledFreq = scrambleShort(freq);
      fwrite(&scrambledFreq, 1, sizeof(scrambledFreq), fp);
    }

    fclose(fp);
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
