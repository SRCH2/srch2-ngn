/*
 * NewQuadTree_Test.cpp
 *
 *  Created on: Jul 2, 2014
 *      Author: mahdi
 */


// For using this test case make sure that to set the value of all
// these parameters in src/core/geosearch/QuadTreeNode.h
// these variable should have exactly these values:
//const unsigned MAX_NUM_OF_ELEMENTS = 6;
//const double MIN_SEARCH_RANGE_SQUARE = (0.24 * 0.24);
//const double MIN_DISTANCE_SCORE = 0.05;
//const double MBR_LIMIT = (0.0005 * 0.0005);
//const unsigned CHILD_NUM_SQRT = 2;
//const unsigned CHILD_NUM = (CHILD_NUM_SQRT * CHILD_NUM_SQRT);

#include <iostream>
#include "src/core/geosearch/QuadTree.h"
#include "src/core/geosearch/QuadTreeNode.h"
#include "record/LocationRecordUtil.h"
#include <list>
using namespace std;
using namespace srch2::instantsearch;

bool verifyResults(vector<vector<GeoElement*>*> & results,vector<vector<GeoElement*>*> & expectedResults){
	if(results.size()!=expectedResults.size())
		return false;
	vector<GeoElement*> tmpRes;
	vector<GeoElement*> tmpExp;

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



void printResults(vector<vector<GeoElement*>*> & results){
	vector<GeoElement*> tmpRes;
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

bool testMergeNewQuadTree(QuadTree* quadtree){
	quadtree->remove(new GeoElement(10,10,3));
	quadtree->remove(new GeoElement(100,100,2));
	quadtree->remove(new GeoElement(-110,90,6));
	quadtree->remove(new GeoElement(-80,-100,10));

	// Storing results of the query and expected results
	vector<vector<GeoElement*>*> expectedResults;
	vector<vector<GeoElement*>*>  results;

	Rectangle rectangle;
	rectangle.max.x=20;
	rectangle.max.y=20;
	rectangle.min.x=10;
	rectangle.min.y=-10;
	quadtree->rangeQuery(results,rectangle);
	vector<GeoElement*> res;
	res.push_back(new GeoElement(0,0,1));
	res.push_back(new GeoElement(0,0,4));
	res.push_back(new GeoElement(0,0,5));
	res.push_back(new GeoElement(0,0,7));
	res.push_back(new GeoElement(0,0,9));
	expectedResults.push_back(&res);

	return verifyResults(results,expectedResults);
}

bool testRemoveElementNewQuadTree(QuadTree* quadtree){
	quadtree->remove(new GeoElement(10,10,8));

	// Storing results of the query and expected results
	vector<vector<GeoElement*>*> expectedResults;
	vector<vector<GeoElement*>*>  results;

	//Rectangle queryRange(pair(pair(-20,-20),pair(20,20)));
	Rectangle rectangle;
	rectangle.max.x=20;
	rectangle.max.y=20;
	rectangle.min.x=10;
	rectangle.min.y=-10;
	quadtree->rangeQuery(results,rectangle);
	vector<GeoElement*> res;
	res.push_back(new GeoElement(0,0,2));
	res.push_back(new GeoElement(0,0,3));
	res.push_back(new GeoElement(0,0,9));
	expectedResults.push_back(&res);
	vector<GeoElement*> res2;
	res2.push_back(new GeoElement(0,0,5));
	expectedResults.push_back(&res2);

	return verifyResults(results,expectedResults);
}

bool testMultiNodeNewQuadTree(QuadTree* quadtree){
	GeoElement* element1 = new GeoElement(-110,90,6);
	GeoElement* element2 = new GeoElement(-110,80,7);
	GeoElement* element3 = new GeoElement(10,10,8);
	GeoElement* element4 = new GeoElement(110,110,9);
	GeoElement* element5 = new GeoElement(-80,-100,10);

	quadtree->insert(element1);
	quadtree->insert(element2);
	quadtree->insert(element3);
	quadtree->insert(element4);
	quadtree->insert(element5);

	// Storing results of the query and expected results
	vector<vector<GeoElement*>*> expectedResults;
	vector<vector<GeoElement*>*>  results;

	//Rectangle queryRange(pair(pair(-20,-20),pair(20,20)));
	Rectangle rectangle;
	rectangle.max.x=20;
	rectangle.max.y=20;
	rectangle.min.x=10;
	rectangle.min.y=-10;
	quadtree->rangeQuery(results, rectangle);
	vector<GeoElement*> res;
	res.push_back(new GeoElement(0,0,2));
	res.push_back(new GeoElement(0,0,3));
	res.push_back(new GeoElement(0,0,8));
	res.push_back(new GeoElement(0,0,9));
	expectedResults.push_back(&res);
	vector<GeoElement*> res2;
	res2.push_back(new GeoElement(0,0,5));
	expectedResults.push_back(&res2);

	return verifyResults(results,expectedResults);
}

// Test the case where we only have one node(root) with a few records in the tree
bool testSingleNodeNewQuadTree(QuadTree* quadtree)
{
	GeoElement* element1 = new GeoElement(-100,100,1);
	GeoElement* element2 = new GeoElement(100,100,2);
	GeoElement* element3 = new GeoElement(1,1,3);
	GeoElement* element4 = new GeoElement(-100,-100,4);
	GeoElement* element5 = new GeoElement(100,-100,5);

	// Create five records
	quadtree->insert(element1);
	quadtree->insert(element2);
	quadtree->insert(element3);
	quadtree->insert(element4);
	quadtree->insert(element5);

	// Storing results of the query and expected results
	vector<vector<GeoElement*>*> expectedResults;
	vector<vector<GeoElement*>*>  results;

	//Rectangle queryRange(pair(pair(-20,-20),pair(20,20)));
	Rectangle rectangle;
	rectangle.max.x=20;
	rectangle.max.y=20;
	rectangle.min.x=10;
	rectangle.min.y=10;
	quadtree->rangeQuery(results,rectangle);
	vector<GeoElement*> res;
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
	cout << "NewQuadTree_Test" << endl;

	QuadTree* quadtree = new QuadTree();
	if(testSingleNodeNewQuadTree(quadtree))
		cout<< "Single Node QuadTree Test Passed" << endl;
	else
		cout<< "Single Node QuadTree Test failed" << endl;

	if(testMultiNodeNewQuadTree(quadtree)){
		cout<< "Multi Node QuadTree Test Passed" << endl << endl;

		cout<< "Split Node QuadTree Test Passed" << endl;
	}
	else{
		cout<< "Multi Node QuadTree Test failed" << endl << endl;

		cout<< "Split QuadTree Test failed" << endl;
	}

	if(testRemoveElementNewQuadTree(quadtree))
		cout<< "Remove Element QuadTree Test Passed" << endl;
	else
		cout<< "Remove Element QuadTree Test failed" << endl;

	if(testMergeNewQuadTree(quadtree))
		cout<< "Merge Node QuadTree Test Passed" << endl;
	else
		cout<< "Merge Node Element QuadTree Test failed" << endl;

	return 0;
}




