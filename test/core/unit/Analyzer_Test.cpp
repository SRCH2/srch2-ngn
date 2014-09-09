// $Id: Analyzer_Test.cpp 3456 2013-06-14 02:11:13Z iman $

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

#include <vector>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <sys/stat.h>
#include <iostream>
#include <functional>
#include <map>

#include "analyzer/StandardAnalyzer.h"
#include "analyzer/SimpleAnalyzer.h"
#include "analyzer/ChineseAnalyzer.h"

#include "index/InvertedIndex.h"
#include "operation/IndexerInternal.h"
#include "util/Assert.h"
#include "util/cowvector/compression/cowvector_S16.h"
#include "analyzer/AnalyzerContainers.h"
using namespace std;
using namespace srch2::instantsearch;



//SimpleAnalyzer organizes a tokenizer using " " as the delimiter and a "ToLowerCase" filter
void testSimpleAnalyzer()
{
    string src="We are美丽 Chinese";
    AnalyzerInternal *simpleAnlyzer = new SimpleAnalyzer(NULL, NULL, NULL, NULL, string(""));
    TokenStream * tokenStream = simpleAnlyzer->createOperatorFlow();
    tokenStream->fillInCharacters(src);
    vector<string> vectorString;
    vectorString.push_back("we");
    vectorString.push_back("are美丽");
    vectorString.push_back("chinese");
    int i=0;
    while(tokenStream->processToken())
    {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        ASSERT(vectorString[i] == src);
        i++;
    }
    delete tokenStream;
    delete simpleAnlyzer;
}
//StandardAnalyzer organizes a tokenizer treating characters >= 256 as a single token and   a "ToLowerCase" filter
void testStandardAnalyzer()
{
    SynonymContainer *syn = SynonymContainer::getInstance(string(""), SYNONYM_KEEP_ORIGIN);
    StopWordContainer *stop = StopWordContainer::getInstance("");
    ProtectedWordsContainer *prot = ProtectedWordsContainer::getInstance("");
    StemmerContainer *stem = StemmerContainer::getInstance("");
    stem->init();

    string src="We are美丽 Chineseㄓㄠ";
    AnalyzerInternal *standardAnalyzer = new StandardAnalyzer(NULL, stop, prot, syn, string(""));
    TokenStream * tokenStream = standardAnalyzer->createOperatorFlow();
    standardAnalyzer->setTokenStream(tokenStream);
    tokenStream->fillInCharacters(src);
    vector<string> vectorString;
    vectorString.push_back("we");
    vectorString.push_back("are");
    vectorString.push_back("美");
    vectorString.push_back("丽");
    vectorString.push_back("chinese");
    vectorString.push_back("ㄓㄠ");
    int i=0;
    while(tokenStream->processToken())
    {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        ASSERT(vectorString[i] == src);
        i++;
    }
    delete standardAnalyzer;
    stem->free();
    stop->free();
    syn->free();
    prot->free();
}

void testChineseAnalyzer(const string &dataDir){
    string dictPath = dataDir + "/srch2_dict_ch.core";
    string src="We are美丽 Chineseㄓㄠ我是一个中国人。，上海自来水来自海上，从４月１０号起，“一票制” 朱镕基";
    src +="!，。》@##%     在民国时期，插画Picture在中国曾经盛极一时。";
    src += "END";
    ChineseDictionaryContainer *dict = ChineseDictionaryContainer::getInstance(dictPath);
    AnalyzerInternal *chineseAnalyzer = new ChineseAnalyzer(dict, NULL, NULL, NULL, string(""));
    TokenStream * tokenStream = chineseAnalyzer->createOperatorFlow();
    chineseAnalyzer->setTokenStream(tokenStream);
    tokenStream->fillInCharacters(src);
    vector<string> vectorString;
    vectorString.push_back("we");
    vectorString.push_back("are");
    vectorString.push_back("美丽");
    vectorString.push_back("chinese");
    vectorString.push_back("ㄓㄠ");

    vectorString.push_back("我是");
    vectorString.push_back("一个");
    vectorString.push_back("中国人");
    vectorString.push_back("上海");
    vectorString.push_back("自来水");
    vectorString.push_back("来自");
    vectorString.push_back("海上");
    vectorString.push_back("从");
    vectorString.push_back("４");
    vectorString.push_back("月");
    vectorString.push_back("１０");
    vectorString.push_back("号");
    vectorString.push_back("起");
    vectorString.push_back("一票制");
    vectorString.push_back("朱");
    vectorString.push_back("镕");
    vectorString.push_back("基");
    vectorString.push_back("在");
    vectorString.push_back("民国");
    vectorString.push_back("时期");
    vectorString.push_back("插画");
    vectorString.push_back("picture");
    vectorString.push_back("在");
    vectorString.push_back("中国");
    vectorString.push_back("曾经");
    vectorString.push_back("盛");
    vectorString.push_back("极");
    vectorString.push_back("一时");
    vectorString.push_back("end");

    int i=0;
    while(tokenStream->processToken())
    {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << src << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }
    ASSERT(i==vectorString.size());
    delete chineseAnalyzer;
}

