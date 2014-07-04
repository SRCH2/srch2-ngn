/*
 * QTreeNode.h
 *
 *  Created on: Jul 1, 2014
 *      Author: mahdi
 */

#ifndef __QTREENODE_H__
#define __QTREENODE_H__

#include <stdlib.h>
#include <vector>
#include <instantsearch/Record.h>
#include <instantsearch/Ranker.h>
#include "record/LocationRecordUtil.h"


using namespace std;

namespace srch2{
namespace instantsearch{

const unsigned MAX_NUM_OF_ELEMENTS = 6;    // The maximum number of PosElements each leaf node can have
const double MIN_SEARCH_RANGE_SQUARE = (0.24 * 0.24);    // The largest range we should search for, in degree (used in calculating the score)
const double MIN_DISTANCE_SCORE = 0.05;
const double MBR_LIMIT = (0.0005 * 0.0005); //0.005 The min size of a single rectangle
const unsigned CHILD_NUM_SQRT = 2;    // Square root of the maximum number of children each intermediate node can have
const unsigned CHILD_NUM = (CHILD_NUM_SQRT * CHILD_NUM_SQRT);

class PosElement
{
public:

	Point point;            // The location point of the record
	unsigned forwardListID; // Offset of this record in the forwardlist

	PosElement(){};

	PosElement(const Record *record, unsigned recordInternalId);

	PosElement(double x, double y, unsigned int recordInternalId);

	virtual ~PosElement(){};

	// Two Elements are equal if they have same recordID
	bool operator==(const PosElement &e) const
	{
		return forwardListID == e.forwardListID;
	};

	// Return distance of this record from the center of the range
	double getDistance(const Shape &range);

	// Return geo score of this record for a specific range
	double getScore(const SpatialRanker *ranker, const Shape &range);

};

class QTreeNode
{
public:
	QTreeNode(){};

	QTreeNode(Rectangle rectangle);

	QTreeNode(Rectangle rectangle, PosElement* elements);

	virtual ~QTreeNode();

	// Insert new geo element to elements for the leaf nodes or insert
	// the new geo elements in the subtree of this node for the internal nodes.
	bool insertPosElement(PosElement* element);

	// Remove a geo element from the elements for the leaf nodes or remove
	// the geo element from the subtree of this node for the internal nodes.
	bool removePosElement(PosElement* element);

	// return all the elements of the node for leaf nodes or return all the elements
	// in the subtree of this node for internal nodes.
	// it doesn't append elements of different leaf nodes with each other
	// it just return a vector of pointer to these vector of elements
	void getElements(vector <vector<PosElement*>*> & results);

	// return all the geo elements in the query range
	void rangeQuery(vector<vector<PosElement*>*> & results, const Shape &range);

	Rectangle getRectangle(){
		return this->rectangle;
	}

private:
	Rectangle rectangle;            // Rectangle boundary of the node
	bool isLeaf;                    // true->Leaf (children is null), false->internal (elements is null)
	vector<QTreeNode*> children;    // Pointers to the children if this node is an internal node
	int numOfElementsInSubtree;     // Number of geo elements in the subtree of this node
	vector<PosElement*> elements;   // Store geo elements if this node is a leaf node

	// Split the leaf node and make it an internal node if the number
	// of elements is greater than MAX_NUM_OF_ELEMENTS
	void split();

	// Merge all the children of this node if the total number of geo elements in
	// them is less than or equal to MAX_NUM_OF_ELEMENTS
	void mergeChildren();

	// Find offset of the child which rectangle contains this point
	unsigned findChildContainingPoint(Point& point);

	// Create a new rectangle based on the child's offset and parent's rectangle
	void createNewRectangle(Rectangle &newrectangle, const Rectangle &rectangle, const unsigned child);

	// Return all the elements of the subtree of this node
	void getElements(vector<PosElement*> & results);
};


}
}


#endif /* __QTREENODE_H__ */
