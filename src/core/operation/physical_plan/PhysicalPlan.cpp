
#include "PhysicalPlan.h"
using namespace std;

namespace srch2 {
namespace instantsearch {



// This function is called to determine whether the output properties of an operator
// match the input properties of another one.
bool IteratorProperties::isMatchAsInputTo(const IteratorProperties & prop){
	for(vector<PhysicalPlanIteratorProperty>::const_iterator property = prop.properties.begin(); property != prop.properties.end() ; ++property){
		if(find(this->properties.begin(),this->properties.end(), *property) == this->properties.end()){
			return false;
		}
	}
	return true;
}

// Adds this property to the list of properties that this object has
void IteratorProperties::addProperty(PhysicalPlanIteratorProperty prop){
	this->properties.push_back(prop);
}


unsigned PhysicalPlanNode::getChildrenCount() {
	return children.size();
}

PhysicalPlanNode * PhysicalPlanNode::getChildAt(unsigned offset) {
	if(offset >= children.size() || offset < 0){
		ASSERT(false);
		return NULL;
	}
	return children.at(offset);
}

void PhysicalPlanNode::addChild(PhysicalPlanNode * child) {
	if(child == NULL){
		ASSERT(false);
		return;
	}
	children.push_back(child);
}


void PhysicalPlanNode::setParent(PhysicalPlanNode * parent){
	this->parent = parent;
}

PhysicalPlanNode * PhysicalPlanNode::getParent(){
	return parent;
}

PhysicalPlan::PhysicalPlan(ForwardIndex * forwardIndex , InvertedIndex * invertedIndex, Trie * trie, CatalogManager * catalogManager){
	this->forwardIndex = forwardIndex;
	this->invertedIndex = invertedIndex;
	this->trie = trie;
	this->catalogManager = catalogManager;
	this->tree = NULL;
}

PhysicalPlan::~PhysicalPlan(){

	if(tree != NULL) delete tree;
}


PhysicalPlanNode * PhysicalPlan::createNode(PhysicalPlanNodeType nodeType){

	// TODO : based on the type, one iterator must be allocated and returned.
	return NULL;
}

ForwardIndex * PhysicalPlan::getForwardIndex(){
	return this->forwardIndex;
}

InvertedIndex * PhysicalPlan::getInvertedIndex(){
	return this->invertedIndex;
}

Trie * PhysicalPlan::getTrie(){
	return this->trie;
}

CatalogManager * PhysicalPlan::getCatalogManager(){
	return this->catalogManager;
}

PhysicalPlanNode * PhysicalPlan::getPlanTree(){
	return this->tree;
}


}}
