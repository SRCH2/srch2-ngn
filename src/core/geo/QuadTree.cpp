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

QuadTreeRootNodeAndFreeLists::QuadTreeRootNodeAndFreeLists(){
	Rectangle newRectangle;
	newRectangle.min.x = GEO_BOTTOM_LEFT_X;
	newRectangle.min.y = GEO_BOTTOM_LEFT_Y;
	newRectangle.max.x = GEO_TOP_RIGHT_X;
	newRectangle.max.y = GEO_TOP_RIGHT_Y;
	this->root = new QuadTreeNode(newRectangle);
}

QuadTreeRootNodeAndFreeLists::QuadTreeRootNodeAndFreeLists(const QuadTreeNode* src){
	this->root = new QuadTreeNode(src);
}

QuadTreeRootNodeAndFreeLists::~QuadTreeRootNodeAndFreeLists(){
	//delete both free list members
	for( vector<QuadTreeNode*>::iterator it = this->quadtreeNodes_free_list.begin();
			it != this->quadtreeNodes_free_list.end() ; ++it){
		delete *it;
	}
	for( vector<GeoElement*>::iterator it = this->geoElements_free_list.begin();
			it != this->geoElements_free_list.end() ; ++it){
		delete *it;
	}
	delete this->root;
}

QuadTree::QuadTree(){
	this->root_readview.reset(new QuadTreeRootNodeAndFreeLists());
	this->root_writeview = new QuadTreeNode(this->root_readview.get()->root);
	this->commited = false;
	this->mergeRequired = 0;
	pthread_spin_init(&m_spinlock, 0);
}

QuadTree::~QuadTree(){
	QuadTreeNode* root = this->root_writeview;
	if(root == NULL)
		return;
	for(unsigned childIterator = 0; childIterator < root->getChildren()->size(); ++childIterator){
		this->deleteQuadTreeNode(root->getChildren()->at(childIterator));
	}
	/* Free root_writeview pointer. The if condition is a defensive check to make sure
	 * that we do not get into a double free situation. 'root_readview' is a shared_pointer
	 * which automatically deletes the pointer it is holding.
	 */
	if(this->root_writeview != this->root_readview->root
			&& this->root_writeview != NULL)
		delete this->root_writeview;

	pthread_spin_destroy(&m_spinlock);
}

void QuadTree::deleteQuadTreeNode(QuadTreeNode* quadTreeNode){
	if(quadTreeNode == NULL)
		return;
	else{
		for(unsigned childIterator = 0; childIterator < quadTreeNode->getChildren()->size(); ++childIterator){
			this->deleteQuadTreeNode(quadTreeNode->getChildren()->at(childIterator));
		}
	}
	delete quadTreeNode;
	quadTreeNode = NULL;
}

void QuadTree::getQuadTreeRootNode_ReadView(boost::shared_ptr<QuadTreeRootNodeAndFreeLists> &quadTreeRootNode_ReadView) const{
	pthread_spin_lock(&m_spinlock);
	quadTreeRootNode_ReadView = this->root_readview;
	pthread_spin_unlock(&m_spinlock);
}

QuadTreeNode* QuadTree::getQuadTreeRootNode_WriteView() const{
	return this->root_writeview;
}

bool QuadTree::insert(const Record *record, unsigned recordInternalId){
	GeoElement* newElement = new GeoElement(record, recordInternalId);
	return this->insert(newElement);
}

bool QuadTree::insert(GeoElement* element){
	if(this->commited)
		return false;
	QuadTreeNode* root = this->root_writeview;
	ASSERT(root != NULL);
	return root->insertGeoElement(element);
}

bool QuadTree::insert_ThreadSafe(const Record* record, unsigned int recordInternalId){
	GeoElement* newElement = new GeoElement(record, recordInternalId);
	return this->insert_ThreadSafe(newElement);
}

bool QuadTree::insert_ThreadSafe(Point point, unsigned recordInternalId){
	GeoElement* newElement = new GeoElement(point.x, point.y, recordInternalId);
	return this->insert_ThreadSafe(newElement);
}

bool QuadTree::insert_ThreadSafe(GeoElement* element){
	this->mergeRequired = true;

	boost::shared_ptr<QuadTreeRootNodeAndFreeLists> quadTreeRootNode_ReadView;
	this->getQuadTreeRootNode_ReadView(quadTreeRootNode_ReadView);

	ASSERT(this->root_writeview != NULL);
	return this->root_writeview->insertGeoElement_ThreadSafe(element,quadTreeRootNode_ReadView);
}

