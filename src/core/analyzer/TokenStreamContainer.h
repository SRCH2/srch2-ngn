/*
 * SharedToken.h
 *
 *  Created on: 2013-5-17
 */

#ifndef __CORE_ANALYZER_TOKENSTREAMCONTAINER_H__
#define __CORE_ANALYZER_TOKENSTREAMCONTAINER_H__

#include <vector>
#include "util/encoding.h"
#include "CharSet.h"
#include <instantsearch/Analyzer.h>
namespace srch2
{
namespace instantsearch
{

class TokenStreamContainer {
public:
	TokenStreamContainer():offset(0), currentTokenPosition(0), currentTokenOffset(0), type(ANALYZED_ORIGINAL_TOKEN){}
    void fillInCharacters(const std::vector<CharType> &charVector, bool isPrefix=false){
        currentToken.clear();
        completeCharVector = charVector;
        currentTokenOffset = 0;
        currentTokenPosition = 0;
        offset = 0;
        type = ANALYZED_ORIGINAL_TOKEN;
        this->isPrefix = isPrefix;
    }
	/*
	 * For example:  process "We went to school"
	 * 		completeCharVector = "We went to school"
	 * 		When we process to the first character 'W', currentToken="W",
	 * 		offset=0, Token position = 1, currentTokenOffset = 1
	 * 		When we move to the second character 'e', currentToken="We",
	 * 		offset=1,  Token position = 1, currentTokenOffset = 1
	 * 		...
	 * 		When we move to 'n' at 6th place, currentToken= 'wen'
	 * 		offset=5,  Token position = 2, currentTokenOffset = 4
	 */
	std::vector<CharType> currentToken;			//current token
	std::vector<CharType> completeCharVector; 	//complete char vector of a string
	int offset;									//the offset of current position to process
	unsigned currentTokenPosition;
	unsigned currentTokenOffset;        // offset of current token from the beginning of the buffer.
	AnalyzedTokenType type;
	unsigned currentTokenLen;
	bool isPrefix;
};
}}
#endif /* __CORE_ANALYZER__TOKENSTREAMCONTAINER_H__ */
