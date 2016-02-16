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
#include "ChineseTokenizer.h"
#include "AnalyzerContainers.h"
#include <utility>  // std::pair
#include "CharSet.h"
#include "util/Assert.h"
#include "util/Logger.h"

using namespace std;

// These const values are used in tokenize() function
const int UNKNOWN_CHAR_FREQ = 2000;
const int TOKEN_LENGTH_PENALTY = 150;
const int MAXIMUM_SEQUENCE_SCORE = 1000000;

namespace srch2{
namespace instantsearch{

ChineseTokenizer::ChineseTokenizer(const ChineseDictionaryContainer* chineseDictionaryContainer)
    :Tokenizer(),mChineseDictionaryContainer(chineseDictionaryContainer), mCurrentChineseTokens(){
    mCurrentChineseTokens.reserve(32);  // Assuming most Chinese sentences have less than 32 tokens. It will grow automatically if larger.
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
 * Generate more tokens and update the currentToken in the container.
 * 1. It will generate multiple Chinese Tokens when we meet Chinese characters.
 *    And store them into mCurrentChineseTokens in the reversed order. 
 * 2. The non-Chinese token will be treated the same as StandardTokenizer. 
 * @return :
 *    true: if the currentToken was set to a new token.
 *    false: end of stream. 
 */
bool ChineseTokenizer::incrementToken(){
    (tokenStreamContainer->currentToken).clear();
    unsigned currentType = CharSet::DELIMITER_TYPE;
    CharType currentChar = 0;

    while(currentType == CharSet::DELIMITER_TYPE || currentType == CharSet::WHITESPACE){ // We ignore delimiters and whitespaces
        if ( isEnd() ){
            return false;
        }
        currentChar = getCurrentChar();
        currentType = characterSet.getCharacterType(currentChar);
        ++(tokenStreamContainer->offset);
    }

    if (currentType == CharSet::HANZI_TYPE){
        return chineseIncrement( tokenStreamContainer->offset - 1);
    } else {
        return nonChineseIncrement(currentType, currentChar);
    }
}

/**
 * Identifies the end of consecutive Chinese characters in a sequence of characters
 * @return: the end position of the sequence,
 *          Note: the last position is at "end-1".
 */
int ChineseTokenizer::identifyEndOfChineseSequence(){
    while (!isEnd()){
        CharType currentChar = getCurrentChar();
        if ( characterSet.getCharacterType(currentChar) != CharSet::HANZI_TYPE){
            break;
        }
        ++(tokenStreamContainer->offset);
    }
    return tokenStreamContainer->offset;
}

bool ChineseTokenizer::chineseIncrement(int iChineseStart){
    int iChineseStop = identifyEndOfChineseSequence();   
    tokenize(tokenStreamContainer->completeCharVector, iChineseStart, iChineseStop);
    ASSERT(!mCurrentChineseTokens.empty());
    feedCurrentToken(mCurrentChineseTokens.back());
    mCurrentChineseTokens.pop_back();
    return true;
}

/**
 * We keep scanning this sequence of characters to produce a single token. 
 * We detect the end of a token in the following cases:
 *
 * 1.The new character is a delimiter;
 * 2.The new character is not a LATIN nor a BOPOMOFO;
 *   In this case, we produce a single token for this character.
 * 3.The new character is either LATIN or BOPOMOFO, 
 *   and it has a different type than the previous character;
 *
 *  */
bool ChineseTokenizer::nonChineseIncrement(unsigned currentType, CharType currentChar){
    vector<CharType> * const pCurrentToken = &(tokenStreamContainer->currentToken);
    unsigned previousType = CharSet::DELIMITER_TYPE;
    while (true){
        switch(currentType){
            case CharSet::DELIMITER_TYPE:
            case CharSet::WHITESPACE:
                ASSERT(!pCurrentToken->empty());
                return true;
            case CharSet::LATIN_TYPE:
            case CharSet::BOPOMOFO_TYPE:
                if (pCurrentToken->empty() || currentType == previousType){
                    pCurrentToken->push_back(currentChar);
                } else {
                    --(tokenStreamContainer->offset);
                    return true;
                }
                break;
            default:    // case 2 
                if (pCurrentToken->empty()){
                    ASSERT( currentType != CharSet::HANZI_TYPE);
                    pCurrentToken->push_back(currentChar);
                } else {
                    --(tokenStreamContainer->offset);
                }
                return true;
        }   
        if(isEnd()){
            break;
        }
        currentChar = getCurrentChar();
        previousType = currentType;
        currentType = characterSet.getCharacterType(currentChar);
        ++(tokenStreamContainer->offset);
    }

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
 * Tokenize a sequence of pure Chinese characaters in the range of [istart, istop)
 * The result is stored into mCurrentChineseTokens.
 *
 * Algorithm: 
 * We run a dynamic programming algorithm to segment the sequence into tokens.
 * For each gap, we compute and store the best segmentation (with a score) 
 * for the subsequence up to this gap. 
 * The smaller the score is, the better it is. 
 *
 * 1.The recurrence function is defined on the gaps of the character positions. 
 *   If we have N characters, we have N+1 gaps.
 * 2.We also store the previous gap that produces the best segmentation 
 *   for the subsequence up to this gap so far.
 * 
 * Special case:
 *  If we see a single Chinese character that does not exist in the dictionary, 
 *  then we give it a default UNKNOWN_CHAR_FREQ score.
 *
 * @param sentence: the data source of Chinese sequence
 * @param istart: the start position of the sentence
 * @param istop: then end position of the sentence (note:the last char is istop-1) 
 */
void ChineseTokenizer::tokenize(const vector<CharType> &sentence, int istart, int istop){
    mCurrentChineseTokens.clear();  
    if (istop <= istart){
        return ;
    }
    int size = istop - istart + 1; 
    int *scoreAtGap= new int[size];
    int *preBestGap= new int[size];
    memset(scoreAtGap, MAXIMUM_SEQUENCE_SCORE, sizeof(int)*(size));
    memset(preBestGap, -1, sizeof(int)*(size));
    scoreAtGap[0] = 0;
    preBestGap[0] = 0;

    for(int endPosition = 1; endPosition < size; ++endPosition){
        for(int spanSize = 1; spanSize < mChineseDictionaryContainer->getMaxWordLength() && spanSize <= endPosition; ++spanSize){
            short freq = mChineseDictionaryContainer->getFreq(sentence, (unsigned)(istart + endPosition-spanSize),(unsigned) spanSize);
            if (freq == Dictionary::INVALID_WORD_FREQ){ // The character does not exist
                if ( spanSize == 1){                    // Refer to: Special case
                    freq = UNKNOWN_CHAR_FREQ;   
                }else{
                    continue;
                }
            }
            if (freq + scoreAtGap[endPosition-spanSize] + TOKEN_LENGTH_PENALTY < scoreAtGap[endPosition]){
                scoreAtGap[endPosition] = freq + scoreAtGap[endPosition-spanSize] + TOKEN_LENGTH_PENALTY; 
                preBestGap[endPosition] = endPosition-spanSize;
            }
        }
    }

    // We backtrack to produce the tokens
    for(int pos = size-1; pos > 0; pos = preBestGap[pos]){
        ASSERT(preBestGap[pos] >=0);
        mCurrentChineseTokens.push_back( make_pair (istart + preBestGap[pos], istart + pos) );
    }
    delete [] scoreAtGap;
    delete [] preBestGap;
}

}
}