bool QuadTree::remove_ThreadSafe(Point point, unsigned recordInternalId){
	GeoElement* element = new GeoElement(point.x, point.y, recordInternalId);
	return this->remove_ThreadSafe(element);
}

bool QuadTree::remove_ThreadSafe(GeoElement* element){
	this->mergeRequired = true;

	boost::shared_ptr<QuadTreeRootNodeAndFreeLists> quadTreeRootNode_ReadView;
	this->getQuadTreeRootNode_ReadView(quadTreeRootNode_ReadView);

	ASSERT(this->root_writeview != NULL);
	return this->root_writeview->removeGeoElement_ThreadSafe(element, quadTreeRootNode_ReadView);
}

// For update first remove the element then insert the element again
bool QuadTree::update_ThreadSafe(const Record *record, unsigned recordInternalId){
	GeoElement* element = new GeoElement(record, recordInternalId);
	return this->update_ThreadSafe(element);
}

// For update first remove the element then insert the element again
bool QuadTree::update_ThreadSafe(GeoElement* element){
	this->mergeRequired = true;

	boost::shared_ptr<QuadTreeRootNodeAndFreeLists> quadTreeRootNode_ReadView;
	this->getQuadTreeRootNode_ReadView(quadTreeRootNode_ReadView);

	ASSERT(this->root_writeview != NULL);
	bool rm = this->root_writeview->removeGeoElement_ThreadSafe(element, quadTreeRootNode_ReadView);
	bool in = this->root_writeview->insertGeoElement_ThreadSafe(element, quadTreeRootNode_ReadView);
	return rm && in; // return true if both remove and insert are done successfully
}

void QuadTree::rangeQuery(vector<vector<GeoElement*>*> & results, const Shape &range, QuadTreeNode* root) const{

	ASSERT(root != NULL);
	// First check the intersection of query range with the boundary of the root
	if(range.intersects(root->getRectangle())){
		root->rangeQuery(results, range);
	}
}

void QuadTree::rangeQuery(vector<QuadTreeNode*> & results, const Shape &range, QuadTreeNode* root) const{
	ASSERT(root != NULL);
	// First check the intersection of query range with the boundary of the root
	if(range.intersects(root->getRectangle())){
		root->rangeQuery(results,range);
	}
}

unsigned QuadTree::getTotalNumberOfGeoElements(){
	boost::shared_ptr<QuadTreeRootNodeAndFreeLists> quadTreeRootNode_ReadView;
	this->getQuadTreeRootNode_ReadView(quadTreeRootNode_ReadView);
	QuadTreeNode* root = quadTreeRootNode_ReadView->root;
	return root->getNumOfElementsInSubtree();
}

void QuadTree::commit(){
	ASSERT(commited == false);
	// remove the old readview's root first
	delete this->root_readview->root;
	// reset all the copy flags of nodes in the writeview to false
	this->root_writeview->resetCopyFlag();
	this->root_readview->root = this->root_writeview;
	// create a new write view's root by copying the root of the readview
	this->root_writeview = new QuadTreeNode(this->root_readview->root);
	this->commited = true;
}

void QuadTree::merge(){
	// first we traverse the tree from the writeview on copied nodes and change their
	// copy flag from true to false
	this->root_writeview->resetCopyFlag();

    // In each merge, we first put the current read view to the end of the queue,
    // and reset the current read view. Then we go through the read views one by one
    // in the order of their arrival. For each read view, we check its reference count.
    // If the count is > 1, then it means there are readers that are still using it,
    // so we do nothing and return. If the read view's reference count is 1,
    // then it means the current merge thread is the last thread using this read view,
    // so we can delete it and move onto the next read view on the queue.
    // We repeat the process until either we reach the end of the queue or we
    // find a read view with a reference count > 1.
	this->oldReadViewQueue.push(this->root_readview);
	pthread_spin_lock(&m_spinlock);
	this->root_readview.reset(new QuadTreeRootNodeAndFreeLists(this->root_writeview));
	// We can safely release the lock now, since the only chance the read view can be modified is during merge().
	// But merge() can only happen when another writer comes in, and we assume at any time only one writer can come in.
	// So this case cannot happen.
	pthread_spin_unlock(&m_spinlock);
	while(!this->oldReadViewQueue.empty() && this->oldReadViewQueue.front().unique()){
		this->oldReadViewQueue.pop();
	}
	if(this->root_writeview){
		delete this->root_writeview;
	}
	this->root_writeview = new QuadTreeNode(this->root_readview.get()->root);
}



}
}