void testLowerCase() {
    cout << "#########################################################################" << endl;
    cout << "#########################################################################" << "LowerCase Filter" << endl;

    SynonymContainer *syn = SynonymContainer::getInstance(string(""), SYNONYM_DONOT_KEEP_ORIGIN);
    ProtectedWordsContainer *prot = ProtectedWordsContainer::getInstance("");
    AnalyzerInternal *simpleAnlyzer = new StandardAnalyzer(NULL, NULL, prot, syn, string(""));
    TokenStream * tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);

    string src = "Here IS A Set OF some inStructIOns fOR WHo has the bOOks";
    tokenStream->fillInCharacters(src);
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
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << originalWords[i] << "   =>   " << src << " " << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }

    // deleting the objects
    delete simpleAnlyzer;
    syn->free();
    prot->free();
}


void testStemmerFilter(string dataDir) {
    cout << "\n\n";
    cout << "#########################################################################" << endl;
    cout << "#########################################################################" << "Stemmer Filter" << endl;
    cout << "stemmer File: " << dataDir + "/StemmerHeadwords.txt" << "\n\n";

    // when you are running ctest you should be in the build directory
    SynonymContainer *syn = SynonymContainer::getInstance(string(""), SYNONYM_DONOT_KEEP_ORIGIN);
    StemmerContainer *stem = StemmerContainer::getInstance(dataDir + "/StemmerHeadwords.txt");
    AnalyzerInternal *simpleAnlyzer = new SimpleAnalyzer(stem, NULL, NULL, syn, string(""));
    TokenStream * tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);

    cout << "TEST 1: No Stemming" << endl;
    // TEST 1 (no stemming)
    // input string
    string src = "People show that they are good";
    tokenStream->fillInCharacters(src);
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
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << originalWords[i] << "   =>   " << src << " " << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }


    cout << endl << endl << "TEST 2: Stem English Words" << endl;
    // TEST 2 (stem English words)
    src = "Our instructions package shoWs the results";
    tokenStream->fillInCharacters(src);
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
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << originalWords[i] << "   =>   " << src << " " << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }


    cout << endl << endl << "TEST 3: Stem English & Non-English Words" << endl;
    // TEST 3 (stem non-English words)
    src = "meanings meanings2 of Befall and pencils丽 سلام following";
    tokenStream->fillInCharacters(src);
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
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << originalWords[i] << "   =>   " << src << " " << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }

    // deleting the objects
    delete simpleAnlyzer;
    syn->free();
    stem->free();
}

