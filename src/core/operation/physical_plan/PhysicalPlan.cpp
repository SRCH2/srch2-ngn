
#include "PhysicalPlan.h"
#include "operation/QueryEvaluatorInternal.h"
#include "util/QueryOptimizerUtil.h"

using namespace std;

namespace srch2 {
namespace instantsearch {



// This function is called to determine whether the output properties of an operator
// match the input properties of another one.
bool IteratorProperties::isMatchAsInputTo(const IteratorProperties & prop , IteratorProperties & reason){
	bool result = true;
	for(vector<PhysicalPlanIteratorProperty>::const_iterator property = prop.properties.begin(); property != prop.properties.end() ; ++property){
		if(find(this->properties.begin(),this->properties.end(), *property) == this->properties.end()){
			reason.addProperty(*property);
			result = false;
		}
	}

	return result;
}

// Adds this property to the list of properties that this object has
void IteratorProperties::addProperty(PhysicalPlanIteratorProperty prop){
	this->properties.push_back(prop);
}


unsigned PhysicalPlanOptimizationNode::getChildrenCount() {
	return children.size();
}

PhysicalPlanOptimizationNode * PhysicalPlanOptimizationNode::getChildAt(unsigned offset) {
	if(offset >= children.size() || offset < 0){
		ASSERT(false);
		return NULL;
	}
	return children.at(offset);
}

void PhysicalPlanOptimizationNode::setChildAt(unsigned offset, PhysicalPlanOptimizationNode * child) {
	if(offset >= children.size() || offset < 0){
		ASSERT(false);
		return;
	}
	children.at(offset) = child;
}

void PhysicalPlanOptimizationNode::addChild(PhysicalPlanOptimizationNode * child) {
	if(child == NULL){
		ASSERT(false);
		return;
	}
	children.push_back(child);
}


// parent is not set in the code, it's going to be removed if not needed TODO
void PhysicalPlanOptimizationNode::setParent(PhysicalPlanOptimizationNode * parent){
	this->parent = parent;
}

PhysicalPlanOptimizationNode * PhysicalPlanOptimizationNode::getParent(){
	return parent;
}

void PhysicalPlanOptimizationNode::printSubTree(unsigned indent){
	PhysicalPlanNodeType type = getType();
	srch2::util::QueryOptimizerUtil::printIndentations(indent);
	switch (type) {
		case PhysicalPlanNode_SortById:
			cout << "[SortByID]" << endl;
			break;
		case PhysicalPlanNode_SortByScore:
			cout << "[SortByScore]" << endl;
			break;
		case PhysicalPlanNode_MergeTopK:
			cout << "[AND TopK]" << endl;
			break;
		case PhysicalPlanNode_MergeSortedById:
			cout << "[AND SortedByID]" << endl;
			break;
		case PhysicalPlanNode_MergeByShortestList:
			cout << "[AND ShortestList]" << endl;
			break;
		case PhysicalPlanNode_UnionSortedById:
			cout << "[OR SortedByID]" << endl;
			break;
		case PhysicalPlanNode_UnionSortedByScoreTopK:
			cout << "[OR SortedByScore TopK]" << endl;
			break;
		case PhysicalPlanNode_UnionSortedByScore:
			cout << "[OR SortedByScore]" << endl;
			break;
		case PhysicalPlanNode_UnionLowestLevelTermVirtualList:
			cout << "[TVL]" << endl;
			break;
		case PhysicalPlanNode_UnionLowestLevelSimpleScanOperator:
			cout << "[SCAN]" << endl;
			break;
		case PhysicalPlanNode_RandomAccessTerm:
			cout << "[TERM]" << endl;
			break;
		case PhysicalPlanNode_RandomAccessAnd:
			cout << "[R.A.AND]" << endl;
			break;
		case PhysicalPlanNode_RandomAccessOr:
			cout << "[R.A.OR]" << endl;
			break;
		case PhysicalPlanNode_RandomAccessNot:
			cout << "[R.A.NOT]" << endl;
			break;
	}
	for(vector<PhysicalPlanOptimizationNode *>::iterator child = children.begin(); child != children.end() ; ++child){
		(*child)->printSubTree(indent+1);
	}
}

void PhysicalPlanNode::setPhysicalPlanOptimizationNode(PhysicalPlanOptimizationNode * optimizationNode){
	this->optimizationNode = optimizationNode;
}
PhysicalPlanOptimizationNode * PhysicalPlanNode::getPhysicalPlanOptimizationNode(){
	return this->optimizationNode;
}

PhysicalPlan::PhysicalPlan(	QueryEvaluatorInternal * queryEvaluator){
	this->queryEvaluator = 	queryEvaluator;
	this->tree = NULL;
	this->ranker = NULL;
	this->executionParameters = NULL;
}

PhysicalPlan::~PhysicalPlan(){
	if(tree != NULL) delete tree;
	if(ranker != NULL) delete ranker;
	if(executionParameters != NULL) delete executionParameters;
}


PhysicalPlanNode * PhysicalPlan::createNode(PhysicalPlanNodeType nodeType){

	// TODO : based on the type, one iterator must be allocated and returned.
	return NULL;
}

ForwardIndex * PhysicalPlan::getForwardIndex(){
	return this->queryEvaluator->getForwardIndex();
}

const InvertedIndex * PhysicalPlan::getInvertedIndex(){
	return this->queryEvaluator->getInvertedIndex();
}

const Trie * PhysicalPlan::getTrie(){
	return this->queryEvaluator->getTrie();
}

PhysicalPlanNode * PhysicalPlan::getPlanTree(){
	return this->tree;
}

void PhysicalPlan::setPlanTree(PhysicalPlanNode * tree){
	this->tree = tree;
}

Ranker * PhysicalPlan::getRanker(){
	ASSERT(ranker != NULL);
	return this->ranker;
}


void PhysicalPlan::setSearchTypeAndRanker(srch2is::QueryType searchType){
	this->searchType = searchType;
	if(this->ranker != NULL){
		delete this->ranker;
	}
	switch (this->searchType) {
		case srch2is::SearchTypeTopKQuery:
			this->ranker = new DefaultTopKRanker();
			break;
		case srch2is::SearchTypeGetAllResultsQuery:
			this->ranker = new GetAllResultsRanker();
			break;
		case srch2is::SearchTypeMapQuery:
			this->ranker = new SpatialRanker();
			break;
		case srch2is::SearchTypeRetrievById:
			this->ranker = new DefaultTopKRanker();
			break;
	}
}

srch2is::QueryType PhysicalPlan::getSearchType(){
	return this->searchType;
}

void PhysicalPlan::setExecutionParameters(PhysicalPlanExecutionParameters * executionParameters){
	ASSERT(this->executionParameters == NULL && executionParameters != NULL);
	this->executionParameters = executionParameters;
}

PhysicalPlanExecutionParameters * PhysicalPlan::getExecutionParameters(){
	ASSERT(this->executionParameters != NULL);
	return this->executionParameters;
}


// getters
unsigned PhysicalPlanRecordItem::getRecordId() const {
	return this->recordId;
}
float PhysicalPlanRecordItem::getRecordStaticScore() const{
	return this->recordStaticScore;
}
float PhysicalPlanRecordItem::getRecordRuntimeScore() const{
	return this->recordRuntimeScore;
}
void PhysicalPlanRecordItem::getRecordMatchingPrefixes(vector<TrieNodePointer> & matchingPrefixes) const{
	matchingPrefixes.insert(matchingPrefixes.end(),this->matchingPrefixes.begin(),this->matchingPrefixes.end());
}
void PhysicalPlanRecordItem::getRecordMatchEditDistances(vector<unsigned> & editDistances) const{
	editDistances.insert(editDistances.end(),this->editDistances.begin(),this->editDistances.end());
}
void PhysicalPlanRecordItem::getRecordMatchAttributeBitmaps(vector<unsigned> & attributeBitmaps) const{
	attributeBitmaps.insert(attributeBitmaps.end(),this->attributeBitmaps.begin(),this->attributeBitmaps.end());
}
void PhysicalPlanRecordItem::getPositionIndexOffsets(vector<unsigned> & positionIndexOffsets)const {
	positionIndexOffsets.insert(positionIndexOffsets.end(),this->positionIndexOffsets.begin(),this->positionIndexOffsets.end());
}

// setters
void PhysicalPlanRecordItem::setRecordId(unsigned id) {
	this->recordId = id;
}
void PhysicalPlanRecordItem::setRecordStaticScore(float staticScore) {
	this->recordStaticScore = staticScore;
}
void PhysicalPlanRecordItem::setRecordRuntimeScore(float runtimeScore) {
	this->recordRuntimeScore = runtimeScore;
}
void PhysicalPlanRecordItem::setRecordMatchingPrefixes(const vector<TrieNodePointer> & matchingPrefixes) {
	this->matchingPrefixes = matchingPrefixes;
}
void PhysicalPlanRecordItem::setRecordMatchEditDistances(const vector<unsigned> & editDistances) {
	this->editDistances = editDistances;
}
void PhysicalPlanRecordItem::setRecordMatchAttributeBitmaps(const vector<unsigned> & attributeBitmaps) {
	this->attributeBitmaps = attributeBitmaps;
}
void PhysicalPlanRecordItem::setPositionIndexOffsets(const vector<unsigned> & positionIndexOffsets){
	this->positionIndexOffsets = positionIndexOffsets;
}

}}
