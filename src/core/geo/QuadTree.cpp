/*
 * QuadTree.cpp
 *
 *  Created on: Jul 1, 2014
 *      Author: mahdi
 */

#include <geo/QuadTree.h>

using namespace std;

namespace srch2{
namespace instantsearch{

QuadTree::QuadTree(){
	Rectangle newRectangle;
    newRectangle.min.x = GEO_BOTTOM_LEFT_X;
    newRectangle.min.y = GEO_BOTTOM_LEFT_Y;
    newRectangle.max.x = GEO_TOP_RIGHT_X;
    newRectangle.max.y = GEO_TOP_RIGHT_Y;
	this->root = new QuadTreeNode(newRectangle);
}

QuadTree::~QuadTree(){
	delete this->root;
}

bool QuadTree::insert(const Record *record, unsigned recordInternalId){
	GeoElement* newElement = new GeoElement(record, recordInternalId);
	ASSERT(this->root != NULL);
	return this->root->insertGeoElement(newElement);
}

bool QuadTree::insert(GeoElement* element){
	ASSERT(this->root != NULL);
	return this->root->insertGeoElement(element);
}

bool QuadTree::remove(const Record *record, unsigned recordInternalId){
	GeoElement* element = new GeoElement(record, recordInternalId);
	ASSERT(this->root != NULL);
	return this->root->removeGeoElement(element);
}

bool QuadTree::remove(GeoElement* element){
	ASSERT(this->root != NULL);
	return this->root->removeGeoElement(element);
}

// For update first remove the element then insert the element again
bool QuadTree::update(const Record *record, unsigned recordInternalId){
	GeoElement* element = new GeoElement(record, recordInternalId);
	ASSERT(this->root != NULL);
	bool rm = this->root->removeGeoElement(element);
	bool in = this->root->insertGeoElement(element);
	return rm && in; // return true if both remove and insert are done successfully
}

// For update first remove the element then insert the element again
bool QuadTree::update(GeoElement* element){
	ASSERT(this->root != NULL);
	bool rm = this->root->removeGeoElement(element);
	bool in = this->root->insertGeoElement(element);
	return rm && in; // return true if both remove and insert are done successfully
}

void QuadTree::rangeQuery(vector<vector<GeoElement*>*> & results, const Shape &range) const{
	ASSERT(this->root != NULL);
	// First check the intersection of query range with the boundary of the root
	if(range.intersects(this->root->getRectangle())){
		this->root->rangeQuery(results, range);
	}
}

void QuadTree::rangeQuery(vector<QuadTreeNode*> & results, const Shape &range) const{
	ASSERT(this->root != NULL);
	// First check the intersection of query range with the boundary of the root
	if(range.intersects(this->root->getRectangle())){
		this->root->rangeQuery(results,range);
	}
}



}
}