void testStopFilter(string dataDir) {
    cout << "\n\n";
    cout << "#########################################################################" << endl;
    cout << "#########################################################################" << "Stop Filter" << endl;
    cout << "stopWords File:  " << dataDir + "/stopWordsFile.txt" << "\n";
    cout << "stemmer File:  " << dataDir + "/StemmerHeadwords.txt" << "\n\n";

    // if it is true, it prints the results of the test, else id doesn't

    SynonymContainer *syn = SynonymContainer::getInstance(string(""), SYNONYM_DONOT_KEEP_ORIGIN);
    StemmerContainer *stem = StemmerContainer::getInstance(dataDir + "/StemmerHeadwords.txt");
    StopWordContainer *stop = StopWordContainer::getInstance(dataDir + "/stopWordsFile.txt");
    AnalyzerInternal *simpleAnlyzer = new StandardAnalyzer(stem, stop, NULL, syn, string(""));
    TokenStream * tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);
    string src = "Here IS A Set OF some instructions for who has the books";
    tokenStream->fillInCharacters(src);
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
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << originalWords[i] << "   =>   " << src << " " << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }

    // deleting the objects
    delete simpleAnlyzer;
    syn->free();
    stem->free();
    stop->free();

    stop = StopWordContainer::getInstance(dataDir + "/stopWordsFile.txt");
    ChineseDictionaryContainer * dict = ChineseDictionaryContainer::getInstance(dataDir +"/srch2_dict_ch.core");
    AnalyzerInternal *chineseAnalyzer = new ChineseAnalyzer(
            dict,
            stop,
            NULL, NULL,
            "" // special characters
            );
    tokenStream = chineseAnalyzer->createOperatorFlow();
    chineseAnalyzer->setTokenStream(tokenStream);

    src = "Here IS A Set我的不过滤，这个的要过滤 是";
    tokenStream->fillInCharacters(src);
    vectorString.clear();
    vectorString.push_back("here");
    vectorString.push_back("is");
    vectorString.push_back("set");
    vectorString.push_back("我的");
    vectorString.push_back("不");
    vectorString.push_back("过滤");
    vectorString.push_back("这个");
    vectorString.push_back("要");
    vectorString.push_back("过滤");
    i = 0;
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        ASSERT(vectorString[i] == src);
        i++;
    }
    ASSERT(i == vectorString.size());

    // deleting the objects
    delete chineseAnalyzer;
    stop->free();
}

