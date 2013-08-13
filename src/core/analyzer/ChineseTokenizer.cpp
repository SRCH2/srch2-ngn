//$Id$
#include "ChineseTokenizer.h"

#include <utility>  // std::pair
#include "CharSet.h"
#include "util/Assert.h"

using namespace std;

// These const value used in tokenize function
const int UNKNOWN_CHAR_FREQ = 2000;
const int TOKEN_LENGTH_PENALTY = 150;
const int MAXIMUM_FREQ = 1000000;

namespace srch2{
namespace instantsearch{

ChineseTokenizer::ChineseTokenizer(const string &chineseDictFilePath)
    :mChineseDict(), mCurrentChineseTokens(){
    int ret = mChineseDict.loadDict(chineseDictFilePath);
    if (ret < 0){
        // throw exception in future
    }
    mCurrentChineseTokens.reserve(32);  // Assuming most Chinese sentences have less than 32 tokens. It will grow automatically if larger.
    tokenStreamContainer.reset(new TokenStreamContainer());
}

bool ChineseTokenizer::processToken(){
    if ( mCurrentChineseTokens.size() == 0 ){
        return this->incrementToken();
    }else{
        feedCurrentToken(mCurrentChineseTokens.back());
        mCurrentChineseTokens.pop_back();
    }
    return true;
}

/**
 * Generate more token and update the currentToken
 * 1. It will generate multiple Chinese Token when meet Chinese characters.
 *    And stored them into mCurrentChineseTokens in the reversed order. 
 * 2. The non-Chinese token will be treated the same as StandardTokenizer. 
 * @return :
 *    true: if the currentToken was set to a new token.
 *    flase: end of stream. 
 */
bool ChineseTokenizer::incrementToken(){
    (tokenStreamContainer->currentToken).clear();
    unsigned currentType = CharSet::DELIMITER_TYPE;
    CharType currentChar = 0;

    while(currentType == CharSet::DELIMITER_TYPE){
        if ( isEnd() ){
            return false;
        }
        currentChar = getCurrentChar();
        currentType = CharSet::getCharacterType(currentChar);
        ++(tokenStreamContainer->offset);
    }

    if (currentType == CharSet::HANZI_TYPE){
        return chineseIncrement( tokenStreamContainer->offset - 1);
    } else {
        return nonChineseIncrement(currentType, currentChar);
    }
}

bool ChineseTokenizer::chineseIncrement(int iChineseStart){
    while (!isEnd()){
        CharType currentChar = getCurrentChar();
        if ( CharSet::getCharacterType(currentChar) != CharSet::HANZI_TYPE){
            break;
        }
        ++(tokenStreamContainer->offset);
    }
    tokenize(tokenStreamContainer->completeCharVector, iChineseStart, tokenStreamContainer->offset);
    ASSERT(!mCurrentChineseTokens.empty());
    feedCurrentToken(mCurrentChineseTokens.back());
    mCurrentChineseTokens.pop_back();
    return true;
}

bool ChineseTokenizer::nonChineseIncrement(unsigned currentType, CharType currentChar){
    vector<CharType> * const pCurrentToken = &(tokenStreamContainer->currentToken);
    unsigned previousType = CharSet::DELIMITER_TYPE;
    do{
        switch(currentType){
            case CharSet::DELIMITER_TYPE:
                ASSERT(!pCurrentToken->empty());
                return true;
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
            default:
                if (pCurrentToken->empty()){
                    ASSERT( currentType != CharSet::HANZI_TYPE);
                    pCurrentToken->push_back(currentChar);
                } else {
                    --(tokenStreamContainer->offset);
                }
                return true;
        }   

        currentChar = getCurrentChar();
        previousType = currentType;
        currentType = CharSet::getCharacterType(currentChar);
        ++(tokenStreamContainer->offset);
    } while (!isEnd());

    return !pCurrentToken->empty();
}

void ChineseTokenizer::feedCurrentToken(const pair<short, short> &range){
    vector<CharType> * const pCurrentToken = &(tokenStreamContainer->currentToken);
    pCurrentToken->clear();
    for(short i = range.first; i < range.second; i++){
        pCurrentToken->push_back(
                (tokenStreamContainer->completeCharVector)[i]);
    }
}

/**
 * Tokenize the sequence of pure Chinese sentence in the range of [istart, istop)
 * The result is stored into mCurrentChineseTokens.
 * @param sentence: the data source of Chinese sequence
 * @param istart: the start position of the sentence
 * @param istop: then end position of the sentence (note:the last char is istop-1  
 */
void ChineseTokenizer::tokenize(const vector<CharType> &sentence, int istart, int istop){
    mCurrentChineseTokens.clear();  
    if (istop <= istart){
        return ;
    }
    int size = istop - istart + 1; 
    int *scoreAtPosition= new int[size];
    int *preBestPosition= new int[size];
    memset(scoreAtPosition, MAXIMUM_FREQ, sizeof(int)*(size));
    memset(preBestPosition, -1, sizeof(int)*(size));
    scoreAtPosition[0] = 0;
    preBestPosition[0] = 0;

    for(int endPosition = 1; endPosition < size; ++endPosition){
        for(int span = 1; span < mChineseDict.getMaxLength() && span <= endPosition; ++span){
            short freq = mChineseDict.getFreq(sentence, (unsigned)(istart + endPosition-span),(unsigned) span);
            if (freq == Dictionary::INVALID_WORD_FREQ){ // not exist
                if ( span == 1){                // exception characters
                    freq = UNKNOWN_CHAR_FREQ;   // single char penalty
                }else{
                    continue;
                }
            }
            if (freq + scoreAtPosition[endPosition-span] < scoreAtPosition[endPosition]){
                scoreAtPosition[endPosition] = freq + scoreAtPosition[endPosition-span] + TOKEN_LENGTH_PENALTY; 
                preBestPosition[endPosition] = endPosition-span;
            }
        }
    }

    for(int pos = size-1; pos > 0; ){
        ASSERT(preBestPosition[pos] >=0);
        mCurrentChineseTokens.push_back( make_pair (istart + preBestPosition[pos], istart + pos) );
        pos = preBestPosition[pos];
    }
    delete [] scoreAtPosition;
    delete [] preBestPosition;
}

}
}
