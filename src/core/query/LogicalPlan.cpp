

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
	stats = NULL;
	forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

LogicalPlanNode::LogicalPlanNode(LogicalPlanNodeType nodeType){
	ASSERT(nodeType != LogicalPlanNodeTypeTerm);
	this->nodeType = nodeType;
	this->exactTerm= NULL;
	this->fuzzyTerm = NULL;
	stats = NULL;
	forcedPhysicalNode = PhysicalPlanNode_NOT_SPECIFIED;
}

/*
 * This constructor will create an empty object for deserialization
 */
LogicalPlanNode::LogicalPlanNode(){
	this->nodeType = LogicalPlanNodeTypeAnd;
	this->exactTerm= NULL;
	this->fuzzyTerm = NULL;
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
	for(vector<LogicalPlanNode *>::iterator child = children.begin() ; child != children.end() ; ++child){
		if(*child != NULL){
			delete *child;
		}
	}
	if(stats != NULL) delete stats;
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

	buffer = srch2::util::serializeFixedTypes(nodeType, buffer);
	buffer = srch2::util::serializeFixedTypes(forcedPhysicalNode, buffer);

	buffer = srch2::util::serializeFixedTypes(this->exactTerm != NULL, buffer);
	buffer = srch2::util::serializeFixedTypes(this->fuzzyTerm != NULL, buffer);

	if(this->exactTerm != NULL){
		buffer = this->exactTerm->serializeForNetwork(buffer);
	}
	if(this->fuzzyTerm != NULL){
		buffer = this->fuzzyTerm->serializeForNetwork(buffer);
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
	buffer = srch2::util::serializeFixedTypes(this->children.size(), buffer);
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
void * LogicalPlanNode::deserializeForNetwork(LogicalPlanNode * node, void * buffer){
	LogicalPlanNodeType type;
	buffer = srch2::util::deserializeFixedTypes(buffer, type);
	switch (type) {
		case LogicalPlanNodeTypeAnd:
		case LogicalPlanNodeTypeOr:
		case LogicalPlanNodeTypeTerm:
		case LogicalPlanNodeTypeNot:
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
	buffer = srch2::util::deserializeFixedTypes(buffer, isExactTermNotNull); // not NULL
	buffer = srch2::util::deserializeFixedTypes(buffer, isFuzzyTermNotNull); // not NULL

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

	if(node->nodeType == LogicalPlanNodeTypePhrase){
		((LogicalPlanPhraseNode *)node)->getPhraseInfo()->deserializeForNetwork(buffer);
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
unsigned LogicalPlanNode::getNumberOfBytesForSerializationForNetwork(){
	//calculate number of bytes
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(nodeType);
	numberOfBytes += sizeof(bool);
	if(this->exactTerm != NULL){
		numberOfBytes += this->exactTerm->getNumberOfBytesForSerializationForNetwork();
	}
	numberOfBytes += sizeof(bool);
	if(this->fuzzyTerm != NULL){
		numberOfBytes += this->fuzzyTerm->getNumberOfBytesForSerializationForNetwork();
	}
	numberOfBytes += sizeof(forcedPhysicalNode);

	if(nodeType == LogicalPlanNodeTypePhrase){
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
	tree = NULL;
	postProcessingInfo = NULL;
	fuzzyQuery = exactQuery = NULL;
	postProcessingPlan = NULL;
}

LogicalPlan::~LogicalPlan(){
	if(tree != NULL) delete tree;
	if(postProcessingInfo != NULL){
		delete postProcessingInfo;
	}
	delete postProcessingPlan;
	delete fuzzyQuery; delete exactQuery;
}

LogicalPlanNode * LogicalPlan::createTermLogicalPlanNode(const std::string &queryKeyword, TermType type,const float boost, const float fuzzyMatchPenalty, const uint8_t threshold , unsigned fieldFilter){
	Term * term = new Term(queryKeyword, type, boost, fuzzyMatchPenalty, threshold);
	term->addAttributeToFilterTermHits(fieldFilter);
	LogicalPlanNode * node = new LogicalPlanNode(term , NULL);
	return node;
}

LogicalPlanNode * LogicalPlan::createOperatorLogicalPlanNode(LogicalPlanNodeType nodeType){
	ASSERT(nodeType != LogicalPlanNodeTypeTerm);
	LogicalPlanNode * node = new LogicalPlanNode(nodeType);
	return node;
}
LogicalPlanNode * LogicalPlan::createPhraseLogicalPlanNode(const vector<string>& phraseKeyWords,
		const vector<unsigned>& phraseKeywordsPosition,
		short slop, unsigned fieldFilter) {

	LogicalPlanNode * node = new LogicalPlanPhraseNode(phraseKeyWords, phraseKeywordsPosition,
			slop, fieldFilter);
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
	if(tree != NULL){
		ss << tree->getSubtreeUniqueString().c_str();
	}
	ss << docIdForRetrieveByIdSearchType;
	if(postProcessingInfo != NULL){
		ss << postProcessingInfo->toString().c_str();
	}
	ss << queryType;
	ss << offset;
	ss << numberOfResultsToRetrieve;
	ss << shouldRunFuzzyQuery;
	if(exactQuery != NULL){
		ss << exactQuery->toString().c_str();
	}
	if(fuzzyQuery != NULL){
		ss << fuzzyQuery->toString().c_str();
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

	buffer = srch2::util::serializeFixedTypes(exactQuery != NULL, buffer); // isNULL
	buffer = srch2::util::serializeFixedTypes(fuzzyQuery != NULL, buffer); // isNULL
	buffer = srch2::util::serializeFixedTypes(postProcessingInfo != NULL,buffer); // isNULL
	//NOTE: postProcessingPlan must be removed completely. It's not used anymore
	buffer = srch2::util::serializeFixedTypes(tree != NULL, buffer); // isNULL

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
	}
	if(isFuzzyQueryNotNull){
		logicalPlan.fuzzyQuery = new Query(SearchTypeTopKQuery);
		buffer = Query::deserializeForNetwork(*logicalPlan.fuzzyQuery, buffer);
	}
	// NOTE: postProcessingPlan is not serialized because it's not used anymore and it must be deleted
	if(isPostProcessingInfoNotNull){
		logicalPlan.postProcessingInfo = new ResultsPostProcessingInfo();
		ResultsPostProcessingInfo::deserializeForNetwork(*logicalPlan.postProcessingInfo, buffer);
	}
	if(isTreeNotNull){
		buffer = LogicalPlanNode::deserializeForNetwork(logicalPlan.tree, buffer);
	}

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
	numberOfBytes += sizeof(this->docIdForRetrieveByIdSearchType);

	numberOfBytes += sizeof(bool)*4; // isNULL
	// exact query
	numberOfBytes += sizeof(bool);
	if(exactQuery != NULL){
		numberOfBytes += exactQuery->getNumberOfBytesForSerializationForNetwork();
	}
	// fuzzy query
	numberOfBytes += sizeof(bool);
	if(fuzzyQuery != NULL){
		numberOfBytes += fuzzyQuery->getNumberOfBytesForSerializationForNetwork();
	}
	//NOTE: postProcessingPlan is not counted because it's not used and it must be deleted
	// postProcessingInfo
	numberOfBytes += sizeof(bool);
	if(postProcessingInfo != NULL){
		numberOfBytes += postProcessingInfo->getNumberOfBytesForSerializationForNetwork();
	}
	//tree
	if(tree != NULL){
		numberOfBytes += tree->getNumberOfBytesForSerializationForNetwork();
	}

	return numberOfBytes;
}

}
}
