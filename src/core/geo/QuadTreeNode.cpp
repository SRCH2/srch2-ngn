/*
 * QuadTreeNode.cpp
 *
 *  Created on: Jul 1, 2014
 *      Author: mahdi
 */

#include <geo/QuadTreeNode.h>
#include "util/Logger.h"
#include "util/Assert.h"

using namespace std;
using srch2::util::Logger;

namespace srch2{
namespace instantsearch{

/*********GeoElement********************************************************/

GeoElement::GeoElement(const Record* record, unsigned int recordInternalId)
{
	this->point.x = record->getLocationAttributeValue().first;
	this->point.y = record->getLocationAttributeValue().second;
	this->forwardListID = recordInternalId;
}

GeoElement::GeoElement(double x, double y, unsigned int recordInternalId){
	this->point.x = x;
	this->point.y = y;
	this->forwardListID = recordInternalId;
}

double GeoElement::getDistance(const Shape & range){
	return sqrt(range.getMinDist2FromLatLong(this->point.x, this->point.y));
}

double GeoElement::getScore(const Shape & range){
	 // calculate the score
	double minDist2UpperBound = max( range.getSearchRadius2() , GEO_MIN_SEARCH_RANGE_SQUARE);
	double resultMinDist2 = range.getMinDist2FromLatLong(this->point.x, this->point.y);
	double distanceRatio = ((double)sqrt(minDist2UpperBound) - (double)sqrt(resultMinDist2)) / (double)sqrt(minDist2UpperBound);
	return max( distanceRatio * distanceRatio, GEO_MIN_DISTANCE_SCORE );
}

/*********QuadTreeNode******************************************************/

QuadTreeNode::QuadTreeNode(Rectangle &rectangle){
	this->rectangle = rectangle;
	this->isLeaf = true;
	this->numOfElementsInSubtree = 0;
	this->numOfLeafNodesInSubtree = 1;
}

QuadTreeNode::QuadTreeNode(Rectangle &rectangle, GeoElement* elements){
	this->rectangle = rectangle;
	this->isLeaf = true;
	this->numOfElementsInSubtree = 1;
	this->numOfLeafNodesInSubtree = 1;
	this->elements.push_back(elements);
}

QuadTreeNode::~QuadTreeNode(){
	if(!isLeaf){
		for(unsigned i = 0; i < children.size(); i++){
			if(children[i] != NULL)
				delete children[i];
		}
	}
	children.clear();
}

bool QuadTreeNode::insertGeoElement(GeoElement* element){
	// A leaf node //
	if(this->isLeaf){
		// Split the leaf if it is full
		if(this->numOfElementsInSubtree >= GEO_MAX_NUM_OF_ELEMENTS
				// For avoiding too small regions in the quadtree
				&& ((this->rectangle.max.x - this->rectangle.min.x) * (this->rectangle.max.y - this->rectangle.min.y)) > GEO_MBR_LIMIT){
			this->numOfLeafNodesInSubtree = this->split();
		}else{
			this->elements.push_back(element);
			this->numOfElementsInSubtree++;
			return true;
		}
	}
	// An internal node //
	this->numOfElementsInSubtree++;
	// Find the child based on geo information
	unsigned child = findChildContainingPoint(element->point);
	if(this->children[child] != NULL){ // This child is already created
		// recursively call this function at the corresponding child
		unsigned numOfLeafNodeInChild = this->children[child]->getNumOfLeafNodesInSubtree();
		bool flag = this->children[child]->insertGeoElement(element);
		this->numOfLeafNodesInSubtree += (this->children[child]->getNumOfLeafNodesInSubtree() - numOfLeafNodeInChild);
		return flag;
	}

	// The node doesn't have this child. We need to create a new child.
	// First create a new rectangle to assign to this child
	Rectangle newRectangle;
	createNewRectangle(newRectangle, this->rectangle, child);
	QuadTreeNode* newNode = new QuadTreeNode(newRectangle, element);

	// Put this new node in children
	this->children[child] = newNode;
	this->numOfLeafNodesInSubtree += newNode->getNumOfLeafNodesInSubtree();

	return true;
}

bool QuadTreeNode::removeGeoElement(GeoElement* element){

	if(this->isLeaf){ // A leaf node
		// Search the elements to find this element to remove it
		for(unsigned i = 0; i < this->elements.size(); i++){
			if(*this->elements[i] == *element){
				delete this->elements[i];
				this->elements.erase (this->elements.begin()+i);
				this->numOfElementsInSubtree--;
				return true;
			}
		}
	}else{ // An internal node
		// Find the child base on location information and recursively call this function at the corresponding child
		unsigned child = findChildContainingPoint(element->point);
		if(this->children[child] != NULL){
			unsigned numOfLeafNodeInChild = this->children[child]->getNumOfLeafNodesInSubtree();
			if( this->children[child]->removeGeoElement(element) ){
				this->numOfElementsInSubtree--;
				this->numOfLeafNodesInSubtree += (this->children[child]->getNumOfLeafNodesInSubtree() - numOfLeafNodeInChild);
				// if the number of elements in the subtree of this node is less than  MAX_NUM_OF_ELEMENTS we should merge this node.
				if(this->numOfElementsInSubtree < GEO_MAX_NUM_OF_ELEMENTS)
					mergeChildren();
				return true;
			}
		}
	}
	// This element doesn't exist in the tree
	return false;
}

void QuadTreeNode::getElements(vector <vector<GeoElement*>*> & results){
	if(this->isLeaf){ // leaf node - just add the pointer of its elements to the results
		results.push_back(&this->elements);
	}else{ // internal node - needs to get the elements of all its children recursively
		for(unsigned i = 0 ; i < this->children.size(); i++){
			if(this->children[i] != NULL ){
				this->children[i]->getElements(results);
			}
		}
	}
}

void QuadTreeNode::getElements(vector<GeoElement*> & results){
	if(this->isLeaf){ // leaf node - just add all of its elements to the results
		for(unsigned i = 0 ; i < this->elements.size() ; i++){
			results.push_back(this->elements[i]);
		}
	}else{ // internal node - needs to add the elements of its children recursively
		for(unsigned i = 0 ; i < this->children.size(); i++){
			if(this->children[i] != NULL)
				this->children[i]->getElements(results);
		}
	}
}

void QuadTreeNode::rangeQuery(vector<vector<GeoElement*>*> & results, const Shape &range){
	if(this->isLeaf){ // leaf node - just add the pointer of its elements to the results
		results.push_back(&this->elements);
	}else{ // internal node - needs to check range of its children with query range for pruning
		for(unsigned i = 0 ; i < GEO_CHILD_NUM ; i++){
			if(this->children[i] != NULL){
				if(range.intersects(this->children[i]->rectangle)){
					this->children[i]->rangeQuery(results, range);
				}
			}
		}
	}
}

void QuadTreeNode::rangeQuery(vector<QuadTreeNode*> & results, const Shape &range){
	if(range.contains(this->rectangle)){ // query range contains this node's rectangle.
		results.push_back(this);
	}else{ // call the rangeQuery for children of this node.
		for(unsigned i = 0 ; i < GEO_CHILD_NUM ; i++){
			if(this->children[i] != NULL){
				if(range.intersects(this->children[i]->rectangle)){
					this->children[i]->rangeQuery(results,range);
				}
			}
		}
	}
}

unsigned QuadTreeNode::split(){
	ASSERT(this->isLeaf == true);
	this->isLeaf = false; // after split, the node will no longer be a leaf
	this->numOfLeafNodesInSubtree = 0;
	// set child points to NULL.
	for(unsigned i = 0; i < GEO_CHILD_NUM; i++)
	        this->children.push_back(NULL);


	// Reinsert all the elements in this node
	for(unsigned i = 0; i < this->elements.size(); i++)
		insertGeoElement(this->elements[i]);

	// Now this node is an internal node. So we can clear its elements.
	this->elements.clear();
}

void QuadTreeNode::mergeChildren(){
	// This node should be an internal node for merge.
	ASSERT(this->numOfElementsInSubtree < GEO_MAX_NUM_OF_ELEMENTS );
	ASSERT(this->elements.size() == 0);
	ASSERT(this->isLeaf == false);


	getElements(this->elements); // Move all the elements in its subtree to node
	this->isLeaf = true; // this node become a leaf after merge
	this->numOfLeafNodesInSubtree = 1;
	for(unsigned i = 0 ; i < GEO_CHILD_NUM ; i++){
		delete this->children[i];
	}
	this->children.clear();
}

// this function calculates the index of the child containing this point based on the boundary of this node's rectangle
// for examle if a node has 4 children the number of children in the rectangle is like:
//      ----------------
//      |  2   |   3   |
//      ----------------
//      |  0   |   1   |
//      ----------------
unsigned QuadTreeNode::findChildContainingPoint(Point& point){
	double xRatio = (point.x - this->rectangle.min.x) / (this->rectangle.max.x - this->rectangle.min.x);
	double yRatio = (point.y - this->rectangle.min.y) / (this->rectangle.max.y - this->rectangle.min.y);

	unsigned x = (unsigned)(xRatio * GEO_CHILD_NUM_SQRT);
	unsigned y = (unsigned)(yRatio * GEO_CHILD_NUM_SQRT);

	if((x + y * GEO_CHILD_NUM_SQRT) >= GEO_CHILD_NUM){
		Logger::debug("The point is not in the range!! ");
		Logger::debug("%.10f,%d,%.10f,%.10f", yRatio, y, this->rectangle.min.y,  this->rectangle.max.y);
		Logger::debug("%.10f,%.10f", point.x, point.y);
	}

	return x + y * GEO_CHILD_NUM_SQRT;
}

void QuadTreeNode::createNewRectangle(Rectangle &newRectangle, const Rectangle &rectangle, const unsigned child){
	unsigned x = (unsigned)(child % GEO_CHILD_NUM_SQRT);
	unsigned y = (unsigned)(child / GEO_CHILD_NUM_SQRT);

	double single = (rectangle.max.x - rectangle.min.x) / GEO_CHILD_NUM_SQRT;

	newRectangle.min.x = rectangle.min.x + x * single;
	newRectangle.min.y = rectangle.min.y + y * single;
	newRectangle.max.x = rectangle.min.x + (x + 1) * single;
	newRectangle.max.y = rectangle.min.y + (y + 1) * single;
}

}
}

