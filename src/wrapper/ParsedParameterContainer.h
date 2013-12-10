//$Id: ResultsPostProcessor.h 3456 2013-06-26 02:11:13Z Jamshid $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#ifndef __WRAPPER_PARSEDPARAMETERCONTAINER_H_
#define __WRAPPER_PARSEDPARAMETERCONTAINER_H_

#include <vector>
#include <string>
#include <map>
#include <algorithm>

#include "WrapperConstants.h"
#include "FilterQueryEvaluator.h"
#include "SortFilterEvaluator.h"
#include "instantsearch/LogicalPlan.h"
#include "instantsearch/ResultsPostProcessor.h"


namespace srch2 {
namespace httpwrapper {

class TermIntermediateStructure{
public:
	TermIntermediateStructure(){
		fieldFilterNumber = 0;
	}
	string termQueryString;
	string rawQueryKeyword;
	float keywordSimilarityThreshold;
	int keywordBoostLevel;
	srch2::instantsearch::TermType keywordPrefixComplete;
	vector<string> fieldFilter;
	BooleanOperation fieldFilterOp;
	bool isPhraseKeywordFlag ;
	short phraseSlop;
	unsigned fieldFilterNumber;

	void print(){
		Logger::console("Term : (%s %f %d %d) ",rawQueryKeyword.c_str(),keywordSimilarityThreshold,keywordBoostLevel,keywordPrefixComplete);
	}
};

class ParseTreeNode{
public:
	LogicalPlanNodeType type;
	ParseTreeNode * parent;
	vector<ParseTreeNode *> children;
	TermIntermediateStructure * temporaryTerm;

//	static int objectCount;
	ParseTreeNode(	LogicalPlanNodeType type,	ParseTreeNode * parent){
	 this->type = type;
	 this->parent = parent;
	 this->temporaryTerm = NULL;
//	 objectCount++;
	}
	~ParseTreeNode(){
		for(vector<ParseTreeNode *>::iterator child = children.begin() ; child != children.end() ; ++child){
			if(*child != NULL) delete *child;
		}

		if(temporaryTerm != NULL){
			delete temporaryTerm;
		}
//		objectCount--;
	}

	string indentation(unsigned indent){
		string result = "";
		for(unsigned i=0;i<indent;i++){
			result += "\t";
		}
		return result;
	}

	void print(unsigned indent = 0){
		switch (type) {
			case LogicalPlanNodeTypeAnd:
				cout << indentation(indent) << "-- AND" << endl;
				break;
			case LogicalPlanNodeTypeOr:
				cout << indentation(indent) << "-- OR" << endl;
				break;
			case LogicalPlanNodeTypeNot:
				cout << indentation(indent) << "-- NOT" << endl;
				break;
			case LogicalPlanNodeTypeTerm:
				cout << indentation(indent) << "-- TERM" << endl;
				break;
		}
		for(vector<ParseTreeNode *>::iterator child = children.begin() ; child != children.end() ; ++child){
			(*child)->print(indent+1);
		}
		cout << indentation(indent) << "--" << endl;
	}
	bool checkValiditiyOfPointers(){
		for(vector<ParseTreeNode *>::iterator child = children.begin() ; child != children.end() ; ++child){
			if ((*child)->parent != this) return false;
		}
		return true;
	}

};


class ParseTreeLeadNodeIterator{
public:
	ParseTreeNode * root;
	ParseTreeNode * currentLeafNode;
	vector<vector<ParseTreeNode *> > dfsChildrenStack;
	ParseTreeLeadNodeIterator(ParseTreeNode * root){
		this->currentLeafNode = NULL;
		init(root);
	}
	~ParseTreeLeadNodeIterator(){}

	bool hasMore(){
		if(currentLeafNode == NULL){
			return false;
		}
		return true;
	}

