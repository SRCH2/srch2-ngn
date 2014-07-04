/*
 * NewQuadTree_Test.cpp
 *
 *  Created on: Jul 2, 2014
 *      Author: mahdi
 */


// For using this test case make sure that to set the value of all
// these parameters in src/core/geosearch/QTreeNode.h
// these variable should have exactly these values:
//const unsigned MAX_NUM_OF_ELEMENTS = 6;
//const double MIN_SEARCH_RANGE_SQUARE = (0.24 * 0.24);
//const double MIN_DISTANCE_SCORE = 0.05;
//const double MBR_LIMIT = (0.0005 * 0.0005);
//const unsigned CHILD_NUM_SQRT = 2;
//const unsigned CHILD_NUM = (CHILD_NUM_SQRT * CHILD_NUM_SQRT);

#include <iostream>
#include "src/core/geosearch/QTree.h"
#include "src/core/geosearch/QTreeNode.h"
#include "record/LocationRecordUtil.h"
#include <list>
using namespace std;
using namespace srch2::instantsearch;

bool verifyResults(vector<vector<PosElement*>*> & results,vector<vector<PosElement*>*> & expectedResults){
	if(results.size()!=expectedResults.size())
		return false;
	vector<PosElement*> tmpRes;
	vector<PosElement*> tmpExp;

	list<unsigned> resList;
	list<unsigned> expList;

	for(int i = 0;i<results.size();i++){
		tmpRes = *results[i];
		tmpExp =*expectedResults[i];
		for(int j = 0;j<tmpRes.size();j++){
			resList.push_back(tmpRes[j]->forwardListID);
		}
		for(int j = 0;j<tmpExp.size();j++){
			expList.push_back(tmpExp[j]->forwardListID);
		}
	}
	if(resList.size()!=expList.size())
		return false;
	resList.sort();
	expList.sort();

	list<unsigned>::const_iterator iterator1 = resList.begin();
	list<unsigned>::const_iterator iterator2 = expList.begin();
	while(iterator1 != resList.end()) {
		if(*iterator1 != *iterator2)
			return false;
		iterator1++;
		iterator2++;
	}
	cout << endl;
	return true;
}



void printResults(vector<vector<PosElement*>*> & results){
	vector<PosElement*> tmpRes;
	list<unsigned> resList;

	for(int i = 0;i<results.size();i++){
		tmpRes = *results[i];
		for(int j = 0;j<tmpRes.size();j++){
			resList.push_back(tmpRes[j]->forwardListID);
		}
	}
	resList.sort();

	list<unsigned>::const_iterator iterator1 = resList.begin();
	while(iterator1 != resList.end()) {
		cout << *iterator1 << "**";
		iterator1++;
	}
	cout << endl;
}

bool testMergeNewQTree(QTree* quadtree){
	quadtree->remove(new PosElement(10,10,3));
	quadtree->remove(new PosElement(100,100,2));
	quadtree->remove(new PosElement(-110,90,6));
	quadtree->remove(new PosElement(-80,-100,10));

	// Storing results of the query and expected results
	vector<vector<PosElement*>*> expectedResults;
	vector<vector<PosElement*>*>  results;

	Rectangle rectangle;
	rectangle.max.x=20;
	rectangle.max.y=20;
	rectangle.min.x=10;
	rectangle.min.y=-10;
	quadtree->rangeQuery(results,rectangle);
	vector<PosElement*> res;
	res.push_back(new PosElement(0,0,1));
	res.push_back(new PosElement(0,0,4));
	res.push_back(new PosElement(0,0,5));
	res.push_back(new PosElement(0,0,7));
	res.push_back(new PosElement(0,0,9));
	expectedResults.push_back(&res);

	return verifyResults(results,expectedResults);
}

bool testRemoveElementNewQTree(QTree* quadtree){
	quadtree->remove(new PosElement(10,10,8));

	// Storing results of the query and expected results
	vector<vector<PosElement*>*> expectedResults;
	vector<vector<PosElement*>*>  results;

	//Rectangle queryRange(pair(pair(-20,-20),pair(20,20)));
	Rectangle rectangle;
	rectangle.max.x=20;
	rectangle.max.y=20;
	rectangle.min.x=10;
	rectangle.min.y=-10;
	quadtree->rangeQuery(results,rectangle);
	vector<PosElement*> res;
	res.push_back(new PosElement(0,0,2));
	res.push_back(new PosElement(0,0,3));
	res.push_back(new PosElement(0,0,9));
	expectedResults.push_back(&res);
	vector<PosElement*> res2;
	res2.push_back(new PosElement(0,0,5));
	expectedResults.push_back(&res2);

	return verifyResults(results,expectedResults);
}

