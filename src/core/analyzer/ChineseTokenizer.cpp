/*
 * =====================================================================================
 *
 *       Filename:  ChineseTokenizer.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/07/2013 10:32:30 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "ChineseTokenizer.h"

#include <utility>  // std::pair
#include "CharSet.h"
#include "util/Assert.h"

using namespace std;
const int UNKNOWN_CHAR_FREQ = 2000;
const int TOKEN_LENGTH_PENALTY = 150;
const int MAXIMUM_FREQ = 1000000;

namespace srch2{
namespace instantsearch{

ChineseTokenizer::ChineseTokenizer(const string &dictPath)
    :mDict(dictPath), mBuffer(){
    mBuffer.reserve(32);
    tokenStreamContainer.reset(new TokenStreamContainer());
    pCurrentToken = &(tokenStreamContainer->currentToken);
}

bool ChineseTokenizer::processToken(){
    if ( mBuffer.size() == 0 ){
        // mBuffer is empty, it's time to incrementToken
        if( !this->incrementToken() ){
            //has chinese token
            //try chinese buffer
            if ( mBuffer.size() == 0){
                return false;
            }
            feedCurrentToken(mBuffer.back());
            mBuffer.pop_back();
        }
    }else{
        feedCurrentToken(mBuffer.back());
        mBuffer.pop_back();
    }
    return true;
}

/**
 * Generate more token and update the currentToken
 * 1. It will generate multiple Chinese Token when meet Chinese characters.
 *    And stored them into mBuffer in the reversed order. 
 *    It will NOT update the currentToken in this case.
 * 2. The non-Chinese token will be treated the same as StandardTokenizer. 
 * @return :
 *    true: if the currentToken was set to a new non-Chinese token.
 *    flase: there is no non-Chinese token in the coming stream. 
 */
bool ChineseTokenizer::incrementToken(){
    pCurrentToken->clear();
    unsigned previousType = (unsigned)-1;
    unsigned currentType = (unsigned)-1;
    unsigned iChineseStart = 0;
    unsigned iChineseStop = 0;

    while(true){
        if ((tokenStreamContainer->offset)
                >= (tokenStreamContainer->completeCharVector).size()){
            if(iChineseStop < iChineseStart){
                iChineseStop = (tokenStreamContainer->completeCharVector).size();
                tokenize(tokenStreamContainer->completeCharVector, iChineseStart, iChineseStop);
                return false;
            }
            return pCurrentToken->empty() ?
                    false : true;
        }
        CharType currentChar = 
            (tokenStreamContainer->completeCharVector)[tokenStreamContainer->offset];
        previousType = currentType;
        currentType = CharSet::getCharacterType(currentChar);
        ++(tokenStreamContainer->offset);

        if (currentType == CharSet::HANZI_TYPE){
            if (previousType != CharSet::HANZI_TYPE){
                iChineseStart = tokenStreamContainer->offset - 1;
            }
            if (pCurrentToken->empty()){    
                // The first char is Chinese, continue to ChineseToken logic
                continue;
            } else { // Finish the non-Chinese token logic
                --(tokenStreamContainer->offset);
                return true;
            }
        }

        // currentType != CharSet::HANZI_TYPE
        if (previousType == CharSet::HANZI_TYPE){
            iChineseStop = tokenStreamContainer->offset - 1;
            tokenize(tokenStreamContainer->completeCharVector, iChineseStart, iChineseStop);
            --(tokenStreamContainer->offset);
            return false;
        }
    
        if(standardIncrement(previousType, currentType, currentChar)){
            return true;
        }else{  // meet Chinese or finished stream
            continue;
        }
    }
}

bool ChineseTokenizer::standardIncrement(unsigned previousType, unsigned currentType, 
        const CharType &currentChar){
    switch(currentType){
        case CharSet::DELIMITER_TYPE:
            if (!pCurrentToken->empty()){
                return true;
            }
            break;
        case CharSet::LATIN_TYPE:
        case CharSet::BOPOMOFO_TYPE:
            if (previousType == currentType){
                pCurrentToken->push_back(currentChar);
            } else {
                if (pCurrentToken->empty()){
                    pCurrentToken->push_back(currentChar);
                }else{
                    --(tokenStreamContainer->offset);
                    return true;
                }
            }
            break;
        case CharSet::HANZI_TYPE: 
            if (!pCurrentToken->empty()){ // Finished the standard logic
                --(tokenStreamContainer->offset);
                return true;
            } else {
                // should not happen;
                ASSERT(false);
            }
            break;
        default:
            if (pCurrentToken->empty()){
                pCurrentToken->push_back(currentChar);
            } else {
                --(tokenStreamContainer->offset);
            }
            return true;
    }   
    return false;
}



void ChineseTokenizer::feedCurrentToken(pair<short, short> range){
    pCurrentToken->clear();
    for(short i = range.first; i < range.second; i++){
        pCurrentToken->push_back(
                (tokenStreamContainer->completeCharVector)[i]);
    }
}

int ChineseTokenizer::tokenize(const vector<CharType> &sentence, int istart, int istop){
    mBuffer.clear();  
    if (istop <= istart){
        return 0;
    }
    int size = istop - istart + 1; 
    int *positionScore = new int[size];
    int *bestPosition = new int[size];
    memset(positionScore, MAXIMUM_FREQ, sizeof(int)*(size));
    memset(bestPosition, -1, sizeof(int)*(size));
    positionScore[0] = 0;
    bestPosition[0] = 0;

    for(int i = 1; i < size; ++i){
        for(int span = 1; span < mDict.getMaxLength() && span <= i; ++span){
            short freq = mDict.getFreq(sentence, (unsigned)(istart + i-span),(unsigned) span);
            if (freq == Dictionary::INVALID_FREQ){ // not exist
                if ( span == 1){
                    freq = UNKNOWN_CHAR_FREQ;    // single char penalty
                }else{
                    continue;
                }
            }
            if (freq + positionScore[i-span] < positionScore[i]){
                positionScore[i] = freq + positionScore[i-span] + TOKEN_LENGTH_PENALTY; 
                bestPosition[i] = i-span;
            }
        }
    }

    for(int i = size-1; i > 0; ){
        if (bestPosition[i] < 0){ // exception characters
            bestPosition[i] = i-1;
        }
        mBuffer.push_back( make_pair (istart + bestPosition[i], istart + i) );
        i = bestPosition[i];
    }
    delete [] positionScore;
    delete [] bestPosition;
    return mBuffer.size();
}

}
}
