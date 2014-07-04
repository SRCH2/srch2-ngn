/*
 * QTree.cpp
 *
 *  Created on: Jul 1, 2014
 *      Author: mahdi
 */

#include <geosearch/QTree.h>

using namespace std;

namespace srch2{
namespace instantsearch{

QTree::QTree(){
	Rectangle newRectangle;
    newRectangle.min.x = BOTTOM_LEFT_X;
    newRectangle.min.y = BOTTOM_LEFT_Y;
    newRectangle.max.x = TOP_RIGHT_X;
    newRectangle.max.y = TOP_RIGHT_Y;
	this->root = new QTreeNode(newRectangle);
}

QTree::~QTree(){
	delete this->root;
}

bool QTree::insert(const Record *record, unsigned recordInternalId){
	PosElement* newElement = new PosElement(record, recordInternalId);
	return this->root->insertPosElement(newElement);
}

bool QTree::insert(PosElement* element){
	return this->root->insertPosElement(element);
}

bool QTree::remove(const Record *record, unsigned recordInternalId){
	PosElement* element = new PosElement(record, recordInternalId);
	return this->root->removePosElement(element);
}

bool QTree::remove(PosElement* element){
	return this->root->removePosElement(element);
}

// For update first remove the element then insert the element again
bool QTree::update(const Record *record, unsigned recordInternalId){
	PosElement* element = new PosElement(record, recordInternalId);
	bool rm = this->root->removePosElement(element);
	bool in = this->root->insertPosElement(element);
	return rm && in; // return true if both remove and insert are done successfully
}

// For update first remove the element then insert the element again
bool QTree::update(PosElement* element){
	bool rm = this->root->removePosElement(element);
	bool in = this->root->insertPosElement(element);
	return rm && in; // return true if both remove and insert are done successfully
}

void QTree::rangeQuery(vector<vector<PosElement*>*> & results, const Shape &range) const{
	// First check the intersection of query range with boundary of root
	if(range.intersects(this->root->getRectangle())){
		this->root->rangeQuery(results, range);
	}
}


}
}