void testSynonymFilter(string dataDir) {
    cout << "\n\n";
    cout << "#########################################################################" << endl;
    cout << "############################ Synonym Test ###############################" << endl;
    cout << "stopWords File:  " << dataDir + "/stopWordsFile.txt" << "\n";
    cout << "stemmer File:  " << dataDir + "/StemmerHeadwords.txt" << "\n";
    cout << "stynonym File:  " << dataDir + "/synonymFile.txt" << "\n\n";

    // if it is true, it prints the results of the test, else id doesn't

    SynonymContainer *syn = SynonymContainer::getInstance(dataDir + "/synonymFile.txt", SYNONYM_KEEP_ORIGIN);
    StemmerContainer *stem = StemmerContainer::getInstance(dataDir + "/StemmerHeadwords.txt");
    stem->init();
    StopWordContainer *stop = StopWordContainer::getInstance(dataDir + "/stopWordsFile.txt");
    AnalyzerInternal *simpleAnlyzer = new SimpleAnalyzer(stem, stop, NULL, syn, string(""));
    TokenStream * tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);
    // TEST 1
    // input string

    string src = "bill is in new york";
    tokenStream->fillInCharacters(src);
    // to print out the results

    cout << "## Test 1:  " << src << endl;
    vector<string> vectorString;
    vectorString.push_back("bill");
    vectorString.push_back("william"); // bill
    vectorString.push_back("is");
    vectorString.push_back("in");
    vectorString.push_back("new");
    vectorString.push_back("york");
    vectorString.push_back("ny"); // new york

    int i = 0;
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << "+++++++ SynonymFilter:  " << src  << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }

    delete simpleAnlyzer;
    // TEST 2
    // input string
    simpleAnlyzer = new SimpleAnalyzer(stem, stop, NULL, syn, string(""));
    tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);

    src = "new wal new wal mart new york new new york city";
    tokenStream->fillInCharacters(src);
    // to print out the results
    cout << "## Test 2:  " << src << endl;
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
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << "------- SynonymFilter:  " << src  << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }

    delete simpleAnlyzer;

    // TEST 3
    // input string
    simpleAnlyzer = new SimpleAnalyzer(stem, stop, NULL, syn, string(""));
    tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);
    src = "new bill bring your own bill bring your own beverage your own beverage bring";
    tokenStream->fillInCharacters(src);
    // to print out the results
    cout << "## Test 3:  " << src << endl;
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
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << "+++++++ SynonymFilter:  " << src  << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }

    delete simpleAnlyzer;

    // TEST 4
    // input string
    simpleAnlyzer = new SimpleAnalyzer(stem, NULL, NULL, syn, string(""));
    tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);
    src = "a b c d e f g a b c d e f t a b c d e f";
    tokenStream->fillInCharacters(src);
    // to print out the results
    cout << "## Test 4:  " << src << endl;
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
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << "------- SynonymFilter:  " << src  << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }

    delete simpleAnlyzer;
    // TEST 5
    // input string
    simpleAnlyzer = new SimpleAnalyzer(stem, NULL, NULL, syn, string(""));
    tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);
    src = "a b d e f new york g a b c d e f t a b c d e f wal mart آسان bill 美 ایمان برجسته";
    tokenStream->fillInCharacters(src);
    // to print out the results

    cout << "## Test 5:  " << src << endl;
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
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << "+++++++ SynonymFilter:  " << src  << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }
    delete simpleAnlyzer;

    // TEST 6
    // input string
    simpleAnlyzer = new SimpleAnalyzer(stem, stop, NULL, syn, string(""));
    tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);
    src = "bill";
    tokenStream->fillInCharacters(src);
    // to print out the results

    cout << "## Test 6:  " << src << endl;

    vectorString.clear();
    vectorString.push_back("bill");
    vectorString.push_back("william");

    i = 0;
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << "------- SynonymFilter:  " <<  src << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }

    delete simpleAnlyzer;
    syn->free();
    syn = SynonymContainer::getInstance(dataDir + "/synonymFile.txt", SYNONYM_DONOT_KEEP_ORIGIN);
    syn->init();
    simpleAnlyzer = new SimpleAnalyzer(stem, stop, NULL, syn, string(""));
    tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);

    // TEST 7
    // input string
    src = "bill is in new york";
    tokenStream->fillInCharacters(src);
    // to print out the results
    cout << "## Test 7:  " << src << endl;
    vectorString.clear();
    vectorString.push_back("william"); // bill
    vectorString.push_back("is");
    vectorString.push_back("in");
    vectorString.push_back("ny"); // new york

    i = 0;
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << "+++++++ SynonymFilter:  " << src  << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }
    delete simpleAnlyzer;
    // TEST 8
    // input string
    simpleAnlyzer = new SimpleAnalyzer(stem, stop, NULL, syn, string(""));
    tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);
    src = "new wal new wal mart new york new new york city";
    tokenStream->fillInCharacters(src);
    // to print out the results
    cout << "## Test 8:  " << src << endl;
    vectorString.clear();
    vectorString.push_back("new");
    vectorString.push_back("wal");
    vectorString.push_back("new");
    vectorString.push_back("walmart"); // wal mart
    vectorString.push_back("ny"); // new york
    vectorString.push_back("new");
    vectorString.push_back("nyc"); // new york city

    i = 0;
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << "------- SynonymFilter:  " << src  << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }
    delete simpleAnlyzer;
    // TEST 9
    simpleAnlyzer = new SimpleAnalyzer(stem, stop, NULL, syn, string(""));
    tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);
    src = "new bill bring your own bill bring your own beverage your own beverage bring";
    tokenStream->fillInCharacters(src);
    // to print out the results
    cout << "## Test 9:  " << src << endl;
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
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << "+++++++ SynonymFilter:  " << src  << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }

    delete simpleAnlyzer;
    // TEST 10
    simpleAnlyzer = new SimpleAnalyzer(stem, NULL, NULL, syn, string(""));
    tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);
    src = "a b c d e f g a b c d e f t a b c d e f";
    tokenStream->fillInCharacters(src);
    cout << "## Test 10:  " << src << endl;

    vectorString.clear();
    vectorString.push_back("x");
    vectorString.push_back("y");
    vectorString.push_back("z");
    vectorString.push_back("t");
    vectorString.push_back("y");
    vectorString.push_back("z");

    i = 0;
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << "------- SynonymFilter:  " << src  << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }



    // deleting the objects
    delete simpleAnlyzer;

    // TEST 11 : Test ChineseAnayzer
    Logger::info("current dir:%s", dataDir.c_str());
    ChineseDictionaryContainer* dict = ChineseDictionaryContainer::getInstance(dataDir + "/srch2_dict_ch.core");
    AnalyzerInternal* chineseAnalyzer = new ChineseAnalyzer( dict
            , stop, NULL, syn, "");
    tokenStream = chineseAnalyzer->createOperatorFlow();
    chineseAnalyzer->setTokenStream(tokenStream);
    src = "ok~dd 美丽还是美";
    tokenStream->fillInCharacters(src);
    cout << "## Test 11:  " << src << endl;

    vectorString.clear();
    vectorString.push_back("ok");
    vectorString.push_back("dd");
    vectorString.push_back("漂亮");
    vectorString.push_back("还是");
    vectorString.push_back("丽");

    i = 0;
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        cout << src << endl;
        ASSERT(vectorString[i] == src);
        i++;
    }
    ASSERT(i==vectorString.size());

    // deleting the objects
    delete chineseAnalyzer;
    syn->free();
    stem->free();
    stop->free();

    /*
     *    Test 12. This test case tests whether the filter picks shorter synonym when large synonym
     *    prefix do not match eventually.
     *
     *    Test record: "club los angeles galaxy"
     *    synonym rule for this test:
     *    los angeles = la
     *    club los angeles lakers = showtime
     */
    SynonymContainer *synonymContainer = SynonymContainer::getInstance(dataDir + "/synonymFile.txt", SYNONYM_KEEP_ORIGIN);
    simpleAnlyzer = new SimpleAnalyzer(NULL, NULL, NULL, synonymContainer, string(""));
    tokenStream = simpleAnlyzer->createOperatorFlow();
    simpleAnlyzer->setTokenStream(tokenStream);

    string testRecord = "club los angeles galaxy";
    tokenStream->fillInCharacters(testRecord);

    cout << "## Test 12:  " << testRecord << endl;
    vectorString.clear();
    vectorString.push_back("club");
    vectorString.push_back("los");
    vectorString.push_back("angeles");
    vectorString.push_back("la");  // los angeles = la
    vectorString.push_back("galaxy");

    i = 0;
    while (tokenStream->processToken()) {
        vector<CharType> charVector;
        charVector = tokenStream->getProcessedToken();
        charTypeVectorToUtf8String(charVector, src);
        int pos = tokenStream->getProcessedTokenPosition();
        int offset = tokenStream->getProcessedTokenCharOffset();
        int charLen = tokenStream->getProcessedTokenLen();
        AnalyzedTokenType type = tokenStream->getProcessedTokentype();

        cout << "+++++++ SynonymFilter:  " << src  << " : " << pos << " : " << offset << " : "
        		<<  charLen << " : " << type <<  endl;
        ASSERT(vectorString[i] == src);
        i++;
    }

    /*
     *    Test 13. This test case tests replacement only synonyms. It ignores the expansion flag and
     *    always replaces original token by its synonym.
     *
     *    Test record: "uci cs department"
     *    synonym rule for this test:
     *    cs=>computer science
     */

    testRecord = "uci cs department";
    cout << "## Test 13:  " << testRecord << endl;

    tokenStream->fillInCharacters(testRecord);

    vectorString.clear();
    vectorString.push_back("uci");
    vectorString.push_back("computer science");   // cs=computer science
    vectorString.push_back("department");

    i = 0;
    while (tokenStream->processToken()) {
    	vector<CharType> charVector;
    	charVector = tokenStream->getProcessedToken();
    	charTypeVectorToUtf8String(charVector, src);
        int pos = tokenStream->getProcessedTokenPosition();
        int offset = tokenStream->getProcessedTokenCharOffset();
        int charLen = tokenStream->getProcessedTokenLen();
        AnalyzedTokenType type = tokenStream->getProcessedTokentype();

        cout << "+++++++ SynonymFilter:  " << src  << " : " << pos << " : " << offset << " : "
        		<<  charLen << " : " << type <<  endl;
    	ASSERT(vectorString[i] == src);
    	i++;
    }
    delete simpleAnlyzer;

}

