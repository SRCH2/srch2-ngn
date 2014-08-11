/*
 * QuadTreeNode.h
 *
 *  Created on: Jul 1, 2014
 *      Author: mahdi
 */

#ifndef __QUADTREENODE_H__
#define __QUADTREENODE_H__

#include <stdlib.h>
#include <vector>
#include <instantsearch/Record.h>
#include <instantsearch/Ranker.h>
#include "record/LocationRecordUtil.h"


using namespace std;

namespace srch2{
namespace instantsearch{

const unsigned GEO_MAX_NUM_OF_ELEMENTS = 32;    // The maximum number of GeoElements each leaf node can have
const double GEO_MIN_SEARCH_RANGE_SQUARE = (0.24 * 0.24);    // The largest range we should search for, in degree (used in calculating the score)
const double GEO_MIN_DISTANCE_SCORE = 0.05;
const double GEO_MBR_LIMIT = (0.0005 * 0.0005); //0.005 The min size of a single rectangle
const unsigned GEO_CHILD_NUM_SQRT = 2;    // Square root of the maximum number of children each intermediate node can have
const unsigned GEO_CHILD_NUM = (GEO_CHILD_NUM_SQRT * GEO_CHILD_NUM_SQRT);

class QuadTreeNode;
class GeoElement;

class QuadTreeRootNodeAndFreeLists{
public:
	vector<QuadTreeNode*> quadtreeNodes_free_list;
	vector<GeoElement*>   geoElements_free_list;
	QuadTreeNode* root;

	QuadTreeRootNodeAndFreeLists();
	QuadTreeRootNodeAndFreeLists(const QuadTreeNode* src);
	~QuadTreeRootNodeAndFreeLists();

private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version){
		ar & this->root;
	}
};

class GeoElement
{
public:

	Point point;            // The location point of the record
	unsigned forwardListID; // Offset of this record in the forwardlist

	GeoElement(){};

	GeoElement(const Record *record, unsigned recordInternalId);

	GeoElement(double x, double y, unsigned int recordInternalId);

	virtual ~GeoElement(){};

	// Two Elements are equal if they have same recordID
	bool operator==(const GeoElement &e) const
	{
		return forwardListID == e.forwardListID;
	};

	// Return distance of this record from the center of the range
	double getDistance(const Shape &range);

	// Return geo score of this record for a specific range
	double getScore(const Shape &range);

private:

	friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->forwardListID;
        ar & this->point;
    }
};

class QuadTreeNode
{
public:
	QuadTreeNode(){
		this->isCopy = false;
	};

	QuadTreeNode(Rectangle &rectangle);

	QuadTreeNode(Rectangle &rectangle, GeoElement* elements);

	QuadTreeNode(const QuadTreeNode* src);

	virtual ~QuadTreeNode();

	// Insert new geo element to elements for the leaf nodes or insert
	// the new geo elements in the subtree of this node for the internal nodes.
	bool insertGeoElement(GeoElement* element);

	bool insertGeoElement_ThreadSafe(GeoElement* element, boost::shared_ptr<QuadTreeRootNodeAndFreeLists> quadTreeRootNode_ReadView);

	// Remove a geo element from the elements for the leaf nodes or remove
	// the geo element from the subtree of this node for the internal nodes.
	bool removeGeoElement_ThreadSafe(GeoElement* element, boost::shared_ptr<QuadTreeRootNodeAndFreeLists> quadTreeRootNode_ReadView);

	// return all the elements of the node for leaf nodes or return all the elements
	// in the subtree of this node for internal nodes.
	// it doesn't append elements of different leaf nodes with each other
	// it just return a vector of pointer to these vector of elements
	void getElements(vector <vector<GeoElement*>*> & results);

	// return all the geo elements in the query range
	void rangeQuery(vector<vector<GeoElement*>*> & results, const Shape &range);

	// Find all the quadtree nodes inside the query range.
	// If the query's rectangle contains the rectangles of all the nodes in a subtree, it only returns the root of that subtree.
	void rangeQuery(vector<QuadTreeNode*> & results, const Shape &range);

	Rectangle getRectangle(){
		return this->rectangle;
	}

	unsigned getNumOfElementsInSubtree(){
		return this->numOfElementsInSubtree;
	}

	bool getIsLeaf(){
		return this->isLeaf;
	}

	vector<QuadTreeNode*>* getChildren(){
		return &children;
	}

	// returns the elements of this node
	vector<GeoElement*>* getElements(){
		return &elements;
	}

	double aggregateValueByJointProbabilityDouble(double p1, double p2){
		return p1+p2;
	}

	unsigned getNumOfLeafNodesInSubtree(){
		return this->numOfLeafNodesInSubtree;
	}

	void resetCopyFlag();

	bool equalTo(QuadTreeNode* node);

private:
	Rectangle rectangle;               // Rectangle boundary of the node
	bool isLeaf;                       // true->Leaf (children is null), false->internal (elements is null)
	vector<QuadTreeNode*> children;    // Pointers to the children if this node is an internal node
	unsigned numOfElementsInSubtree;   // Number of geo elements in the subtree of this node
	unsigned numOfLeafNodesInSubtree;  // Number of Leaf nodes in the subtree of this node.(if this node is a leaf then numOfLeafNodesInSubtree=1)
	vector<GeoElement*> elements;      // Store geo elements if this node is a leaf node

	// we use this flag for concurrency control. so when we made a copy of this node for
	// insertion we don't need to copy it again for next insertions. and in the merge we change
	// this flag to false;
	bool isCopy;


	// Split the leaf node and make it an internal node if the number
	// of elements is greater than MAX_NUM_OF_ELEMENTS
	void split();

	// Merge all the children of this node if the total number of geo elements in
	// them is less than or equal to MAX_NUM_OF_ELEMENTS
	void mergeChildren(boost::shared_ptr<QuadTreeRootNodeAndFreeLists> quadTreeRootNode_ReadView);

	// Find offset of the child which rectangle contains this point
	unsigned findChildContainingPoint(Point& point);

	// Create a new rectangle based on the child's offset and parent's rectangle
	void createNewRectangle(Rectangle &newrectangle, const Rectangle &rectangle, const unsigned child);

	// Return all the elements of the subtree of this node
	void getElements(vector<GeoElement*> & results);

	friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->rectangle;
        ar & this->isLeaf;
        ar & this->children;
        ar & this->numOfElementsInSubtree;
        ar & this->numOfLeafNodesInSubtree;
        ar & this->elements;
        ar & this->isCopy;
    }
};


}
}


#endif /* __QUADTREENODE_H__ */
