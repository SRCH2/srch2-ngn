//$Id: $
/*
 * Highlighter_Test.cpp
 *
 *  Created on: Jan 19, 2014
 *      Author: surendra bisht
 */
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <set>
#include <sys/time.h>
#include "util/Logger.h"
#include "util/Assert.h"
#include "highlighter/Highlighter.h"
#include <instantsearch/Analyzer.h>

using namespace srch2::instantsearch;

void readTestFile(const char * testFIle, vector<string>& records) {
	std::ifstream ifs;
	ifs.open(testFIle);
	if (!ifs.good())
	{
		std::cout << "could not read positional index file from " << testFIle << endl;
		return;
	}
	string line;
	vector<unsigned> piVect;

	while(getline(ifs, line)) {
		records.push_back(line);
	}
}
void printTestHeader(unsigned id) {
	cout << "--------------------------test " << id << "--------------------------------"<< endl;
}
void callSnippetGen(AnalyzerBasedAlgorithm *algo, const string& record, vector<string>& snippets,
		vector<keywordHighlightInfo>& keywordStrToHighlight, vector<string>& expectedResult) {
	snippets.clear();
	algo->getSnippet(NULL, 0, 0, record.c_str(), snippets, false, keywordStrToHighlight);
	for (unsigned i = 0; i < snippets.size(); ++i) {
		cout << "snippet " << i << " : "<< snippets[i] << endl ;
	}
	ASSERT(snippets[0].compare(expectedResult[0]) == 0);
}

/*
 *  Test Plan:
 *
 *  1. multiple keywords (3), single attribute. All matches found the in the attribute and are highlighted.
 *  2. multiple keywords (3), single attribute. Only one match found in the attribute and is highlighted.
 *  3. Two keywords, single attribute.  All matches found the in the attribute and are highlighted.
 *  4. Two keywords, single attribute.  No matches found the in the attribute and default snippet is generated
 *  5. Two keywords with large gap, single attribute. Match found in the attribute and snippet is generated with two fragments.
 *  6. Single keywords, single attribute.  Multiple matches found but only only one highlighted in the attribute to avoid fragmentation.
 *  7. Single Phrase.single attribute. Match found and highlighted.
 *  8. Single proximity phrase, single attribute, Match found and highlighted.
 *  9. Phrase and shared Term :  q = "shard hashed"~2 and shard . Matches found and highlighted.
 *  10. Phrase and Term:  q = google  and "storage architecture". Match found and highlighted.
 *  11. Multiple phrases : q = "storage engines" and "storage systems". Match found and highlighted.
 */

