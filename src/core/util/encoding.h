/*
 * encoding.h
 *
 *  Created on: 2013-4-6
 *      Author: Jiaying
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
