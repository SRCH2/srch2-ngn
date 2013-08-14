
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

//
// This test is to verify the correctness of Analyzer to token a string.
//
#include <vector>
#include <string>
#include "util/Assert.h"
#include "analyzer/StandardAnalyzer.h"
#include "analyzer/SimpleAnalyzer.h"

using namespace std;
using namespace srch2::instantsearch;

//SimpleAnalyzer organizes a tokenizer using " " as the delimiter and a "ToLowerCase" filter
void testSimpleAnalyzer()
{
	string src="We are美丽 Chinese";
	AnalyzerInternal *simpleAnlyzer = new SimpleAnalyzer();
	TokenStream * tokenStream = simpleAnlyzer->createOperatorFlow();
	simpleAnlyzer->loadData(src);
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
	string src="We are美丽 Chineseㄓㄠ";
	AnalyzerInternal *standardAnalyzer = new StandardAnalyzer();
	TokenStream * tokenStream = standardAnalyzer->createOperatorFlow();
	standardAnalyzer->loadData(src);
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
	delete tokenStream;
	delete standardAnalyzer;
}

int main()
{
	testSimpleAnalyzer();
	cout << "SimpleAnalyzer test passed" << endl;

	testStandardAnalyzer();
	cout << "StandardAnalyzer test passed" << endl;

	return 0;
}
