//$Id: $
/*
 * pharseSearchTest.cpp
 *
 *  Created on: Aug 19, 2013
 *      Author: sbisht
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
#include "util/ULEB128.h"
#include "util/Logger.h"
#include "util/Assert.h"
#include <iostream>
#include "operation/PhraseSearcher.h"

using namespace std;
using namespace srch2::instantsearch;
//typedef char uint8_t;

void exactMatchTest1();
void exactMatchTest2();
void exactMatchTest3(map<std::string, std::vector<unsigned> >& piMap);

void proximityTest(map<std::string, std::vector<unsigned> >& piMap);

bool callExactMatch(vector<string>& inpKeywords, map<std::string, std::vector<unsigned> >& piMap);
bool buildPositionIndexes(const char * positionIndexFile, map<std::string, std::vector<unsigned> >& piMap);
bool callProximityMatch(const vector<string>& query, unsigned slop, const vector<vector<unsigned> >& positionListVector);
void getPositionIndexesForquery(const vector<string>& inpKeywords,
		map<std::string, std::vector<unsigned> >&piMap, vector<vector<unsigned> >& positionListVector);
void printPositionList(const vector<vector<unsigned> >& positionListVector);

PhraseSearcher *ps;

int main(int argc, char **argv)
{
	const char *positionIndexFileStr = getenv("positionIndexFile");
	if (positionIndexFileStr == NULL)
	{
		cout << "environment variable 'positionIndexFile' is not set" << endl;
		return -1;
	}
	map<std::string, std::vector<unsigned> > piMap;
	if (!buildPositionIndexes(positionIndexFileStr, piMap))
		return -1;
    ps = new PhraseSearcher();
    cout << "-----------Exact Match 1-----------" << endl;
    exactMatchTest1();
    cout << "-----------Exact Match 2-----------" << endl;
    exactMatchTest2();
    cout << "-----------Exact Match 3-----------" << endl;
    exactMatchTest3(piMap);
    cout << "-----------proximity test -----------" << endl;
    proximityTest(piMap);

}

void exactMatchTest1()
{
    vector<unsigned> listOfSlops;
    vector<vector<unsigned> > plv;
    vector<unsigned> l1;
    l1.push_back(2);l1.push_back(5);l1.push_back(7);l1.push_back(11);l1.push_back(77);
    plv.push_back(l1);
    vector<unsigned> l2;
    l2.push_back(6);l2.push_back(12);l2.push_back(44);l2.push_back(55);l2.push_back(78);
    plv.push_back(l2);
    vector<unsigned> l3;
    l3.push_back(4);l3.push_back(8);l3.push_back(13);l3.push_back(79);
    plv.push_back(l3);
    vector<unsigned> l4;
    l4.push_back(1);l4.push_back(3);l4.push_back(14);l4.push_back(20);l4.push_back(80);
    plv.push_back(l4);
    vector<vector<unsigned> > mvp;

    vector<unsigned> kpp;
    for (int i =0; i < plv.size(); ++i)
    {
    	kpp.push_back(i);
    }
    bool result = ps->exactMatch(plv, kpp, mvp, listOfSlops, true);
    ASSERT(result);

}

/*
 *   Test for duplicate terms
 */
void exactMatchTest2()
{
    vector<unsigned> listOfSlops;
    vector<vector<unsigned> > plv;
    vector<unsigned> l1;
    l1.push_back(2);l1.push_back(5);l1.push_back(7);l1.push_back(11);l1.push_back(15);l1.push_back(77);
    plv.push_back(l1);
    vector<unsigned> l2;
    l2.push_back(6);l2.push_back(12);l2.push_back(44);l2.push_back(55);l2.push_back(78);
    plv.push_back(l2);
    vector<unsigned> l3;
    l3.push_back(4);l3.push_back(8);l3.push_back(13);l3.push_back(79);
    plv.push_back(l3);
    vector<unsigned> l4;
    l4.push_back(1);l4.push_back(3);l4.push_back(14);l4.push_back(20);l4.push_back(80);
    plv.push_back(l4);
    vector<unsigned> l5;
    l5.push_back(2);l5.push_back(5);l5.push_back(7);l5.push_back(11);l5.push_back(15);l5.push_back(77);
    plv.push_back(l5);
    vector<vector<unsigned> >mvp;
    //mvp.reserve(plv.size());
    vector<unsigned> kpp;
    for (int i =0; i < plv.size(); ++i)
    {
    	kpp.push_back(i);
    }
    bool result = ps->exactMatch(plv, kpp, mvp, listOfSlops, true);
    ASSERT(result);

}

void exactMatchTest3(map<std::string, std::vector<unsigned> >& piMap)
{
	// phrase search
	vector<std::string> inpKeywords;
	inpKeywords.push_back("guide"); inpKeywords.push_back("them");
	bool result = callExactMatch(inpKeywords, piMap);
	ASSERT(result);
	inpKeywords.clear();
	inpKeywords.push_back("lord"); inpKeywords.push_back("of");inpKeywords.push_back("the");inpKeywords.push_back("rings");
	result = callExactMatch(inpKeywords, piMap);
	ASSERT(result);
	inpKeywords.clear();
	inpKeywords.push_back("follow"); inpKeywords.push_back("aragorn");
	result = callExactMatch(inpKeywords, piMap);
	ASSERT(result);
	inpKeywords.clear();
	inpKeywords.push_back("gimli");inpKeywords.push_back("legolas");
	result = callExactMatch(inpKeywords, piMap);
	ASSERT(!result);  // the result should be false
}