	/*
	 * Initializing the iteration
	 */
	void init(ParseTreeNode * root){
		this->root = root;
		// Clear all structures
		this->currentLeafNode = NULL;
		dfsChildrenStack.clear();
		/*
		 * Starting from root, we traverse down and keep the children in the stack
		 */
		if(this->root == NULL){
			return;
		}
		// find the first leaf node to be returned by next call of getNext()
		ParseTreeNode * currentNode = this->root;
		while(true){
			if(currentNode->type == LogicalPlanNodeTypeTerm){
				break;
			}
			dfsChildrenStack.push_back(currentNode->children);
			// each node must either be a TERM or have some children
			//ASSERT(dfsChildrenStack.at(dfsChildrenStack.size()-1).size() > 0);
			// set the currentNode to the first child and remove it from the children vector in stack
			currentNode = dfsChildrenStack.at(dfsChildrenStack.size()-1).at(0);
			removeFirstChildOfTopVectorInStack();
		}

		this->currentLeafNode = currentNode;
	}

	void removeFirstChildOfTopVectorInStack(){
		dfsChildrenStack.at(dfsChildrenStack.size()-1).erase(
				dfsChildrenStack.at(dfsChildrenStack.size()-1).begin(),
				dfsChildrenStack.at(dfsChildrenStack.size()-1).begin()+1);
	}

	/*
	 * Going on the next leaf node
	 */
	ParseTreeNode * getNext(){
		/*
		 * 0. save the node to which currentNode is pointing
		 * 1. if top children vector is not empty : check the next node in the top vector of dfsChildrenStack
		 * 2.1. if this node is a TERM node, set it to next to node to be returned, remove it from vector and return node saved in 0
		 * 2.2. else, continue pushing children rightmost children until we reach the
		 * ---- leaf level again and go to 1 again.
		 * 3. if children vector is empty, we pop the stack and go to 1. if stack is also empty set current node to NULL
		 * 3. return the saved node in 0
		 */

		//0. save the node to which currentNode is pointing
		ParseTreeNode * nextLeafNode = this->currentLeafNode;
		//1. if top children vector is not empty : check the next node in the top vector of dfsChildrenStack
		while(true){
			if(dfsChildrenStack.size() == 0){ // stack is empty, leaf nodes are finished.
				this->currentLeafNode = NULL;
				return nextLeafNode;
			}
			vector<ParseTreeNode *> & topChildrenVector = dfsChildrenStack.at(dfsChildrenStack.size()-1);
			if(topChildrenVector.size() != 0){ // 1. if top children vector is not empty : check the next node in the top vector of dfsChildrenStack
				if(topChildrenVector.at(0)->type == LogicalPlanNodeTypeTerm){
					// 2.1. if this node is a TERM node, set it to next to node to be returned, remove it from vector and return node saved in 0
					this->currentLeafNode = topChildrenVector.at(0);
					removeFirstChildOfTopVectorInStack();
					return nextLeafNode;
				}else{
					// erase this node from children vector and keep pushing left most children to the stack
					ParseTreeNode * firstChild = topChildrenVector.at(0);
					// erase it from children vector
					removeFirstChildOfTopVectorInStack();
					// keep pushing left most children until first child is TERM
					while(true){
						// push children of this node
						dfsChildrenStack.push_back(firstChild->children);
						// this node shouldn't be terminal so it's children vector shouldn't be empty
						//ASSERT(dfsChildrenStack.at(dfsChildrenStack.size()-1).size() > 0);
						// get the first member of the new pushed vector (firstChild of the old firstChild)
						firstChild = dfsChildrenStack.at(dfsChildrenStack.size()-1).at(0);
						// remove the first child from top vector
						removeFirstChildOfTopVectorInStack();
						// if it's terminal we have found the next leaf node
						if(firstChild->type == LogicalPlanNodeTypeTerm){
							this->currentLeafNode = firstChild;
							return nextLeafNode;
						} // else : we should keep pushing children
					}
				}

			}else{ // 3. if children vector is empty, we pop the stack and go to 1. if stack is also empty set current node to NULL
				dfsChildrenStack.pop_back();
			}
		}

		ASSERT(false);
		return NULL;
	}



};


class FilterQueryContainer {
public:
    FilterQueryContainer() {
        evaluator = NULL;
    }
    ~FilterQueryContainer() {
        // do not free evaluator here, it's freed in filter
    }
    // this object is created in planGenerator but freed when the filter is being destroyed.
    FilterQueryEvaluator * evaluator;
    void setMessageContainer(std::vector<ParameterName> *parametersInQuery){
        this->parametersInQuery = parametersInQuery;
    }
    std::vector<ParameterName> *parametersInQuery;
};


/*
 * TODO : Extend this design to have multiple sort orders
 * for multiple sort fields ...
 */
class SortQueryContainer
{

public:
    SortQueryContainer() {
        evaluator = NULL;
    }
    ~SortQueryContainer() {
        // do not free evaluator here, it's freed in filter
    }
    // this object is created in planGenerator but freed when the filter is being destroyed.
    SortFilterEvaluator * evaluator;
};

class TopKParameterContainer {
public:
	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> parametersInQuery;