void testAnalyzerSerilization(string dataDir) {

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    string INDEX_DIR = ".";

    /*
     * Test 1
     */
    // INDEXING
    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    Record *record = new Record(schema);

    SynonymContainer *syn = SynonymContainer::getInstance(dataDir + "/synonymFile.txt", SYNONYM_KEEP_ORIGIN);
    StemmerContainer *stem = StemmerContainer::getInstance(dataDir + "/StemmerHeadwords.txt");
    StopWordContainer *stop = StopWordContainer::getInstance(dataDir + "/stopWordsFile.txt");
    ProtectedWordsContainer *prot = ProtectedWordsContainer::getInstance(dataDir + "/protectedWords.txt");

    Analyzer *analyzer = new Analyzer(stem, stop, prot, syn, string(""), SIMPLE_ANALYZER);

    IndexMetaData *indexMetaData = new IndexMetaData( GlobalCache::create(1000,1000),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);

    Indexer *index = Indexer::create(indexMetaData, analyzer, schema);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "come Yesterday Once More");
    record->setRecordBoost(10);
    index->addRecord(record, analyzer);

    index->commit();
    index->save();

    delete schema;
    delete record;
    delete analyzer;
    delete index;
    delete indexMetaData;
    syn->free();
    prot->free();
    stop->free();
    stem->free();

    // LOADING
    IndexMetaData *indexMetaData2 = new IndexMetaData( GlobalCache::create(1000,1000),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
    IndexReaderWriter *indexReaderWriter = new IndexReaderWriter(indexMetaData2);

    delete indexReaderWriter;
    delete indexMetaData2;

    cout << endl << endl ;

    /*
     * Test 2
     */
    // INDEXING
    ///Create Schema
    Schema *schema2 = Schema::create(srch2::instantsearch::DefaultIndex);
    schema2->setPrimaryKey("article_id"); // integer, not searchable
    schema2->setSearchableAttribute("article_id"); // convert id to searchable text
    schema2->setSearchableAttribute("article_authors", 2); // searchable text
    schema2->setSearchableAttribute("article_title", 7); // searchable text

    Record *record2 = new Record(schema2);

    syn = SynonymContainer::getInstance(dataDir + "/synonymFile.txt", SYNONYM_DONOT_KEEP_ORIGIN);
    stem = StemmerContainer::getInstance(dataDir + "/StemmerHeadwords.txt");
    prot = ProtectedWordsContainer::getInstance(dataDir + "/protectedWords.txt");

    Analyzer *analyzer2 = new Analyzer(stem, NULL, prot, syn, string(""), STANDARD_ANALYZER);

    IndexMetaData *indexMetaData3 = new IndexMetaData( GlobalCache::create(1000,1000),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);

    Indexer *index2 = Indexer::create(indexMetaData3, analyzer2, schema2);

    record2->setPrimaryKey(1001);
    record2->setSearchableAttributeValue("article_authors", " and Jack Lennon");
    record2->setSearchableAttributeValue("article_title", "Yeste More");
    record2->setRecordBoost(10);
    index2->addRecord(record2, analyzer2);

    index2->commit();
    index2->save();

    delete schema2;
    delete record2;
    delete analyzer2;
    delete index2;
    delete indexMetaData3;
    syn->free();
    stem->free();
    prot->free();

    // LOADING
    IndexMetaData *indexMetaData4 = new IndexMetaData( GlobalCache::create(1000,1000),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		INDEX_DIR);
    IndexReaderWriter *indexReaderWriter2 = new IndexReaderWriter(indexMetaData4);

    delete indexReaderWriter2;
    delete indexMetaData4;

}

