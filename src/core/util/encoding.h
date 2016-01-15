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
/*
 * encoding.h
 *
 *  Created on: 2013-4-6
 */

#ifndef ENCODING_H_
#define ENCODING_H_

#include <string>
#include <vector>
#include <iostream>
#include "utf8.h"
#include <instantsearch/Constants.h>
using std::string;
using std::vector;
using std::ostream;

using namespace srch2::instantsearch;

//This is the upper bound of the characters, on which we do fuzzy search
const unsigned int CHARTYPE_FUZZY_UPPERBOUND = 128;
const CharType DEFAULT_DELIMITER = (CharType)' ';
const string DEFAULT_DELIMITER_STRING(1, (char)DEFAULT_DELIMITER);

void utf8StringToCharTypeVector(const string &utf8String, vector<CharType> &charTypeVector);
void charTypeVectorToUtf8String(const vector<CharType> &charTypeVector, string &utf8Sring);

void charTypeVectorToUtf8String(const vector<CharType> &charTypeVector, int begin, int end, string &utf8String);

const unsigned getUtf8StringCharacterNumber(const string utf8String);

//for easy use in test
vector<CharType> getCharTypeVector(const string &utf8String);
string getUtf8String(const vector<CharType> &charTypeVector);

ostream& operator<<(ostream& out, const vector<CharType>& charTypeVector);

#endif /* ENCODING_H_ */
