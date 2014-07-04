/*
 * QTree.h
 *
 *  Created on: Jul 1, 2014
 *      Author: mahdi
 */

#ifndef __QTREE_H__
#define __QTREE_H__

#include <vector>

#include "geosearch/QTreeNode.h"

using namespace std;

namespace srch2{
namespace instantsearch{

const double TOP_RIGHT_X = 200.0;    // The top right point of the maximum rectangle range of the whole quadtree
const double TOP_RIGHT_Y = 200.0;
const double BOTTOM_LEFT_X = -200.0;    // The bottom left point of the maximum rectangle range of the whole quadtree
const double BOTTOM_LEFT_Y = -200.0;

class QTree
{
private:
	QTreeNode* root;    // Pointer to the root of the Quadtree.

public:
	QTree();

	virtual ~QTree();

	// Insert a new record to the quadtree
	bool insert(const Record *record, unsigned recordInternalId);

	// Insert a new geo element to the quadtree
	bool insert(PosElement* element);

	// Remove the record from the quadtree
	bool remove(const Record *record, unsigned recordInternalId);

	// Remove the geo element from the quadtree
	bool remove(PosElement* element);

	// Update a record in the quadtree
	bool update(const Record *record, unsigned recordInternalId);

	// Update a geo element in the quadtree
	bool update(PosElement* element);

	// Find all the geo elements in the range
	void rangeQuery(vector<vector<PosElement*>*> & results, const Shape &range) const;

	QTreeNode* getRoot(){
		return this->root;
	};

};


}
}


#endif /* __QTREE_H__ */
