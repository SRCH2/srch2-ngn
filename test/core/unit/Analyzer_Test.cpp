
// $Id: Analyzer_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

// This test is to verify the correctness of Analyzer to token a string.

#include <stdio.h>  /* defines FILENAME_MAX */
#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif



#include <vector>
#include <string>
#include "util/Assert.h"
#include "analyzer/StandardAnalyzer.h"
#include "analyzer/SimpleAnalyzer.h"

using namespace std;
using namespace srch2::instantsearch;

string getCurrentWorkDirectory(){
	char cCurrentPath[FILENAME_MAX];

	 if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
	 {
	     return "";
	 }

	cCurrentPath[sizeof(cCurrentPath) - 1] = '\0'; /* not really required */
	return cCurrentPath;
}

//SimpleAnalyzer organizes a tokenizer using " " as the delimiter and a "ToLowerCase" filter
void testSimpleAnalyzer()
{
	string src="We are美丽 Chinese";
	AnalyzerInternal *simpleAnlyzer = new SimpleAnalyzer();
	TokenOperator * tokenOperator = simpleAnlyzer->createOperatorFlow();
	simpleAnlyzer->loadData(src);
	vector<string> vectorString;
	vectorString.push_back("we");
	vectorString.push_back("are美丽");
	vectorString.push_back("chinese");
	int i=0;
	while(tokenOperator->incrementToken())
	{
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		ASSERT(vectorString[i] == src);
		i++;
	}
	delete tokenOperator;
	delete simpleAnlyzer;
}
//StandardAnalyzer organizes a tokenizer treating characters >= 256 as a single token and   a "ToLowerCase" filter
void testStandardAnalyzer()
{
	string src="We are美丽 Chineseㄓㄠ";
	AnalyzerInternal *standardAnalyzer = new StandardAnalyzer();
	TokenOperator * tokenOperator = standardAnalyzer->createOperatorFlow();
	standardAnalyzer->loadData(src);
	vector<string> vectorString;
	vectorString.push_back("we");
	vectorString.push_back("are");
	vectorString.push_back("美");
	vectorString.push_back("丽");
	vectorString.push_back("chinese");
	vectorString.push_back("ㄓㄠ");
	int i=0;
	while(tokenOperator->incrementToken())
	{
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		ASSERT(vectorString[i] == src);
		i++;
	}
	delete tokenOperator;
	delete standardAnalyzer;
}

void testLowerCase() {
	cout << "#########################################################################" << endl;
	cout << "#########################################################################" << "LowerCase Filter" << endl;
	bool printFlag = true;

	AnalyzerInternal *simpleAnlyzer = new StandardAnalyzer(
			DISABLE_STEMMER_NORMALIZER,
			"",
			"",
			"",
			SYNONYM_DONOT_KEEP_ORIGIN);
	TokenOperator * tokenOperator = simpleAnlyzer->createOperatorFlow();

	string src = "Here IS A Set OF some inStructIOns fOR WHo has the bOOks";
	simpleAnlyzer->loadData(src);
	// to print out the results
	vector<string> originalWords;
	originalWords.push_back("Here");
	originalWords.push_back("IS");
	originalWords.push_back("A");
	originalWords.push_back("Set");
	originalWords.push_back("OF");
	originalWords.push_back("some");
	originalWords.push_back("inStructIOns");
	originalWords.push_back("fOR");
	originalWords.push_back("WHo");
	originalWords.push_back("has");
	originalWords.push_back("the");
	originalWords.push_back("bOOks");

	vector<string> vectorString;
	vectorString.push_back("here");
	vectorString.push_back("is");
	vectorString.push_back("a");
	vectorString.push_back("set");
	vectorString.push_back("of");
	vectorString.push_back("some");
	vectorString.push_back("instructions");
	vectorString.push_back("for");
	vectorString.push_back("who");
	vectorString.push_back("has");
	vectorString.push_back("the");
	vectorString.push_back("books");

	int i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << originalWords[i] << "   =>   " << src << " " << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}

	// deleting the objects
	delete tokenOperator;
	delete simpleAnlyzer;
}


