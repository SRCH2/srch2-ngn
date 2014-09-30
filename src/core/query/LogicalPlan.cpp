

#include "instantsearch/LogicalPlan.h"

#include "util/Assert.h"
#include "operation/HistogramManager.h"
#include "instantsearch/ResultsPostProcessor.h"
#include "instantsearch/Term.h"
#include "instantsearch/Query.h"
#include "sstream"

using namespace std;

namespace srch2 {
namespace instantsearch {


LogicalPlanNode::LogicalPlanNode(Term * exactTerm, Term * fuzzyTerm){
	this->nodeType = LogicalPlanNodeTypeTerm;
	this->exactTerm= exactTerm;
	this->fuzzyTerm = fuzzyTerm;
	this->regionShape = NULL;
	this->stats = NULL;
	this->forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

LogicalPlanNode::LogicalPlanNode(LogicalPlanNodeType nodeType){
	ASSERT(nodeType != LogicalPlanNodeTypeTerm);
	ASSERT(nodeType != LogicalPlanNodeTypeGeo);
	this->nodeType = nodeType;
	this->exactTerm= NULL;
	this->fuzzyTerm = NULL;
	this->stats = NULL;
	this->regionShape = NULL;
	this->forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

/*
 * This constructor will create an empty object for deserialization
 */
LogicalPlanNode::LogicalPlanNode(){
	this->nodeType = LogicalPlanNodeTypeAnd;
	this->exactTerm= NULL;
	this->fuzzyTerm = NULL;
	this->stats = NULL;
	this->regionShape = NULL;
	this->forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

LogicalPlanNode::LogicalPlanNode(const LogicalPlanNode & node){
	this->nodeType = node.nodeType;
	this->forcedPhysicalNode = node.forcedPhysicalNode;
	if(node.exactTerm != NULL){
		this->exactTerm = new Term(*(node.exactTerm));
	}else{
		this->exactTerm = NULL;
	}
	if(node.fuzzyTerm != NULL){
		this->fuzzyTerm = new Term(*(node.fuzzyTerm));
	}else{
		this->fuzzyTerm = NULL;
	}

	if(node.regionShape != NULL){
		ASSERT(node.nodeType == LogicalPlanNodeTypeGeo);
		switch (node.regionShape->getShapeType()) {
			case Shape::TypeRectangle:
				this->regionShape = new Rectangle(*(Rectangle *)(this->regionShape));
				break;
			case Shape::TypeCircle:
				this->regionShape = new Circle(*(Circle *)(this->regionShape));
				break;
			default :
				ASSERT(false);
				break;
		}
	}
	for(unsigned childIdx = 0 ; childIdx < node.children.size(); ++childIdx){
		if(node.children.at(childIdx)->nodeType == LogicalPlanNodeTypePhrase){
			this->children.push_back(
					new LogicalPlanPhraseNode(*((LogicalPlanPhraseNode *)(node.children.at(childIdx)))));
		}else{
			this->children.push_back(
					new LogicalPlanNode(*(node.children.at(childIdx))));
		}
	}
	//if(node.stats == NULL){
	this->stats = NULL; // stats will always be created new
	//}else{
	//	this->stats = new LogicalPlanNodeAnnotation(*(node.stats)); // TODO
	//}
}

LogicalPlanNode::LogicalPlanNode(Shape* regionShape){
	ASSERT(regionShape != NULL);
	this->nodeType = LogicalPlanNodeTypeGeo;
	this->exactTerm = NULL;
	this->fuzzyTerm = NULL;
	this->regionShape = regionShape;
	stats = NULL;
	forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

LogicalPlanNode::~LogicalPlanNode(){
	if(this->exactTerm != NULL){
		delete exactTerm;
	}
	if(this->fuzzyTerm != NULL){
		delete fuzzyTerm;
	}

	if(this->regionShape != NULL){
		delete regionShape;
	}
	for(vector<LogicalPlanNode *>::iterator child = children.begin() ; child != children.end() ; ++child){
		if(*child != NULL){
			delete *child;
		}
	}
	if(this->stats != NULL) delete this->stats;
}

void LogicalPlanNode::setFuzzyTerm(Term * fuzzyTerm){
	this->fuzzyTerm = fuzzyTerm;
}

string LogicalPlanNode::toString(){
	stringstream ss;
	ss << this->nodeType;
	if(this->exactTerm != NULL){
		ss << this->exactTerm->toString();
	}
	if(this->fuzzyTerm != NULL){
		ss << this->fuzzyTerm->toString();
	}
	if(this->regionShape != NULL){
		ss << this->regionShape->toString();
	}
	ss << this->forcedPhysicalNode;
	return ss.str();
}

string LogicalPlanNode::getSubtreeUniqueString(){

	string result = this->toString();
	for(unsigned childOffset = 0 ; childOffset < this->children.size() ; ++childOffset){
		ASSERT(this->children.at(childOffset) != NULL);
		result += this->children.at(childOffset)->getSubtreeUniqueString();
	}
	return result;
}

/*
 * Serialization scheme:
 * | nodeType | forcedPhysicalNode | isNULL | isNULL | [exactTerm] | [fuzzyTerm] | [phraseInfo (if type is LogicalPlanNodePhraseType)] | children |
 * NOTE : stats is NULL until logical plan reaches to the core so we don't serialize it...
 */
void * LogicalPlanNode::serializeForNetwork(void * buffer){

	buffer = srch2::util::serializeFixedTypes(this->nodeType, buffer);
	buffer = srch2::util::serializeFixedTypes(this->forcedPhysicalNode, buffer);

	buffer = srch2::util::serializeFixedTypes(bool(this->exactTerm != NULL), buffer);
	buffer = srch2::util::serializeFixedTypes(bool(this->fuzzyTerm != NULL), buffer);
	buffer = srch2::util::serializeFixedTypes(bool(this->regionShape != NULL), buffer);

	if(this->exactTerm != NULL){
		buffer = this->exactTerm->serializeForNetwork(buffer);
	}
	if(this->fuzzyTerm != NULL){
		buffer = this->fuzzyTerm->serializeForNetwork(buffer);
	}
	if(this->regionShape != NULL){
		buffer = this->regionShape->serializeForNetwork(buffer);
	}

	if(nodeType == LogicalPlanNodeTypePhrase){
		buffer = ((LogicalPlanPhraseNode *)this)->getPhraseInfo()->serializeForNetwork(buffer);
	}
	/*
	 * NOTE: we do not serialize stats because it's null and it only gets filled
	 * in query processing in core. This serialization is used for sending LogicalPlan
	 * from DPExternal to DPInternal so stats is not computed yet.
	 */

	// serialize the number of children
	buffer = srch2::util::serializeFixedTypes(unsigned(this->children.size()), buffer);
	// Serialize children
	for(unsigned childOffset = 0 ; childOffset < this->children.size() ; ++childOffset){
		ASSERT(this->children.at(childOffset) != NULL);
		buffer = this->children.at(childOffset)->serializeForNetwork(buffer);
	}

	return buffer;
}

/*
 * Serialization scheme:
 * | nodeType | forcedPhysicalNode | isNULL | isNULL | [exactTerm] | [fuzzyTerm] | [phraseInfo (if type is LogicalPlanNodePhraseType)] | children |
 * NOTE : stats is NULL until logical plan reaches to the core so we don't serialize it...
 */
void * LogicalPlanNode::deserializeForNetwork(LogicalPlanNode * &node, void * buffer){
	LogicalPlanNodeType type;
	buffer = srch2::util::deserializeFixedTypes(buffer, type);
	switch (type) {
		case LogicalPlanNodeTypeAnd:
		case LogicalPlanNodeTypeOr:
		case LogicalPlanNodeTypeTerm:
		case LogicalPlanNodeTypeNot:
		case LogicalPlanNodeTypeGeo:
			node = new LogicalPlanNode();
			break;
		case LogicalPlanNodeTypePhrase:
			node = new LogicalPlanPhraseNode();
			break;
	}
	node->nodeType = type;
	buffer = srch2::util::deserializeFixedTypes(buffer, node->forcedPhysicalNode);

	bool isExactTermNotNull = false;
	bool isFuzzyTermNotNull = false;
	bool isRegionShapeNotNull = false;
	buffer = srch2::util::deserializeFixedTypes(buffer, isExactTermNotNull); // not NULL
	buffer = srch2::util::deserializeFixedTypes(buffer, isFuzzyTermNotNull); // not NULL
	buffer = srch2::util::deserializeFixedTypes(buffer, isRegionShapeNotNull); // not NULL

	if(isExactTermNotNull){
		// just for memory allocation. This object gets filled in deserialization
		node->exactTerm = ExactTerm::create("",TERM_TYPE_NOT_SPECIFIED);
		buffer = Term::deserializeForNetwork(*node->exactTerm,buffer);
	}
	if(isFuzzyTermNotNull){
		// just for memory allocation. This object gets filled in deserialization
		node->fuzzyTerm = FuzzyTerm::create("",TERM_TYPE_NOT_SPECIFIED);
		buffer = Term::deserializeForNetwork(*node->fuzzyTerm,buffer);
	}

	if(isRegionShapeNotNull){
		ASSERT(node->nodeType == LogicalPlanNodeTypeGeo);
		buffer = Shape::deserializeForNetwork(node->regionShape, buffer);
	}

	if(node->nodeType == LogicalPlanNodeTypePhrase){
		buffer = ((LogicalPlanPhraseNode *)node)->getPhraseInfo()->deserializeForNetwork(buffer);
	}
	// get number of children
	unsigned numberOfChilren = 0;
	buffer = srch2::util::deserializeFixedTypes(buffer, numberOfChilren);
	// Deserialize children
	for(unsigned childOffset = 0 ; childOffset < numberOfChilren ; ++childOffset){
		LogicalPlanNode * newChild ;
		buffer = deserializeForNetwork(newChild, buffer);
		node->children.push_back(newChild);
	}

	return buffer;
}

/*
 * Serialization scheme:
 * | nodeType | forcedPhysicalNode | isNULL | isNULL | [exactTerm] | [fuzzyTerm] | [phraseInfo (if type is LogicalPlanNodePhraseType)] | children |
 * NOTE : stats is NULL until logical plan reaches to the core so we don't serialize it...
 */
unsigned LogicalPlanNode::getNumberOfBytesForSerializationForNetwork(){
	//calculate number of bytes
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(this->nodeType);
	numberOfBytes += sizeof(this->forcedPhysicalNode);

	numberOfBytes += sizeof(bool);
	if(this->exactTerm != NULL){
		numberOfBytes += this->exactTerm->getNumberOfBytesForSerializationForNetwork();
	}

	numberOfBytes += sizeof(bool);
	if(this->fuzzyTerm != NULL){
		numberOfBytes += this->fuzzyTerm->getNumberOfBytesForSerializationForNetwork();
	}

	numberOfBytes += sizeof(bool);
	if(this->regionShape != NULL){
		numberOfBytes += this->regionShape->getNumberOfBytesForSerializationForNetwork();
	}

	if(this->nodeType == LogicalPlanNodeTypePhrase){
		numberOfBytes +=  ((LogicalPlanPhraseNode *)this)->getPhraseInfo()->getNumberOfBytesForSerializationForNetwork();
	}

	// numberOfChilren
	numberOfBytes += sizeof(unsigned);
	// add number of bytes of chilren
	for(unsigned childOffset = 0 ; childOffset < this->children.size() ; ++childOffset){
		ASSERT(this->children.at(childOffset) != NULL);
		numberOfBytes += this->children.at(childOffset)->getNumberOfBytesForSerializationForNetwork();
	}
	return numberOfBytes;
}


//////////////////////////////////////////////// Logical Plan ///////////////////////////////////////////////

LogicalPlan::LogicalPlan(){
	this->tree = NULL;
	this->postProcessingInfo = NULL;
	this->fuzzyQuery = this->exactQuery = NULL;
	this->postProcessingPlan = NULL;
}

LogicalPlan::LogicalPlan(const LogicalPlan & logicalPlan){
	if(logicalPlan.tree != NULL){
	    if(logicalPlan.tree->nodeType == LogicalPlanNodeTypePhrase){
	        this->tree = new LogicalPlanPhraseNode(*((LogicalPlanPhraseNode *)(logicalPlan.tree)));
	    }else{
            this->tree = new LogicalPlanNode(*(logicalPlan.tree));
	    }
	}else{
		this->tree = NULL;
	}

	this->attributesToReturn = logicalPlan.attributesToReturn;

	this->offset = logicalPlan.offset;
	this->numberOfResultsToRetrieve = logicalPlan.numberOfResultsToRetrieve;
	this->shouldRunFuzzyQuery = logicalPlan.shouldRunFuzzyQuery;
	this->queryType = logicalPlan.queryType;
	this->docIdForRetrieveByIdSearchType = logicalPlan.docIdForRetrieveByIdSearchType;
	if(logicalPlan.exactQuery != NULL){
		this->exactQuery = new Query(*(logicalPlan.exactQuery));
	}else{
		this->exactQuery = NULL;
	}

	if(logicalPlan.fuzzyQuery != NULL){
		this->fuzzyQuery = new Query(*(logicalPlan.fuzzyQuery));
	}else{
		this->fuzzyQuery = NULL;
	}

	if(logicalPlan.postProcessingInfo != NULL){
		this->postProcessingInfo = new ResultsPostProcessingInfo(*(logicalPlan.postProcessingInfo));
	}else{
		this->postProcessingInfo = NULL;
	}

	if(logicalPlan.postProcessingPlan != NULL){
		this->postProcessingPlan = new ResultsPostProcessorPlan(*(logicalPlan.postProcessingPlan));
	}else{
		this->postProcessingPlan = NULL;
	}
}

LogicalPlan::~LogicalPlan(){
	if(this->tree != NULL) delete this->tree;
	if(this->postProcessingInfo != NULL){
		delete this->postProcessingInfo;
	}
	if(this->postProcessingPlan != NULL){
		delete this->postProcessingPlan;
	}
	if(this->exactQuery != NULL){
		delete this->exactQuery;
	}

	if(this->fuzzyQuery != NULL){
		delete this->fuzzyQuery;
	}
}

LogicalPlanNode * LogicalPlan::createTermLogicalPlanNode(const std::string &queryKeyword,
		TermType type,const float boost, const float fuzzyMatchPenalty,
		const uint8_t threshold , const vector<unsigned>& fieldFilter,ATTRIBUTES_OP attrOp){
	Term * term = new Term(queryKeyword, type, boost, fuzzyMatchPenalty, threshold);
	term->addAttributesToFilter(fieldFilter, attrOp);
	LogicalPlanNode * node = new LogicalPlanNode(term , NULL);
	return node;
}

LogicalPlanNode * LogicalPlan::createOperatorLogicalPlanNode(LogicalPlanNodeType nodeType){
	ASSERT(nodeType != LogicalPlanNodeTypeTerm);
	ASSERT(nodeType != LogicalPlanNodeTypeGeo);
	LogicalPlanNode * node = new LogicalPlanNode(nodeType);
	return node;
}
LogicalPlanNode * LogicalPlan::createPhraseLogicalPlanNode(const vector<string>& phraseKeyWords,
		const vector<unsigned>& phraseKeywordsPosition,
		short slop, const vector<unsigned>& fieldFilter, ATTRIBUTES_OP attrOp) {

	LogicalPlanNode * node = new LogicalPlanPhraseNode(phraseKeyWords, phraseKeywordsPosition,
			slop, fieldFilter, attrOp);
	return node;
}

LogicalPlanNode * LogicalPlan::createGeoLogicalPlanNode(Shape *regionShape){
	LogicalPlanNode * node = new LogicalPlanNode(regionShape);
	return node;
}


/*
 * This function returns a string representation of the logical plan
 * by concatenating different parts together. The call to getSubtreeUniqueString()
 * gives us a tree representation of the logical plan tree. For example is query is
 * q = FOO1 AND BAR OR FOO2
 * the string of this subtree is something like:
 * FOO1_BAR_FOO2_OR_AND
 */
string LogicalPlan::getUniqueStringForCaching(){
	stringstream ss;
	if(this->tree != NULL){
		ss << this->tree->getSubtreeUniqueString().c_str();
	}
	ss << this->docIdForRetrieveByIdSearchType;
	if(this->postProcessingInfo != NULL){
		ss << this->postProcessingInfo->toString().c_str();
	}
	ss << this->queryType;
	ss << this->offset;
	ss << this->numberOfResultsToRetrieve;
	ss << this->shouldRunFuzzyQuery;
	if(this->exactQuery != NULL){
		ss << this->exactQuery->toString().c_str();
	}
	if(this->fuzzyQuery != NULL){
		ss << this->fuzzyQuery->toString().c_str();
	}
	for(unsigned i = 0 ; i < this->attributesToReturn.size(); ++i){
		ss << this->attributesToReturn.at(i).c_str();
	}
	return ss.str();
}


/*
 * Serialization scheme :
 * | offset | numberOfResultsToRetrieve | shouldRunFuzzyQuery | queryType | \
 *  docIdForRetrieveByIdSearchType | isNULL | isNULL | isNULL | isNULL | \
 *   [exactQuery] | [fuzzyQuery] | [postProcessingInfo] | [tree] |
 */
void * LogicalPlan::serializeForNetwork(void * buffer){
	buffer = srch2::util::serializeFixedTypes(this->offset, buffer);
	buffer = srch2::util::serializeFixedTypes(this->numberOfResultsToRetrieve, buffer);
	buffer = srch2::util::serializeFixedTypes(this->shouldRunFuzzyQuery, buffer);
	buffer = srch2::util::serializeFixedTypes(this->queryType, buffer);
	buffer = srch2::util::serializeString(this->docIdForRetrieveByIdSearchType, buffer);

	buffer = srch2::util::serializeFixedTypes(bool(this->exactQuery != NULL), buffer); // isNULL
	buffer = srch2::util::serializeFixedTypes(bool(this->fuzzyQuery != NULL), buffer); // isNULL
	buffer = srch2::util::serializeFixedTypes(bool(this->postProcessingInfo != NULL),buffer); // isNULL
	//NOTE: postProcessingPlan must be removed completely. It's not used anymore
	buffer = srch2::util::serializeFixedTypes(bool(this->tree != NULL), buffer); // isNULL

	if(exactQuery != NULL){
		buffer = exactQuery->serializeForNetwork(buffer);
	}
	if(fuzzyQuery != NULL){
		buffer = fuzzyQuery->serializeForNetwork(buffer);
	}
	if(postProcessingInfo != NULL){
		buffer = postProcessingInfo->serializeForNetwork(buffer);
	}
	if(tree != NULL){
		buffer = tree->serializeForNetwork(buffer);
	}

	buffer = srch2::util::serializeVectorOfString(this->attributesToReturn, buffer);

	return buffer;
}

/*
 * Serialization scheme :
 * | offset | numberOfResultsToRetrieve | shouldRunFuzzyQuery | queryType | \
 *  docIdForRetrieveByIdSearchType | isNULL | isNULL | isNULL | isNULL | \
 *   [exactQuery] | [fuzzyQuery] | [postProcessingInfo] | [tree] |
 */
void * LogicalPlan::deserializeForNetwork(LogicalPlan & logicalPlan , void * buffer){

	buffer = srch2::util::deserializeFixedTypes(buffer, logicalPlan.offset);
	buffer = srch2::util::deserializeFixedTypes(buffer, logicalPlan.numberOfResultsToRetrieve);
	buffer = srch2::util::deserializeFixedTypes(buffer, logicalPlan.shouldRunFuzzyQuery);
	buffer = srch2::util::deserializeFixedTypes(buffer, logicalPlan.queryType);
	buffer = srch2::util::deserializeString(buffer, logicalPlan.docIdForRetrieveByIdSearchType);

	bool isExactQueryNotNull = false;
	buffer = srch2::util::deserializeFixedTypes(buffer, isExactQueryNotNull);
	bool isFuzzyQueryNotNull = false;
	buffer = srch2::util::deserializeFixedTypes(buffer, isFuzzyQueryNotNull);
	bool isPostProcessingInfoNotNull = false;
	buffer = srch2::util::deserializeFixedTypes(buffer, isPostProcessingInfoNotNull);
	bool isTreeNotNull = false;
	buffer = srch2::util::deserializeFixedTypes(buffer, isTreeNotNull);

	if(isExactQueryNotNull){
		logicalPlan.exactQuery = new Query(SearchTypeTopKQuery);
		buffer = Query::deserializeForNetwork(*logicalPlan.exactQuery, buffer);
	}else{
		logicalPlan.exactQuery = NULL;
	}
	if(isFuzzyQueryNotNull){
		logicalPlan.fuzzyQuery = new Query(SearchTypeTopKQuery);
		buffer = Query::deserializeForNetwork(*logicalPlan.fuzzyQuery, buffer);
	}else{
		logicalPlan.fuzzyQuery = NULL;
	}
	// NOTE: postProcessingPlan is not serialized because it's not used anymore and it must be deleted
	if(isPostProcessingInfoNotNull){
		logicalPlan.postProcessingInfo = new ResultsPostProcessingInfo();
		buffer = ResultsPostProcessingInfo::deserializeForNetwork(*logicalPlan.postProcessingInfo, buffer);
	}else{
		logicalPlan.postProcessingInfo = NULL;
	}
	if(isTreeNotNull){
		buffer = LogicalPlanNode::deserializeForNetwork(logicalPlan.tree, buffer);
	}else{
		logicalPlan.tree = NULL;
	}

	buffer = srch2::util::deserializeVectorOfString(buffer, logicalPlan.attributesToReturn);

	return buffer;
}

/*
 * Serialization scheme :
 * | offset | numberOfResultsToRetrieve | shouldRunFuzzyQuery | queryType | \
 *  docIdForRetrieveByIdSearchType | isNULL | isNULL | isNULL | isNULL | \
 *   [exactQuery] | [fuzzyQuery] | [postProcessingInfo] | [tree] |
 */
unsigned LogicalPlan::getNumberOfBytesForSerializationForNetwork(){
	//calculate number of bytes
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(this->offset);
	numberOfBytes += sizeof(this->numberOfResultsToRetrieve);
	numberOfBytes += sizeof(this->shouldRunFuzzyQuery);
	numberOfBytes += sizeof(this->queryType);
	numberOfBytes += sizeof(unsigned) + this->docIdForRetrieveByIdSearchType.size();

	numberOfBytes += sizeof(bool)*4; // isNULL
	// exact query
	if(this->exactQuery != NULL){
		numberOfBytes += this->exactQuery->getNumberOfBytesForSerializationForNetwork();
	}
	// fuzzy query
	if(this->fuzzyQuery != NULL){
		numberOfBytes += this->fuzzyQuery->getNumberOfBytesForSerializationForNetwork();
	}
	//NOTE: postProcessingPlan is not counted because it's not used and it must be deleted
	// postProcessingInfo
	if(this->postProcessingInfo != NULL){
		numberOfBytes += this->postProcessingInfo->getNumberOfBytesForSerializationForNetwork();
	}
	//tree
	if(this->tree != NULL){
		numberOfBytes += this->tree->getNumberOfBytesForSerializationForNetwork();
	}

	numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(this->attributesToReturn);

	return numberOfBytes;
}

}
}
