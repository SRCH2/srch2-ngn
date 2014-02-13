/*
 * LowerCaseFilter.cpp
 *
 *  Created on: 2013-5-17
 */

#include <iostream>
#include "LowerCaseFilter.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

LowerCaseFilter::LowerCaseFilter(TokenStream* tokenStream) :
    TokenFilter(tokenStream)
{
    this->tokenStreamContainer = tokenStream->tokenStreamContainer;
}

void LowerCaseFilter::transformToLowerCase(vector<CharType> &token) {
    for (int i = 0; i < token.size(); i++) {
        if (token[i] >= (CharType) 'A' && token[i] <= (CharType) 'Z') {
            token[i] = token[i] + 32;
        }
    }
}

bool LowerCaseFilter::processToken() {
    if (this->tokenStream->processToken()) {
        transformToLowerCase(tokenStreamContainer->currentToken);
        return true;
    } else {
        return false;

    }
}
LowerCaseFilter::~LowerCaseFilter() {
    // TODO Auto-generated destructor stub
}
}
}