int main() {
	const char *recordFileStr = getenv("recordsFile");
	if (recordFileStr == NULL)
	{
		cout << "environment variable 'recordsFile' is not set" << endl;
		return -1;
	}
	vector<string> records;
	readTestFile(recordFileStr, records);

	if (records.size() < 4){
		return -1;
	}

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");
	vector<keywordHighlightInfo> keywordStrToHighlight;
	vector<PhraseInfoForHighLight> phrasesInfoList;
	vector<string> snippets;

	keywordHighlightInfo inf;
	inf.flag = HIGHLIGHT_KEYWORD_IS_COMPLETE; utf8StringToCharTypeVector("latency", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_COMPLETE; utf8StringToCharTypeVector("filesystem", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_COMPLETE; utf8StringToCharTypeVector("predictability", inf.key); keywordStrToHighlight.push_back(inf);

	HighlightConfig hconf;
	hconf.snippetSize=150;
	hconf.highlightMarkers.push_back(make_pair("<exact>" , "</exact>"));  //exact
	hconf.highlightMarkers.push_back(make_pair("<fuzzy>" , "</fuzzy>"));  //fuzzy
	vector<string> expectedResults;
	AnalyzerBasedAlgorithm *algo = new AnalyzerBasedAlgorithm(analyzer, phrasesInfoList, hconf);
	printTestHeader(1);
	expectedResults.push_back("... to use. It provides low <exact>latency</exact>, solid <exact>predictability</exact>, is robust in the face of crashes, and is friendly from a <exact>filesystem</exact> backup point of view. ...");
	callSnippetGen(algo, records[3].c_str(), snippets, keywordStrToHighlight, expectedResults);
	printTestHeader(2);
	expectedResults.clear();
	expectedResults.push_back("... In some earlier tests, we saw InnoDB provide a narrower variance of <exact>latency</exact> (such as lower values in the 99th percentile) but we have not seen ...");
	callSnippetGen(algo, records[2].c_str(), snippets, keywordStrToHighlight, expectedResults);

	delete algo;

	keywordStrToHighlight.clear();
	inf.flag = HIGHLIGHT_KEYWORD_IS_COMPLETE; utf8StringToCharTypeVector("disk", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_COMPLETE; utf8StringToCharTypeVector("journaling", inf.key); keywordStrToHighlight.push_back(inf);

	algo = new AnalyzerBasedAlgorithm(analyzer, phrasesInfoList, hconf);

	printTestHeader(3);
	expectedResults.clear();
	expectedResults.push_back("... preliminary comparisons between the two. We tried to be as fair as possible. For instance, InnoDB was given an independent <exact>disk</exact> for its <exact>journaling</exact>.");
	callSnippetGen(algo, records[3].c_str(), snippets, keywordStrToHighlight, expectedResults);
	printTestHeader(4);
	// tests the case where not keywords match and the default snippet is generated.
	expectedResults.clear();
	expectedResults.push_back("If you shard an empty collection using a hashed shard key, MongoDB will automatically create and migrate chunks so that each shard has two chunks. You ...");
	callSnippetGen(algo, records[1].c_str(), snippets, keywordStrToHighlight, expectedResults);

	delete algo;

	keywordStrToHighlight.clear();
	inf.flag = HIGHLIGHT_KEYWORD_IS_COMPLETE; utf8StringToCharTypeVector("shard", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_COMPLETE; utf8StringToCharTypeVector("command", inf.key); keywordStrToHighlight.push_back(inf);

	algo = new AnalyzerBasedAlgorithm(analyzer, phrasesInfoList, hconf);
	printTestHeader(5);
	expectedResults.clear();
	expectedResults.push_back("<exact>shard</exact> has two chunks. You can control how many chunks MongoDB will create ...manually creating chunks on the empty collection using the split <exact>command</exact>.");
	callSnippetGen(algo, records[1].c_str(), snippets, keywordStrToHighlight, expectedResults);

	delete algo;

	keywordStrToHighlight.clear();
	inf.flag = HIGHLIGHT_KEYWORD_IS_COMPLETE; utf8StringToCharTypeVector("mongodb", inf.key); keywordStrToHighlight.push_back(inf);

	algo = new AnalyzerBasedAlgorithm(analyzer, phrasesInfoList, hconf);
	printTestHeader(6);
	expectedResults.clear();
	expectedResults.push_back("If you shard an empty collection using a hashed shard key, <exact>MongoDB</exact> will automatically create and migrate chunks so that each shard has two chunks. ...");
	callSnippetGen(algo, records[1].c_str(), snippets, keywordStrToHighlight, expectedResults);

	delete algo;
	//------------------------------------------------------------------------
	// Now let us test phrases ...
    // exact phrase
	phrasesInfoList.clear();
	keywordStrToHighlight.clear();
	inf.flag = HIGHLIGHT_KEYWORD_IS_PHRASE; utf8StringToCharTypeVector("solid", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_PHRASE; utf8StringToCharTypeVector("predictability", inf.key); keywordStrToHighlight.push_back(inf);
	PhraseInfoForHighLight pifh;
	pifh.slop = 0;
	PhraseTermInfo pti;
	pti.queryPosition = 1; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);
	algo = new AnalyzerBasedAlgorithm(analyzer, phrasesInfoList, hconf);

	printTestHeader(7);
	expectedResults.clear();
	expectedResults.push_back("... obvious right storage engine to use. It provides low latency, <exact>solid</exact> <exact>predictability</exact>, is robust in the face of crashes, and is friendly from a ...");
	callSnippetGen(algo, records[3].c_str(), snippets, keywordStrToHighlight, expectedResults);
	delete algo;
	//------------------------------------------------------------------------
	// proximity "shard hashed"~1
	phrasesInfoList.clear();
	keywordStrToHighlight.clear();
	inf.flag = HIGHLIGHT_KEYWORD_IS_PHRASE; utf8StringToCharTypeVector("shard", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_PHRASE; utf8StringToCharTypeVector("hashed", inf.key); keywordStrToHighlight.push_back(inf);

	pifh.slop = 2;
	pifh.phraseKeyWords.clear();
	pti.queryPosition = 1; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);

	algo = new AnalyzerBasedAlgorithm(analyzer, phrasesInfoList, hconf);

	printTestHeader(8);
	expectedResults.clear();
	expectedResults.push_back("If you shard an empty collection using a <exact>hashed</exact> <exact>shard</exact> key, MongoDB will automatically create and migrate chunks so that each shard has two chunks. ...");
	callSnippetGen(algo, records[1].c_str(), snippets, keywordStrToHighlight, expectedResults);

	delete algo;
	//------------------------------------------------------------------------
	phrasesInfoList.clear();
	keywordStrToHighlight.clear();
	//inf.flag = 1; utf8StringToCharTypeVector("shard", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_HYBRID; utf8StringToCharTypeVector("shard", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_PHRASE; utf8StringToCharTypeVector("hashed", inf.key); keywordStrToHighlight.push_back(inf);

	pifh.slop = 2;
	pifh.phraseKeyWords.clear();
	//pti.queryPosition = 1; pti.recordPosition = NULL; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 1; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);

	algo = new AnalyzerBasedAlgorithm(analyzer, phrasesInfoList, hconf);

	printTestHeader(9);
	expectedResults.clear();
	expectedResults.push_back("If you <exact>shard</exact> an empty collection using a <exact>hashed</exact> <exact>shard</exact> key, MongoDB will automatically create and migrate chunks so that each <exact>shard</exact> has two ...");
	callSnippetGen(algo, records[1].c_str(), snippets, keywordStrToHighlight, expectedResults);

	delete algo;
	//------------------------------------------------------------------------
	phrasesInfoList.clear();
	keywordStrToHighlight.clear();
	inf.flag = HIGHLIGHT_KEYWORD_IS_COMPLETE; utf8StringToCharTypeVector("google", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_PHRASE; utf8StringToCharTypeVector("storage", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_PHRASE; utf8StringToCharTypeVector("architecture", inf.key); keywordStrToHighlight.push_back(inf);

	pifh.slop = 0;
	pifh.phraseKeyWords.clear();
	pti.queryPosition = 1; pti.recordPosition = NULL; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 1; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);

	algo = new AnalyzerBasedAlgorithm(analyzer, phrasesInfoList, hconf);

	printTestHeader(10);
	expectedResults.clear();
	expectedResults.push_back("... attention to LevelDB, which was recently released by <exact>Google</exact>. LevelDBs <exact>storage</exact> <exact>architecture</exact> is more like BigTables memtable/sstable model than it ...");
	callSnippetGen(algo, records[3].c_str(), snippets, keywordStrToHighlight, expectedResults);

	delete algo;
	//------------------------------------------------------------------------
	// multiple phrases
	//--------------------
	phrasesInfoList.clear();
	keywordStrToHighlight.clear();
	inf.flag = HIGHLIGHT_KEYWORD_IS_PHRASE; utf8StringToCharTypeVector("storage", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_PHRASE; utf8StringToCharTypeVector("engines", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = HIGHLIGHT_KEYWORD_IS_PHRASE; utf8StringToCharTypeVector("systems", inf.key); keywordStrToHighlight.push_back(inf);

	vector<unsigned> * k1Array = new vector<unsigned>();
	vector<unsigned> * k2Array = new vector<unsigned>();
	vector<unsigned> * k3Array = new vector<unsigned>();
	vector<unsigned> * k4Array = new vector<unsigned>();

	pifh.slop = 0;
	pifh.phraseKeyWords.clear();
	pti.queryPosition = 1; pti.recordPosition = k1Array; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = k2Array; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 1; pti.recordPosition = NULL; pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);

	pifh.slop = 0;
	pifh.phraseKeyWords.clear();
	pti.queryPosition = 1; pti.recordPosition = k4Array; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = NULL; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = k3Array; pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);

	algo = new AnalyzerBasedAlgorithm(analyzer, phrasesInfoList, hconf);

	printTestHeader(11);
	expectedResults.clear();
	expectedResults.push_back("<exact>storage</exact> <exact>engines</exact>. A number of ... of these two <exact>storage</exact> <exact>systems</exact> can certainly ... using the <exact>storage</exact> <exact>engines</exact> through Riak, ... of embedded <exact>storage</exact> <exact>engines</exact>....");
	callSnippetGen(algo, records[2].c_str(), snippets, keywordStrToHighlight, expectedResults);

	delete algo;
	delete analyzer;
}