/*
 *   This common module runs analyzer to tokenize and filter the generated tokens.
 *   All tokens are compared with expected tokens supplied by tokenizedWords vector
 */
void runAnalyzer(TokenStream * tokenStream , const vector<string>& tokenizedWords) {
	int i = 0;
	while (tokenStream->processToken()) {
		string token;
		vector<CharType> charVector;
		charVector = tokenStream->getProcessedToken();
		charTypeVectorToUtf8String(charVector, token);
                cout << "Expecting " << tokenizedWords[i] << " for " << i << "th word; got " << token << endl;
		ASSERT(i < tokenizedWords.size());
		ASSERT(tokenizedWords[i] == token);
		i++;
	}
	ASSERT(i == tokenizedWords.size());
}
void testLastTokenAsStopWord(string dataDir){
	cout << "#########################################################################" << endl;

    StopWordContainer *stop = StopWordContainer::getInstance(dataDir + "/stopWordsFile.txt");
    SynonymContainer *syn = SynonymContainer::getInstance("", SYNONYM_DONOT_KEEP_ORIGIN);
    StemmerContainer *stem = StemmerContainer::getInstance("");
    ProtectedWordsContainer *prot = ProtectedWordsContainer::getInstance("");

    AnalyzerInternal *standardAnlyzer = new StandardAnalyzer(stem, stop, prot, syn, string(""));
    TokenStream * tokenStream = standardAnlyzer->createOperatorFlow();
    standardAnlyzer->setTokenStream(tokenStream);

	string src = "the"; //"the last word is theater"
	tokenStream->fillInCharacters(src, true);
	// to print out the results
	vector<string> tokenizedWords;
	tokenizedWords.push_back("the");

	runAnalyzer(tokenStream, tokenizedWords);

	// deleting the objects
	delete standardAnlyzer;
    stop->free();
    syn->free();
    stem->free();
    prot->free();
}