void testStemmerFilter() {
	cout << "\n\n";
	cout << "#########################################################################" << endl;
	cout << "#########################################################################" << "Stemmer Filter" << endl;
	cout << "stemmer File: " << getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt" << "\n\n";


	// if it is true, it prints the results of the test, else id doesn't
	bool printFlag = true;

	// when you are running ctest you should be in the build directory
	AnalyzerInternal *simpleAnlyzer = new SimpleAnalyzer(
			ENABLE_STEMMER_NORMALIZER,
			getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
			"", "", SYNONYM_DONOT_KEEP_ORIGIN );
	TokenOperator * tokenOperator = simpleAnlyzer->createOperatorFlow();

	if (printFlag) {
		cout << "TEST 1: No Stemming" << endl;
	}
	// TEST 1 (no stemming)
	// input string
	string src = "People show that they are good";
	simpleAnlyzer->loadData(src);
	// to print out the results
	vector<string> originalWords;
	originalWords.push_back("People");
	originalWords.push_back("show");
	originalWords.push_back("that");
	originalWords.push_back("they");
	originalWords.push_back("are");
	originalWords.push_back("good");

	vector<string> vectorString;
	vectorString.push_back("people");
	vectorString.push_back("show");
	vectorString.push_back("that");
	vectorString.push_back("they");
	vectorString.push_back("are");
	vectorString.push_back("good");

	int i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << originalWords[i] << "   =>   " << src << " " << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}



	if (printFlag) {
		cout << endl << endl << "TEST 2: Stem English Words" << endl;
	}



	// TEST 2 (stem English words)
	src = "Our instructions package shoWs the results";
	simpleAnlyzer->loadData(src);
	// to print out the results
	originalWords.clear();
	originalWords.push_back("Our");
	originalWords.push_back("instructions");
	originalWords.push_back("package");
	originalWords.push_back("shoWs");
	originalWords.push_back("the"); // because of stop words
	originalWords.push_back("results");

	vectorString.clear();
	vectorString.push_back("our");
	vectorString.push_back("instruct");
	vectorString.push_back("package");
	vectorString.push_back("show");
	vectorString.push_back("the");
	vectorString.push_back("result");

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << originalWords[i] << "   =>   " << src << " " << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}




	if (printFlag) {
		cout << endl << endl << "TEST 3: Stem English & Non-English Words"
				<< endl;
	}
	// TEST 3 (stem non-English words)
	src = "meanings meanings2 of Befall and pencils丽 سلام following";
	simpleAnlyzer->loadData(src);
	// to print out the results
	originalWords.clear();
	originalWords.push_back("meanings");
	originalWords.push_back("meanings2");
	originalWords.push_back("of");
	originalWords.push_back("Befall");
	originalWords.push_back("and");
	originalWords.push_back("pencils丽");
	originalWords.push_back("سلام");
	originalWords.push_back("following");

	vectorString.clear();
	vectorString.push_back("mean");
	vectorString.push_back("meanings2");
	vectorString.push_back("of");
	vectorString.push_back("befall");
	vectorString.push_back("and");
	vectorString.push_back("pencils丽");
	vectorString.push_back("سلام");
	vectorString.push_back("following");

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << originalWords[i] << "   =>   " << src << " " << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}

	// deleting the objects
	delete tokenOperator;
	delete simpleAnlyzer;
}

void testStopFilter() {
	cout << "\n\n";
	cout << "#########################################################################" << endl;
	cout << "#########################################################################" << "Stop Filter" << endl;
	cout << "stopWords File:  " << getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/stopWordsFile.txt" << "\n";
	cout << "stemmer File:  " << getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt" << "\n\n";

	// if it is true, it prints the results of the test, else id doesn't
	bool printFlag = true;

	AnalyzerInternal *simpleAnlyzer = new StandardAnalyzer(
			ENABLE_STEMMER_NORMALIZER,
			getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
			getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/stopWordsFile.txt",
			"",
			SYNONYM_DONOT_KEEP_ORIGIN);
	TokenOperator * tokenOperator = simpleAnlyzer->createOperatorFlow();

	string src = "Here IS A Set OF some instructions for who has the books";
	simpleAnlyzer->loadData(src);
	// to print out the results
	vector<string> originalWords;
	originalWords.push_back("Here");
	originalWords.push_back("is");
	originalWords.push_back("set");
	originalWords.push_back("of");
	originalWords.push_back("some");
	originalWords.push_back("instructions");
	originalWords.push_back("for");
	originalWords.push_back("who");
	originalWords.push_back("has");
	originalWords.push_back("books");

	vector<string> vectorString;
	vectorString.push_back("here");
	vectorString.push_back("is");
	vectorString.push_back("set");
	vectorString.push_back("of");
	vectorString.push_back("some");
	vectorString.push_back("instruct");
	vectorString.push_back("for");
	vectorString.push_back("who");
	vectorString.push_back("has");
	vectorString.push_back("books");

	int i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << originalWords[i] << "   =>   " << src << " " << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}

	// deleting the objects
	delete tokenOperator;
	delete simpleAnlyzer;
}