	//
	bool hasParameterInQuery(ParameterName param){
	    return (std::find(parametersInQuery.begin() ,parametersInQuery.end() , param) != parametersInQuery.end());
	}
};

class GetAllResultsParameterContainer {
public:

	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> parametersInQuery;

	bool hasParameterInQuery(ParameterName param){
		return (std::find(parametersInQuery.begin() ,parametersInQuery.end() , param) != parametersInQuery.end());
	}

};

class GeoParameterContainer {
public:
	// while we are parsing we populate this vector by the names of those members
	// which are set. It's a summary of the query parameters.
	std::vector<ParameterName> parametersInQuery;


	// geo related parameters
	float leftBottomLatitude, leftBottomLongitude, rightTopLatitude, rightTopLongitude;
	float centerLatitude,centerLongitude,radius;

	bool hasParameterInQuery(ParameterName param){
		return (std::find(parametersInQuery.begin() ,parametersInQuery.end() , param) != parametersInQuery.end());
	}
};

class ParsedParameterContainer {
public:

    ParsedParameterContainer() {
        filterQueryContainer = NULL;
    	facetQueryContainer = NULL;
    	sortQueryContainer = NULL;
        onlyFacets = false;
        isFuzzy=true;
        prefixMatchPenalty=0;
        isOmitHeader=false;
        maxTimeAllowed=0;
        lengthBoost=0;
        responseResultsFormat=JSON;
        isDebugEnabled=false;
        termFQBooleanOperator=OP_NOT_SPECIFIED;
        termBooleanOperator = OP_NOT_SPECIFIED;
        queryDebugLevel= CompleteDebug;
        isTermBooleanOperatorSet=false;
        isFqBooleanOperatorSet=false;
        resultsStartOffset=0; // defaults to 0
        numberOfResults=10; // defaults to 10
        parseTreeRoot = NULL;
    }

    ~ParsedParameterContainer() {
        if (filterQueryContainer != NULL)
            delete filterQueryContainer;
        // facet container is deleted by ResutlsPostProcessingInfo
        if (sortQueryContainer != NULL)
            delete sortQueryContainer;
        if(parseTreeRoot != NULL){
        	delete parseTreeRoot;
        }
    }
    // while we are parsing we populate this vector by the names of those members
    // which are set. It's a summary of the query parameters.
    std::vector<ParameterName> parametersInQuery;

    std::string docIdForRetrieveByIdSearchType; // if docid parameter is given in the query, this member keeps the value of primary_key to be used later to retrieve the record

