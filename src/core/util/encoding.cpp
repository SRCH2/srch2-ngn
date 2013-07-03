
// $Id: encoding.cpp 3341 2013-05-14 12:56:33Z jiaying $

//
// encoding.cpp
//
//  Created on: 2013-4-6
//      Author: Jiaying
//

#include "encoding.h"


// Tranform a utf-8 string to a CharType vector.
void utf8StringToCharTypeVector(const string &utf8String, vector<CharType> & charTypeVector)
{
	charTypeVector.clear();

	// We check if the string is a valid utf-8 string first. If not, we get a valid utf-8 prefix of this string.
	// Then we transform the string to CharType vector.
	// For example, suppose utf8String = c_1 c_2 c_3 c_4 c_5, where each c_i represents a character.
	// Assume that c_4 is not a valid utf-8 character since it doesn't conform the utf-8 encoding scheme.
	// Then we will get the valid utf-8 prefix c_1 c_2 c_3.  We transform it to a CharType vector
	// and return it.
	string::const_iterator end_it = utf8::find_invalid(utf8String.begin(), utf8String.end());
	if (end_it != utf8String.end()) {
		std::cout << "Invalid UTF-8 encoding detected in " << utf8String << "\n";
	}

	utf8::utf8to16(utf8String.begin(), end_it, back_inserter(charTypeVector));
}
void charTypeVectorToUtf8String(const vector<CharType> &charTypeVector, string &utf8String)
{
	utf8String.clear();
	utf8::utf16to8(charTypeVector.begin(), charTypeVector.end(), back_inserter(utf8String));
}

vector<CharType> getCharTypeVector(const string &utf8String)
{
	vector<CharType> charTypeVector;
	utf8StringToCharTypeVector(utf8String, charTypeVector);
	return charTypeVector;
}
string getUtf8String(const vector<CharType> &charTypeVector)
{
	string utf8String;
	charTypeVectorToUtf8String(charTypeVector, utf8String);
	return utf8String;
}

// Give a utf8String, return its number of unicode endpoints as its length
// For example "vidéo" strlen will return 6, this function will return 5
const unsigned getUtf8StringCharacterNumber(const string utf8String)
{
	string::const_iterator end_it = utf8::find_invalid(utf8String.begin(), utf8String.end());
	if (end_it != utf8String.end()) {
		std::cout << "Invalid UTF-8 encoding detected in " << utf8String << "\n";
	}
	// Get the utf8 string length in the valid range
	// For example a_1 a_2...a_i...a_n is a string, in which a_i is not valid, we will only return i-1[a_1 a_2...a_i-1]as the length
	return utf8::distance(utf8String.begin(), end_it);
}


ostream& operator<<(ostream& out, const vector<CharType>& charTypeVector)
{
   for(vector<CharType>::const_iterator it = charTypeVector.begin(); it != charTypeVector.end(); it++)
	   out<<*it;
   return out;
}

bool operator<(const vector<CharType> &x, const vector<CharType> &y)
{
 int i = 0;
 int xlen = x.size();
 int ylen = y.size();
 while(i != xlen && i != ylen)
 {
	 if(x[i] == y[i])
		 i++;
	 else if(x[i] < y[i])
		 return true;
	 else
		 return false;
 }
 if(xlen < ylen) return true;
 else return false;
}

bool operator==(const vector<CharType> &x, const vector<CharType> &y)
{
	if(x.size() != y.size()) return false;
	else
	{
		for(int i = 0; i< x.size(); i++)
		{
			if(x[i] != y[i])
				return false;
		}
		return true;
	}
}