bool buildPositionIndexes(const char * testFIle, map<std::string, std::vector<unsigned> >& piMap){

	std::ifstream ifs;
	ifs.open(testFIle);
	if (!ifs.good())
	{
		std::cout << "could not read positional index file from " << testFIle << endl;
		return false;
	}
	string line;
	vector<unsigned> piVect;

	while(getline(ifs, line)) {
		vector<string> tokens;
		boost::split(tokens, line, boost::is_any_of(","));
		string keyword = tokens[0];

		boost::algorithm::trim(keyword);
		std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);

		piVect.clear();
		for (unsigned i = 1; i < tokens.size(); ++i)
		{
			unsigned val = (unsigned)atoi(tokens[i].c_str());
			if (val)
				piVect.push_back(val);
		}
		piMap[keyword] = piVect;
	}
	ifs.close();
	return true;
}

bool callExactMatch(vector<string>& inpKeywords, map<std::string, std::vector<unsigned> >& piMap)
{
    vector<vector<unsigned> > positionListVector;
    vector<unsigned> kpp;
    typedef map<std::string, std::vector<unsigned> >::iterator piMapIter;
    for (unsigned j =0; j < inpKeywords.size(); ++j)
    {
        string& tkn =  inpKeywords[j];
        piMapIter iter =  piMap.find((tkn));
        if (iter == piMap.end())
        {
            cout << "not found" << endl;
            return false;
        }
        positionListVector.push_back(iter->second);
        kpp.push_back(j);
    }
    vector<vector<unsigned> > matchedPositions;
    vector<unsigned> listOfSlops;
    bool match = ps->exactMatch(positionListVector, kpp, matchedPositions, listOfSlops, true);
    if (match)
    {
        for (unsigned i=0; i < matchedPositions.size(); ++i)
            cout << matchedPositions[0][i] << " ";

        cout << endl;
        return true;
    }
    else
    {
        cout << "not found" << endl;
        return false;
    }
}

void proximityTest(map<std::string, std::vector<unsigned> >& piMap) {

	vector<vector<unsigned> > positionListVector;
	vector<string> query;
	unsigned ed = 0;

	query.clear();
	string phrase = "lord rings";
	ed = 2;
	cout << "phrase = " << phrase << ", ed = " << ed <<  endl;
	boost::split(query, phrase, boost::is_any_of("\t "));
	getPositionIndexesForquery(query, piMap, positionListVector);
	printPositionList(positionListVector);
	bool result = callProximityMatch(query, ed, positionListVector);
	ASSERT(result);
	query.clear();
	positionListVector.clear();

	phrase = "gollum frodo sam";
	cout << "phrase = " << phrase << ", ed = " << ed << endl;
	boost::split(query, phrase, boost::is_any_of("\t "));
	getPositionIndexesForquery(query, piMap, positionListVector);
	printPositionList(positionListVector);
	result = callProximityMatch(query, ed, positionListVector);
	ASSERT(!result); // result should be false

	query.clear();
	positionListVector.clear();

	phrase = "frodo sam gollum";
	ed = 2;
	cout << "phrase = " << phrase << ", ed = " << ed << endl;
	boost::split(query, phrase, boost::is_any_of("\t "));
	getPositionIndexesForquery(query, piMap, positionListVector);
	printPositionList(positionListVector);
	result = callProximityMatch(query, ed, positionListVector);
	ASSERT(result);

	query.clear();
	positionListVector.clear();

	phrase = "gollum sauron";
	ed = 4;
	cout << "phrase = " << phrase << ", ed = " << ed << endl;
	boost::split(query, phrase, boost::is_any_of("\t "));
	getPositionIndexesForquery(query, piMap, positionListVector);
	printPositionList(positionListVector);
	result = callProximityMatch(query, ed, positionListVector);
	ASSERT(result);

}

bool callProximityMatch(const vector<string>& query,unsigned slop, const vector<vector<unsigned> >& positionListVector){

	vector<vector<unsigned> > matchedPositions;
	vector<unsigned> kpp;
	for (unsigned j =0; j < query.size(); ++j)
	{
		kpp.push_back(j);
	}
	vector<unsigned> listOfSlops;
	bool match = ps->proximityMatch(positionListVector, kpp, slop, matchedPositions, listOfSlops, true);
	if (match)
	{
		cout << "match found at position - " << endl;
		for (unsigned i=0; i < matchedPositions.size(); ++i)
			cout << matchedPositions[0][i] << " ";

		cout << endl;
		return true;
	}
	else
	{
		cout << "No match found" << endl;
		return false;
	}
}

void getPositionIndexesForquery(const vector<string>& inpKeywords,
		map<std::string, std::vector<unsigned> >&piMap, vector<vector<unsigned> >& positionListVector) {

	typedef map<std::string, std::vector<unsigned> >::iterator piMapIter;
	for (unsigned j =0; j < inpKeywords.size(); ++j)
	{
		const string& tkn =  inpKeywords[j];
		piMapIter iter =  piMap.find((tkn));
		if (iter == piMap.end())
		{
			positionListVector.clear();
			//cout << "not found" << end;
			return;
		}
		positionListVector.push_back(iter->second);
	}
}

void printPositionList(const vector<vector<unsigned> >& positionListVector){
	for (unsigned i =0; i < positionListVector.size(); ++i){
		vector<unsigned> cl = positionListVector[i];
		for (unsigned j = 0; j < cl.size(); ++j)
			cout << cl[j] << "  ";
		cout << endl;
	}
}