    bool onlyFacets; //This flag specifies whether the engine only returns the facets (without matching records) in the response.
    bool isFuzzy; // stores the value of query parameter 'fuzzy'. if fuzzy == True, use keyword's SimilarityThreshold as specified with keywords. else set fuzzy level to 0
    float lengthBoost; // store the value of lengthboost query parameter
    float prefixMatchPenalty; // stores the value of 'pmp' query parameter.

    // This object contains the boolean structure of terms. For example for query
    // q= (A AND B)OR(C AND D)
    // it contains a tree like this:
    // OR ---> AND ---> {A}
    // |        |-----> {B}
    // |
    // |-----> AND ---> {C}
    //          |-----> {D}
    //
    //
    ParseTreeNode * parseTreeRoot;

    // debug query parser parameters
    bool isDebugEnabled;
    QueryDebugLevel queryDebugLevel; // variable to store the debug level. see enum QueryDebugLevel for details.

    // field list parser
    std::vector<std::string> responseAttributesList; // if not empty, response should have only these fields. else check config file.

    // start offset parser parameters
    unsigned resultsStartOffset; // start offset in the response vector. useful in pagination

    // number of results parser
    unsigned numberOfResults; // number of records to return in the response. usefull in pagination

    // time allowed parser parameters
    unsigned maxTimeAllowed; // zero means no time restriction, max engine time allowed.

    // omit header parser parameters
    bool isOmitHeader; // true-> remove the header from the response and return only the records

    // reponse write type parameters
    ResponseResultsFormat responseResultsFormat; // JSON/XML etc

    // filter query parser parameters
    FilterQueryContainer * filterQueryContainer; // contains all filter query related info.

	// facet parser parameters
	FacetQueryContainer * facetQueryContainer;
	// sort parser parameters
	SortQueryContainer * sortQueryContainer;

    // different search type specific parameters
    TopKParameterContainer * topKParameterContainer; // contains all Top k only parameters. currently none.
    GetAllResultsParameterContainer * getAllResultsParameterContainer; //getAllResults specefic params. currently none.
    // both of the above members are just place holders. They are actually used now.
    GeoParameterContainer * geoParameterContainer; // Geo specific params

    /// messages for the query processing pipeline
    // msgString to be added to a vector
    std::vector<std::pair<MessageType, string> > messages; // stores the messages related to warnings and errors.

    /*
     * function create and return the message string.
     */
    std::string getMessageString() {
        std::string result = "";
        for (std::vector<std::pair<MessageType, std::string> >::iterator m =
                messages.begin(); m != messages.end(); ++m) {
            switch (m->first) {
            case MessageError:
                result += "ERROR : " + m->second + "\n";
                break;
            case MessageWarning:
                result += "WARNING : " + m->second + "\n";
                break;
            case MessageNotice:
            	result += "NOTICE : " + m->second + "\n";
            	break;
            }
        }
        return result;
    }

    /*
     * function to check whether the param is set in the summary vector or not.
     */
    bool hasParameterInQuery(ParameterName param) const {
        return (std::find(parametersInQuery.begin(), parametersInQuery.end(), param)
                != parametersInQuery.end());
    }

    // term operator vector
    // TODO:no need of this vector, change it to bool.
    BooleanOperation termBooleanOperator; // boolean operator between different query terms. e.g. field1:keyword1 AND field2:keyword2. AND is a termBoolean Operator
    bool isTermBooleanOperatorSet;
    // FilterQuery term operator vector
    //TODO:no need of this vector, change it to bool.
    BooleanOperation termFQBooleanOperator; // boolean operator between different filterQuery terms. fq=field:[10 TO 20] AND field2:keyword
    // parsed error?
    bool isFqBooleanOperatorSet;

    // the map whose key is analyzed phrase and value is keyword offsets in phrase
    // "into the wild" becomes "into wild" after applying stop word filter.
    // the map stores key = "into wild" and value = "1, 3".
    std::map<string, vector<unsigned> > PhraseKeyWordsPositionMap;
};

}
}

#endif // _WRAPPER_PARSEDPARAMETERCONTAINER_H_