void testSynonymFilter() {
	cout << "\n\n";
	cout << "#########################################################################" << endl;
	cout << "#########################################################################" << "Stop Filter" << endl;
	cout << "stopWords File:  " << getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/stopWordsFile.txt" << "\n";
	cout << "stemmer File:  " << getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt" << "\n";
	cout << "stynonym File:  " << getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt" << "\n\n";

	// if it is true, it prints the results of the test, else id doesn't
	bool printFlag = true;

	AnalyzerInternal *simpleAnlyzer = new SimpleAnalyzer(
			ENABLE_STEMMER_NORMALIZER,
			getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
			getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/stopWordsFile.txt",
			getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt",
			SYNONYM_KEEP_ORIGIN);
	TokenOperator * tokenOperator = simpleAnlyzer->createOperatorFlow();

	// TEST 1
	// input string

	string src = "bill is in new york";
	simpleAnlyzer->loadData(src);
	// to print out the results

	if (printFlag) {
		cout << "## Test 1:  " << src << endl;
	}
	vector<string> vectorString;
	vectorString.push_back("bill");
	vectorString.push_back("william"); // bill
	vectorString.push_back("is");
	vectorString.push_back("in");
	vectorString.push_back("new");
	vectorString.push_back("york");
	vectorString.push_back("ny"); // new york

	int i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << "+++++++ SynonymFilter:  " << src  << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}

	// TEST 2
	// input string
	simpleAnlyzer = new SimpleAnalyzer(
				ENABLE_STEMMER_NORMALIZER,
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/stopWordsFile.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt",
				SYNONYM_KEEP_ORIGIN);
	tokenOperator = simpleAnlyzer->createOperatorFlow();
	src = "new wal new wal mart new york new new york city";
	simpleAnlyzer->loadData(src);
	// to print out the results
	if (printFlag) {
		cout << "## Test 2:  " << src << endl;
	}
	vectorString.clear();
	vectorString.push_back("new");
	vectorString.push_back("wal");
	vectorString.push_back("new");
	vectorString.push_back("wal");
	vectorString.push_back("mart");
	vectorString.push_back("walmart"); // wal mart
	vectorString.push_back("new");
	vectorString.push_back("york");
	vectorString.push_back("ny"); // new york
	vectorString.push_back("new");
	vectorString.push_back("new"); // new york city
	vectorString.push_back("york"); // new york city
	vectorString.push_back("city"); // new york city
	vectorString.push_back("nyc"); // new york city

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << "------- SynonymFilter:  " << src  << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}



	// TEST 3
	// input string
	simpleAnlyzer = new SimpleAnalyzer(
				ENABLE_STEMMER_NORMALIZER,
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/stopWordsFile.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt",
				SYNONYM_KEEP_ORIGIN);
	tokenOperator = simpleAnlyzer->createOperatorFlow();
	src = "new bill bring your own bill bring your own beverage your own beverage bring";
	simpleAnlyzer->loadData(src);
	// to print out the results
	if (printFlag) {
		cout << "## Test 3:  " << src << endl;
	}
	vectorString.clear();
	vectorString.push_back("new");
	vectorString.push_back("bill");
	vectorString.push_back("william");
	vectorString.push_back("bring");
	vectorString.push_back("your");
	vectorString.push_back("own");
	vectorString.push_back("bill");
	vectorString.push_back("william");
	vectorString.push_back("bring");
	vectorString.push_back("your");
	vectorString.push_back("own");
	vectorString.push_back("beverage");
	vectorString.push_back("byob");
	vectorString.push_back("your");
	vectorString.push_back("own");
	vectorString.push_back("beverage");
	vectorString.push_back("yob");
	vectorString.push_back("bring");

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << "+++++++ SynonymFilter:  " << src  << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}


	// TEST 4
	// input string
	simpleAnlyzer = new SimpleAnalyzer(
				ENABLE_STEMMER_NORMALIZER,
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
				"",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt",
				SYNONYM_KEEP_ORIGIN);
	tokenOperator = simpleAnlyzer->createOperatorFlow();
	src = "a b c d e f g a b c d e f t a b c d e f";
	simpleAnlyzer->loadData(src);
	// to print out the results
	if (printFlag) {
		cout << "## Test 4:  " << src << endl;
	}
	vectorString.clear();
	vectorString.push_back("a");
	vectorString.push_back("b");
	vectorString.push_back("c");
	vectorString.push_back("d");
	vectorString.push_back("e");
	vectorString.push_back("f");
	vectorString.push_back("g");
	vectorString.push_back("x");
	vectorString.push_back("a");
	vectorString.push_back("b");
	vectorString.push_back("c");
	vectorString.push_back("y");
	vectorString.push_back("d");
	vectorString.push_back("e");
	vectorString.push_back("f");
	vectorString.push_back("z");
	vectorString.push_back("t");
	vectorString.push_back("a");
	vectorString.push_back("b");
	vectorString.push_back("c");
	vectorString.push_back("y");
	vectorString.push_back("d");
	vectorString.push_back("e");
	vectorString.push_back("f");
	vectorString.push_back("z");

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << "------- SynonymFilter:  " << src  << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}

	// TEST 5
	// input string
	simpleAnlyzer = new SimpleAnalyzer(
				ENABLE_STEMMER_NORMALIZER,
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
				"",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt",
				SYNONYM_KEEP_ORIGIN);
	tokenOperator = simpleAnlyzer->createOperatorFlow();
	src = "a b d e f new york g a b c d e f t a b c d e f wal mart آسان bill 美 ایمان برجسته";
	simpleAnlyzer->loadData(src);
	// to print out the results

	if (printFlag) {
		cout << "## Test 5:  " << src << endl;
	}

	vectorString.clear();
	vectorString.push_back("a");
	vectorString.push_back("b");
	vectorString.push_back("d");
	vectorString.push_back("e");
	vectorString.push_back("f");
	vectorString.push_back("z");
	vectorString.push_back("new");
	vectorString.push_back("york");
	vectorString.push_back("ny");
	vectorString.push_back("g");
	vectorString.push_back("a");
	vectorString.push_back("b");
	vectorString.push_back("c");
	vectorString.push_back("y");
	vectorString.push_back("d");
	vectorString.push_back("e");
	vectorString.push_back("f");
	vectorString.push_back("z");
	vectorString.push_back("t");
	vectorString.push_back("a");
	vectorString.push_back("b");
	vectorString.push_back("c");
	vectorString.push_back("y");
	vectorString.push_back("d");
	vectorString.push_back("e");
	vectorString.push_back("f");
	vectorString.push_back("z");
	vectorString.push_back("wal");
	vectorString.push_back("mart");
	vectorString.push_back("walmart");
	vectorString.push_back("آسان");
	vectorString.push_back("راحت");
	vectorString.push_back("bill");
	vectorString.push_back("william");
	vectorString.push_back("美");
	vectorString.push_back("丽");
	vectorString.push_back("ایمان");
	vectorString.push_back("برجسته");
	vectorString.push_back("مشتی");

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << "+++++++ SynonymFilter:  " << src  << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}

	// TEST 6
	// input string
	simpleAnlyzer = new SimpleAnalyzer(
				ENABLE_STEMMER_NORMALIZER,
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/stopWordsFile.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt",
				SYNONYM_KEEP_ORIGIN);
	tokenOperator = simpleAnlyzer->createOperatorFlow();
	src = "bill";
	simpleAnlyzer->loadData(src);
	// to print out the results

	if (printFlag) {
		cout << "## Test 6:  " << src << endl;
	}

	vectorString.clear();
	vectorString.push_back("bill");
	vectorString.push_back("william");

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << "------- SynonymFilter:  " <<  src << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}

	simpleAnlyzer = new SimpleAnalyzer(
				ENABLE_STEMMER_NORMALIZER,
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/stopWordsFile.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt",
				SYNONYM_DONOT_KEEP_ORIGIN);
	tokenOperator = simpleAnlyzer->createOperatorFlow();

	// TEST 7
	// input string
	src = "bill is in new york";
	simpleAnlyzer->loadData(src);
	// to print out the results
	if (printFlag) {
		cout << "## Test 7:  " << src << endl;
	}


	vectorString.clear();
	vectorString.push_back("william"); // bill
	vectorString.push_back("is");
	vectorString.push_back("in");
	vectorString.push_back("ny"); // new york

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << "+++++++ SynonymFilter:  " << src  << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}

	// TEST 8
	// input string
	simpleAnlyzer = new SimpleAnalyzer(
				ENABLE_STEMMER_NORMALIZER,
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/stopWordsFile.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt",
				SYNONYM_DONOT_KEEP_ORIGIN);
	tokenOperator = simpleAnlyzer->createOperatorFlow();
	src = "new wal new wal mart new york new new york city";
	simpleAnlyzer->loadData(src);
	// to print out the results
	if (printFlag) {
		cout << "## Test 8:  " << src << endl;
	}

	vectorString.clear();
	vectorString.push_back("new");
	vectorString.push_back("wal");
	vectorString.push_back("new");
	vectorString.push_back("walmart"); // wal mart
	vectorString.push_back("ny"); // new york
	vectorString.push_back("new");
	vectorString.push_back("nyc"); // new york city

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << "------- SynonymFilter:  " << src  << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}

	// TEST 9
	simpleAnlyzer = new SimpleAnalyzer(
				ENABLE_STEMMER_NORMALIZER,
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/stopWordsFile.txt",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt",
				SYNONYM_DONOT_KEEP_ORIGIN);
	tokenOperator = simpleAnlyzer->createOperatorFlow();
	src = "new bill bring your own bill bring your own beverage your own beverage bring";
	simpleAnlyzer->loadData(src);
	// to print out the results
	if (printFlag) {
		cout << "## Test 9:  " << src << endl;
	}

	vectorString.clear();
	vectorString.push_back("new");
	vectorString.push_back("william");
	vectorString.push_back("bring");
	vectorString.push_back("your");
	vectorString.push_back("own");
	vectorString.push_back("william");
	vectorString.push_back("byob");
	vectorString.push_back("yob");
	vectorString.push_back("bring");

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << "+++++++ SynonymFilter:  " << src  << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}


	// TEST 10
	simpleAnlyzer = new SimpleAnalyzer(
				ENABLE_STEMMER_NORMALIZER,
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/StemmerHeadwords.txt",
				"",
				getCurrentWorkDirectory() + "/../../test/core/unit/test_data/analyzer/synonymFile.txt",
				SYNONYM_DONOT_KEEP_ORIGIN);
	tokenOperator = simpleAnlyzer->createOperatorFlow();
	src = "a b c d e f g a b c d e f t a b c d e f";
	simpleAnlyzer->loadData(src);
	if (printFlag) {
		cout << "## Test 10:  " << src << endl;
	}

	vectorString.clear();
	vectorString.push_back("x");
	vectorString.push_back("y");
	vectorString.push_back("z");
	vectorString.push_back("t");
	vectorString.push_back("y");
	vectorString.push_back("z");

	i = 0;
	while (tokenOperator->incrementToken()) {
		vector<CharType> charVector;
		tokenOperator->getCurrentToken(charVector);
		charTypeVectorToUtf8String(charVector, src);
		if (printFlag) {
			cout << "------- SynonymFilter:  " << src  << endl;
		}
		ASSERT(vectorString[i] == src);
		i++;
	}



	// deleting the objects
	delete tokenOperator;
	delete simpleAnlyzer;
}


int main() {
	testSimpleAnalyzer();
	cout << "SimpleAnalyzer test passed" << endl;

	testStandardAnalyzer();
	cout << "StandardAnalyzer test passed" << endl;

	testStemmerFilter();
	cout << "StemmerFilter test passed" << endl;

	testStopFilter();
	cout << "StopFilter test passed" << endl;

	testSynonymFilter();
	cout << "SynonymFilter test passed" << endl;
	return 0;
}