bool testMultiNodeNewQTree(QTree* quadtree){
	PosElement* element1 = new PosElement(-110,90,6);
	PosElement* element2 = new PosElement(-110,80,7);
	PosElement* element3 = new PosElement(10,10,8);
	PosElement* element4 = new PosElement(110,110,9);
	PosElement* element5 = new PosElement(-80,-100,10);

	quadtree->insert(element1);
	quadtree->insert(element2);
	quadtree->insert(element3);
	quadtree->insert(element4);
	quadtree->insert(element5);

	// Storing results of the query and expected results
	vector<vector<PosElement*>*> expectedResults;
	vector<vector<PosElement*>*>  results;

	//Rectangle queryRange(pair(pair(-20,-20),pair(20,20)));
	Rectangle rectangle;
	rectangle.max.x=20;
	rectangle.max.y=20;
	rectangle.min.x=10;
	rectangle.min.y=-10;
	quadtree->rangeQuery(results, rectangle);
	vector<PosElement*> res;
	res.push_back(new PosElement(0,0,2));
	res.push_back(new PosElement(0,0,3));
	res.push_back(new PosElement(0,0,8));
	res.push_back(new PosElement(0,0,9));
	expectedResults.push_back(&res);
	vector<PosElement*> res2;
	res2.push_back(new PosElement(0,0,5));
	expectedResults.push_back(&res2);

	return verifyResults(results,expectedResults);
}

// Test the case where we only have one node(root) with a few records in the tree
bool testSingleNodeNewQTree(QTree* quadtree)
{
	PosElement* element1 = new PosElement(-100,100,1);
	PosElement* element2 = new PosElement(100,100,2);
	PosElement* element3 = new PosElement(1,1,3);
	PosElement* element4 = new PosElement(-100,-100,4);
	PosElement* element5 = new PosElement(100,-100,5);

	// Create five records
	quadtree->insert(element1);
	quadtree->insert(element2);
	quadtree->insert(element3);
	quadtree->insert(element4);
	quadtree->insert(element5);

	// Storing results of the query and expected results
	vector<vector<PosElement*>*> expectedResults;
	vector<vector<PosElement*>*>  results;

	//Rectangle queryRange(pair(pair(-20,-20),pair(20,20)));
	Rectangle rectangle;
	rectangle.max.x=20;
	rectangle.max.y=20;
	rectangle.min.x=10;
	rectangle.min.y=10;
	quadtree->rangeQuery(results,rectangle);
	vector<PosElement*> res;
	res.push_back(element1);
	res.push_back(element2);
	res.push_back(element3);
	res.push_back(element4);
	res.push_back(element5);
	expectedResults.push_back(&res);

	return verifyResults(results,expectedResults);

}

int main(int argc, char *argv[])
{
	cout << "NewQTree_Test" << endl;

	QTree* quadtree = new QTree();
	if(testSingleNodeNewQTree(quadtree))
		cout<< "Single Node QTree Test Passed" << endl;
	else
		cout<< "Single Node QTree Test failed" << endl;

	if(testMultiNodeNewQTree(quadtree)){
		cout<< "Multi Node QTree Test Passed" << endl << endl;

		cout<< "Split Node QTree Test Passed" << endl;
	}
	else{
		cout<< "Multi Node QTree Test failed" << endl << endl;

		cout<< "Split QTree Test failed" << endl;
	}

	if(testRemoveElementNewQTree(quadtree))
		cout<< "Remove Element QTree Test Passed" << endl;
	else
		cout<< "Remove Element QTree Test failed" << endl;

	if(testMergeNewQTree(quadtree))
		cout<< "Merge Node QTree Test Passed" << endl;
	else
		cout<< "Merge Node Element QTree Test failed" << endl;

	return 0;
}




