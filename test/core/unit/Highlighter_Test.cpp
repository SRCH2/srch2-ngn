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
void callSnippetGen(AnalyzerBasedAlgorithm *algo, const string& record, vector<string>& snippets) {
	snippets.clear();
	algo->getSnippet(0, 0, record.c_str(), snippets);
	for (unsigned i = 0; i < snippets.size(); ++i) {
		cout << "snippet " << i << " : "<< snippets[i] << endl ;
	}
}
void freeRecordPositionForPhrase(vector<PhraseInfoForHighLight>  phrasesInfoList) {
	for(unsigned j =0; j < phrasesInfoList.size(); ++j) {
		PhraseInfoForHighLight pifh = phrasesInfoList[j];
		for (unsigned i = 0; i < pifh.phraseKeyWords.size(); ++i) {
			if (pifh.phraseKeyWords[i].recordPosition)
				delete pifh.phraseKeyWords[i].recordPosition;
		}
	}
}

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
	inf.flag = 1; utf8StringToCharTypeVector("latency", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 1; utf8StringToCharTypeVector("filesystem", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 1; utf8StringToCharTypeVector("predictability", inf.key); keywordStrToHighlight.push_back(inf);

	HighlightConfig hconf;
	hconf.snippetSize=150;
	hconf.highlightMarkerPre="<b>";
	hconf.highlightMarkerPost="</b>";
	AnalyzerBasedAlgorithm *algo = new AnalyzerBasedAlgorithm(analyzer, keywordStrToHighlight, phrasesInfoList, hconf);
	printTestHeader(1);
	callSnippetGen(algo, records[3].c_str(), snippets);
	printTestHeader(2);
	callSnippetGen(algo, records[2].c_str(), snippets);

	delete algo;

	keywordStrToHighlight.clear();
	inf.flag = 1; utf8StringToCharTypeVector("disk", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 1; utf8StringToCharTypeVector("journaling", inf.key); keywordStrToHighlight.push_back(inf);

	algo = new AnalyzerBasedAlgorithm(analyzer, keywordStrToHighlight, phrasesInfoList, hconf);

	printTestHeader(3);
	callSnippetGen(algo, records[3].c_str(), snippets);
	printTestHeader(4);
	callSnippetGen(algo, records[1].c_str(), snippets);

	delete algo;

	keywordStrToHighlight.clear();
	inf.flag = 1; utf8StringToCharTypeVector("shard", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 1; utf8StringToCharTypeVector("command", inf.key); keywordStrToHighlight.push_back(inf);

	algo = new AnalyzerBasedAlgorithm(analyzer, keywordStrToHighlight, phrasesInfoList, hconf);
	printTestHeader(5);
	callSnippetGen(algo, records[1].c_str(), snippets);

	delete algo;

	keywordStrToHighlight.clear();
	inf.flag = 1; utf8StringToCharTypeVector("mongodb", inf.key); keywordStrToHighlight.push_back(inf);

	algo = new AnalyzerBasedAlgorithm(analyzer, keywordStrToHighlight, phrasesInfoList, hconf);
	printTestHeader(6);
	callSnippetGen(algo, records[1].c_str(), snippets);

	delete algo;
	//------------------------------------------------------------------------
	// Now let us test phrases ...
    // exact phrase
	phrasesInfoList.clear();
	keywordStrToHighlight.clear();
	inf.flag = 2; utf8StringToCharTypeVector("solid", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 2; utf8StringToCharTypeVector("predictability", inf.key); keywordStrToHighlight.push_back(inf);
	PhraseInfoForHighLight pifh;
	pifh.slop = 0;
	PhraseTermInfo pti;
	pti.queryPosition = 1; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);
	algo = new AnalyzerBasedAlgorithm(analyzer, keywordStrToHighlight, phrasesInfoList, hconf);

	printTestHeader(7);
	callSnippetGen(algo, records[3].c_str(), snippets);

	freeRecordPositionForPhrase(phrasesInfoList);
	delete algo;
	//------------------------------------------------------------------------
	// proximity "shard hashed"~1
	phrasesInfoList.clear();
	keywordStrToHighlight.clear();
	inf.flag = 2; utf8StringToCharTypeVector("shard", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 2; utf8StringToCharTypeVector("hashed", inf.key); keywordStrToHighlight.push_back(inf);

	pifh.slop = 2;
	pifh.phraseKeyWords.clear();
	pti.queryPosition = 1; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);

	algo = new AnalyzerBasedAlgorithm(analyzer, keywordStrToHighlight, phrasesInfoList, hconf);

	printTestHeader(8);
	callSnippetGen(algo, records[1].c_str(), snippets);

	freeRecordPositionForPhrase(phrasesInfoList);
	delete algo;
	//------------------------------------------------------------------------
	phrasesInfoList.clear();
	keywordStrToHighlight.clear();
	//inf.flag = 1; utf8StringToCharTypeVector("shard", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 3; utf8StringToCharTypeVector("shard", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 2; utf8StringToCharTypeVector("hashed", inf.key); keywordStrToHighlight.push_back(inf);

	pifh.slop = 2;
	pifh.phraseKeyWords.clear();
	//pti.queryPosition = 1; pti.recordPosition = NULL; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 1; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);

	algo = new AnalyzerBasedAlgorithm(analyzer, keywordStrToHighlight, phrasesInfoList, hconf);

	printTestHeader(9);
	callSnippetGen(algo, records[1].c_str(), snippets);

	freeRecordPositionForPhrase(phrasesInfoList);
	delete algo;
	//------------------------------------------------------------------------
	phrasesInfoList.clear();
	keywordStrToHighlight.clear();
	inf.flag = 1; utf8StringToCharTypeVector("google", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 2; utf8StringToCharTypeVector("storage", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 2; utf8StringToCharTypeVector("architecture", inf.key); keywordStrToHighlight.push_back(inf);

	pifh.slop = 0;
	pifh.phraseKeyWords.clear();
	pti.queryPosition = 1; pti.recordPosition = NULL; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 1; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = new vector<unsigned>(); pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);

	algo = new AnalyzerBasedAlgorithm(analyzer, keywordStrToHighlight, phrasesInfoList, hconf);

	printTestHeader(10);
	callSnippetGen(algo, records[3].c_str(), snippets);

	freeRecordPositionForPhrase(phrasesInfoList);
	delete algo;
	//------------------------------------------------------------------------
	// multiple phrases
	//--------------------
	phrasesInfoList.clear();
	keywordStrToHighlight.clear();
	inf.flag = 2; utf8StringToCharTypeVector("storage", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 2; utf8StringToCharTypeVector("engines", inf.key); keywordStrToHighlight.push_back(inf);
	inf.flag = 2; utf8StringToCharTypeVector("systems", inf.key); keywordStrToHighlight.push_back(inf);

	vector<unsigned> * k1Array = new vector<unsigned>();
	vector<unsigned> * k2Array = new vector<unsigned>();
	vector<unsigned> * k3Array = new vector<unsigned>();

	pifh.slop = 0;
	pifh.phraseKeyWords.clear();
	pti.queryPosition = 1; pti.recordPosition = k1Array; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = k2Array; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 1; pti.recordPosition = NULL; pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);

	pifh.slop = 0;
	pifh.phraseKeyWords.clear();
	pti.queryPosition = 1; pti.recordPosition = k1Array; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = NULL; pifh.phraseKeyWords.push_back(pti);
	pti.queryPosition = 2; pti.recordPosition = k3Array; pifh.phraseKeyWords.push_back(pti);
	phrasesInfoList.push_back(pifh);

	algo = new AnalyzerBasedAlgorithm(analyzer, keywordStrToHighlight, phrasesInfoList, hconf);

	printTestHeader(11);
	callSnippetGen(algo, records[2].c_str(), snippets);

	delete k1Array; delete k2Array; delete k3Array;
	delete algo;

	delete analyzer;
}