void testProtectedWords(string dataDir){
	cout << "#########################################################################" << endl;

    StopWordContainer *stop = StopWordContainer::getInstance("");
    stop->init();
    SynonymContainer *syn = SynonymContainer::getInstance("", SYNONYM_DONOT_KEEP_ORIGIN);
    syn->init();
    ProtectedWordsContainer *prot = ProtectedWordsContainer::getInstance(dataDir + "/protectedWords.txt");
    prot->init();

    AnalyzerInternal *standardAnlyzer = new StandardAnalyzer(NULL, stop, prot, syn, string(""));
    TokenStream * tokenStream = standardAnlyzer->createOperatorFlow();
    standardAnlyzer->setTokenStream(tokenStream);

	string src = "C++ is successor of C. .NET Framework (pronounced dot net) is developed by Microsoft.";
	tokenStream->fillInCharacters(src);
	// to print out the results
	vector<string> tokenizedWords;
	tokenizedWords.push_back("c++");
	tokenizedWords.push_back("is");
	tokenizedWords.push_back("successor");
	tokenizedWords.push_back("of");
	tokenizedWords.push_back("c");
	tokenizedWords.push_back(".net");
	tokenizedWords.push_back("framework");
	tokenizedWords.push_back("pronounced");
	tokenizedWords.push_back("dot");
	tokenizedWords.push_back("net");
	tokenizedWords.push_back("is");
	tokenizedWords.push_back("developed");
	tokenizedWords.push_back("by");
	tokenizedWords.push_back("microsoft");

	runAnalyzer(tokenStream, tokenizedWords);

	src = "Node.js is built on Chrome's JavaScript engine";
	tokenStream->fillInCharacters(src);
	// to print out the results
	tokenizedWords.clear();
	tokenizedWords.push_back("node.js");
	tokenizedWords.push_back("is");
	tokenizedWords.push_back("built");
	tokenizedWords.push_back("on");
	tokenizedWords.push_back("chrome");
	tokenizedWords.push_back("s");
	tokenizedWords.push_back("javascript");
	tokenizedWords.push_back("engine");

	runAnalyzer(tokenStream, tokenizedWords);

	src = "C# and Java-Script";
	tokenStream->fillInCharacters(src);
	// to print out the results
	tokenizedWords.clear();
	tokenizedWords.push_back("c#");
	tokenizedWords.push_back("and");
	tokenizedWords.push_back("java");
	tokenizedWords.push_back("script");

	runAnalyzer(tokenStream, tokenizedWords);


	src = "Pro*C is embedded sql in C";
	tokenStream->fillInCharacters(src);
	// to print out the results
	tokenizedWords.clear();
	tokenizedWords.push_back("pro");
	tokenizedWords.push_back("c");
	tokenizedWords.push_back("is");
	tokenizedWords.push_back("embedded");
	tokenizedWords.push_back("sql");
    tokenizedWords.push_back("in");
    tokenizedWords.push_back("c");

    runAnalyzer(tokenStream, tokenizedWords);

	// deleting the objects
	delete standardAnlyzer;

    syn->free();
    prot->free();
    stop->free();
}

int buildChineseDictionary(const string & builder, const string & textFile, const string &outputBin){
    Logger::debug("builder: %s", builder.c_str());
    Logger::debug("textFile: %s", textFile.c_str());
    Logger::debug("outputBin: %s", outputBin.c_str());
    struct stat stResult;
    if ( stat(builder.c_str(), &stResult) != 0){
        Logger::warn("utility bin not found, the test will not rebuild the ChineseDictionary.%s"
                , outputBin.c_str());
        return 0;
    }
    string command = builder + " " + textFile + " " + outputBin;
    int ret = system(command.c_str());
    if (ret != 0){
        Logger::error("ChineseDictionaryBuilder run error.");
    }
    return ret;
}

int main() {
    if ((getenv("dataDir") == NULL) ) {
        cout << "dataDir as an environment variable should be set." << endl;
        cout << "dataDir is the path to the analyzer data files such as StemmerHeadwords.txt etc." << endl;
        ASSERT (getenv("dataDir") != NULL );
        return 0;
    }
    Logger::setLogLevel(Logger::SRCH2_LOG_DEBUG);

    string dataDir(getenv("dataDir"));

    string chineseDictionaryBuilder(getenv("cnDictBuilder"));
    string chineseDictionaryTextFile(getenv("cnDictTxt"));
    const string chineseDictionaryBinary = dataDir +"/srch2_dict_ch.core";

    int ret = buildChineseDictionary(chineseDictionaryBuilder, chineseDictionaryTextFile, 
            chineseDictionaryBinary);
    if (ret != 0) return ret;

    SynonymContainer::getInstance(dataDir + "/synonymFile.txt", SYNONYM_KEEP_ORIGIN)->init();
    StemmerContainer::getInstance(dataDir + "/StemmerHeadwords.txt")->init();
    StopWordContainer::getInstance(dataDir + "/stopWordsFile.txt")->init();
    ChineseDictionaryContainer::getInstance(chineseDictionaryBinary)->init();

    testSimpleAnalyzer();
    cout << "SimpleAnalyzer test passed" << endl;

    testStandardAnalyzer();
    cout << "StandardAnalyzer test passed" << endl;

    testChineseAnalyzer(dataDir);
    cout << "ChineseAnalyzer test passed" << endl;

    testLowerCase();
    cout << "LowerCaseFilter test passed" << endl;

    testStemmerFilter(dataDir);
    cout << "StemmerFilter test passed" << endl;

    testStopFilter(dataDir);
    cout << "StopFilter test passed" << endl;

    testSynonymFilter(dataDir);
    cout << "SynonymFilter test passed" << endl;

    testAnalyzerSerilization(dataDir);
    cout << "Analyzer Serialization test passed" << endl;

    testLastTokenAsStopWord(dataDir);
    cout << "Last stopword is not dropped... test passed" << endl;

    testProtectedWords(dataDir);
    cout << "Protected words test passed" << endl;

    return 0;
}
