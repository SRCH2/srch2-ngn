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

 * Copyright �� 2010 SRCH2 Inc. All rights reserved
 */

#ifndef __WRAPPER_PARSEDPARAMETERCONTAINER_H_
#define __WRAPPER_PARSEDPARAMETERCONTAINER_H_

#include <vector>
#include <string>
#include <map>
#include <algorithm>

#include "WrapperConstants.h"
#include "postprocessing/FilterQueryEvaluator.h"
#include "postprocessing/SortFilterEvaluator.h"
#include "instantsearch/LogicalPlan.h"
#include "instantsearch/ResultsPostProcessor.h"
#include "ParsedParameterContainer.h"


namespace srch2 {
namespace httpwrapper {

/*
 * This class contains the information required to make a Term object coming from the
 * query. This information (such as rawQueryString and keywordSimilarityThreshold) are
 * specified in the query, then saved in this class, and then copied to the constructor of
 * Term.
 */
class TermIntermediateStructure{
public:
	TermIntermediateStructure(){
		keywordBoostLevel = 1;
		keywordSimilarityThreshold = 1;
		keywordPrefixComplete = TERM_TYPE_NOT_SPECIFIED;
		isPhraseKeywordFlag = false;
		fieldFilterAttrOperation = ATTRIBUTES_OP_OR;
	}

	//Copy constructor. Create a new node with same value of Object "input".
	TermIntermediateStructure(TermIntermediateStructure * input) {
		this->termQueryString = input->termQueryString;
		this->rawQueryKeyword = input->rawQueryKeyword;

		this->keywordSimilarityThreshold = input->keywordSimilarityThreshold;
		this->keywordBoostLevel = input->keywordBoostLevel;
		this->keywordPrefixComplete = input->keywordPrefixComplete;

		for (vector<string>::iterator it = input->fieldFilter.begin();
				it != input->fieldFilter.end(); it++) {
			this->fieldFilter.push_back(*it);
		}

		this->fieldFilterOp = input->fieldFilterOp;

		this->isPhraseKeywordFlag = input->isPhraseKeywordFlag;
		this->phraseSlop = input->phraseSlop;

		for (vector<unsigned>::iterator it = input->fieldFilterList.begin();
				it != input->fieldFilterList.end(); it++) {
			this->fieldFilterList.push_back(*it);
		}

		this->fieldFilterAttrOperation = input->fieldFilterAttrOperation;
	}

	// termQueryString contains the keyword and all the modifiers. It's the original
	// string coming from the query. For example, if the query is "foo*~0.5 AND author:bar",
	// termQueryString is foo*~0.5 for the first term and "author:bar" for the second one.
	string termQueryString;
	// rawQueryKeyword is the actual searchable keyword. in the above example, "foo" and
	// "bar" are the values that we keep in rawQueryKeyword.
	string rawQueryKeyword;

	float keywordSimilarityThreshold;
	int keywordBoostLevel;
	srch2::instantsearch::TermType keywordPrefixComplete;

	vector<string> fieldFilter;
	BooleanOperation fieldFilterOp;

	bool isPhraseKeywordFlag ;
	short phraseSlop;

	vector<unsigned> fieldFilterList;
	ATTRIBUTES_OP fieldFilterAttrOperation;
	void print(){
		Logger::console("Term : (%s %f %d %d) ",rawQueryKeyword.c_str(),keywordSimilarityThreshold,keywordBoostLevel,keywordPrefixComplete);
	}
};

/*
 * This class contains the information required for a geo search coming from the
 * query. This information (such as rblat and lblong and ...) are
 * specified in the query, then saved in this class.
 */
class GeoIntermediateStructure{
public:
	GeoIntermediateStructure(float lblat, float lblong, float rtlat, float rtlong){
		this->type = GeoTypeRectangular;
		this->lblat = lblat;
		this->lblong = lblong;
		this->rtlat = rtlat;
		this->rtlong = rtlong;
	}

	GeoIntermediateStructure(float clat, float clong, float radius){
		this->type = GeoTypeCircular;
		this->clat = clat;
		this->clong = clong;
		this->radius = radius;
	}

	ParameterName type; // specified the type of the region of the query. It could be GeoTypeRectangular or GeoTypeCircular
	float lblat;        // latitude of left bottom point of the rectangle for rectangular query region
	float lblong;       // longitude of left bottom point of the rectangle for rectangular query region
	float rtlat;        // latitude of right top point of the rectangle for rectangular query region
	float rtlong;       // longitude of right top point of the rectangle for rectangular query region
	float clat;         // latitude of center of the circle for circular query region
	float clong;        // longitude of center of the circle for circular query region
	float radius;       // radius of the circle for circular query region
};

class ParseTreeNode{
public:
	LogicalPlanNodeType type;
	ParseTreeNode * parent;
	vector<ParseTreeNode *> children;
	TermIntermediateStructure * termIntermediateStructure;
	GeoIntermediateStructure * geoIntermediateStructure;

//	static int objectCount;
	ParseTreeNode(	LogicalPlanNodeType type,	ParseTreeNode * parent){
	 this->type = type;
	 this->parent = parent;
	 this->termIntermediateStructure = NULL;
	 this->geoIntermediateStructure  = NULL;
//	 objectCount++;
	}
	ParseTreeNode(	const ParseTreeNode & right){
	 this->type = right.type;
	 this->parent = NULL;
	 for(unsigned childIdx = 0 ; childIdx < right.children.size(); ++childIdx){
		 ParseTreeNode * childClone = new ParseTreeNode(*(right.children.at(childIdx)));
		 this->children.push_back(childClone);
		 childClone->parent = this;
	 }
	 if(right.termIntermediateStructure == NULL){
		 this->termIntermediateStructure = NULL;
	 }else{
		 this->termIntermediateStructure = new TermIntermediateStructure(*(right.termIntermediateStructure));
	 }

	 if(right.geoIntermediateStructure == NULL){
		 this->geoIntermediateStructure = NULL;
	 }else{
		 this->geoIntermediateStructure = new GeoIntermediateStructure(*(right.geoIntermediateStructure));
	 }
	}
    ~ParseTreeNode(){
		for(vector<ParseTreeNode *>::iterator child = children.begin() ; child != children.end() ; ++child){
			if(*child != NULL) delete *child;
		}

		if(termIntermediateStructure != NULL){
			delete termIntermediateStructure;
		}
		if(geoIntermediateStructure  != NULL){
			delete geoIntermediateStructure;
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
			case LogicalPlanNodeTypePhrase:
				cout << indentation(indent) << "-- PHRASE" << endl;
				break;
			case LogicalPlanNodeTypeGeo:
				cout << indentation(indent) << "-- Geo"  << endl;
				break;
		}
		for(vector<ParseTreeNode *>::iterator child = children.begin() ; child != children.end() ; ++child){
			(*child)->print(indent+1);
		}
		cout << indentation(indent) << "--" << endl;
	}
	bool checkValiditiyOfParentPointers(){
		for(vector<ParseTreeNode *>::iterator child = children.begin() ; child != children.end() ; ++child){
			if ((*child)->parent != this) return false;
		}
		return true;
	}

};


class ParseTreeLeafNodeIterator{
public:
	unsigned leafNodeCursor;
	vector<ParseTreeNode *> leafNodes;
	ParseTreeLeafNodeIterator(ParseTreeNode * root){
		init(root);
	}
	~ParseTreeLeafNodeIterator(){}

	bool hasMore(){
		return (leafNodeCursor < leafNodes.size());
	}

	/*
	 * Initializing the iteration :
	 * Traverses the tree in preorder and saves all leaf nodes for later getNext calls
	 */
	void init(ParseTreeNode * root){
		leafNodes.clear();
		traversePreOrderAndAppendLeafNodesToLeafNodesVector(root);
		leafNodeCursor = 0;
	}

	/*
	 * This function traverses the subtree of 'root' and appends discovered leaf node to
	 * leafNodes vector. It's a recursive function.
	 */
	void traversePreOrderAndAppendLeafNodesToLeafNodesVector(ParseTreeNode * root){
		if(root == NULL){
			return;
		}
		if(root->type == LogicalPlanNodeTypeTerm){
			leafNodes.push_back(root);
			return;
		}
		for(unsigned childOffset = 0 ; childOffset != root->children.size() ; ++childOffset){
			traversePreOrderAndAppendLeafNodesToLeafNodesVector(root->children.at(childOffset));
		}
	}

	/*
	 * Returns the next leaf node
	 */
	ParseTreeNode * getNext(){
		ASSERT(leafNodeCursor < leafNodes.size());
		return leafNodes.at(leafNodeCursor++);
	}
};


class FilterQueryContainer {
public:
    FilterQueryContainer() {
        evaluator = NULL;
        parametersInQuery = NULL;
    }
    ~FilterQueryContainer() {
        // do not free evaluator here, it's freed in filter
    }
    FilterQueryContainer(const FilterQueryContainer & right) {
    	if(right.evaluator != NULL){
			evaluator = new FilterQueryEvaluator(*(right.evaluator));
    	}else{
    		evaluator = NULL;
    	}
    	if(right.parametersInQuery != NULL){
    		parametersInQuery = new std::vector<ParameterName> (*(right.parametersInQuery));
    	}else{
    		parametersInQuery = NULL;
    	}
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
    SortQueryContainer(const SortQueryContainer & right) {
    	if(right.evaluator == NULL){
			evaluator = NULL;
    	}else{
    		evaluator = new SortFilterEvaluator(*(right.evaluator));
    	}
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

	~GeoParameterContainer(){
		parametersInQuery.clear();
	}
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

struct QueryFieldAttributeBoost{
  std::string attribute;
  float boost;
};

struct QueryFieldBoostContainer {
  std::vector<QueryFieldAttributeBoost> boosts;
};

class ParsedParameterContainer {
public:

    ParsedParameterContainer() {
        filterQueryContainer = NULL;
    	facetQueryContainer = NULL;
    	sortQueryContainer = NULL;
    	geoParameterContainer = NULL;
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
        qfContainer= NULL;
        isHighlightOn=true;
        hasRoleCore = false;
        attrAclOn = true;
    }

    ParsedParameterContainer& operator=(const ParsedParameterContainer & right){
    	if(this == &right){
    		return *this;
    	}
    	new (this) ParsedParameterContainer(right);
    	return *this;
    }

    ParsedParameterContainer(const ParsedParameterContainer & right) {
        filterQueryContainer = NULL;
        if(right.filterQueryContainer != NULL){
        	filterQueryContainer = new FilterQueryContainer(*(right.filterQueryContainer));
        }
    	facetQueryContainer = NULL;
        if(right.facetQueryContainer != NULL){
        	facetQueryContainer = new FacetQueryContainer(*(right.facetQueryContainer));
        }
    	sortQueryContainer = NULL;
        if(right.sortQueryContainer != NULL){
        	sortQueryContainer = new SortQueryContainer(*(right.sortQueryContainer));
        }
    	geoParameterContainer = NULL;
        if(right.geoParameterContainer != NULL){
        	geoParameterContainer = new GeoParameterContainer(*(right.geoParameterContainer));
        }
        onlyFacets = right.onlyFacets ;
        isFuzzy= right.isFuzzy;
        prefixMatchPenalty= right.prefixMatchPenalty;
        isOmitHeader=right.isOmitHeader;
        maxTimeAllowed= right.maxTimeAllowed;
        lengthBoost= right.lengthBoost;
        responseResultsFormat= right.responseResultsFormat;
        isDebugEnabled=right.isDebugEnabled;
        termFQBooleanOperator=right.termFQBooleanOperator;
        termBooleanOperator = right.termBooleanOperator;
        docIdForRetrieveByIdSearchType = right.docIdForRetrieveByIdSearchType;
        queryDebugLevel= right.queryDebugLevel;
        isTermBooleanOperatorSet=right.isTermBooleanOperatorSet;
        isFqBooleanOperatorSet=right.isTermBooleanOperatorSet;
        resultsStartOffset=right.resultsStartOffset; // defaults to 0
        numberOfResults=right.numberOfResults; // defaults to 10
        parseTreeRoot = NULL;
        if(right.parseTreeRoot != NULL){
        	parseTreeRoot = new ParseTreeNode(*(right.parseTreeRoot));
        }
        qfContainer= NULL;
        if(right.qfContainer != NULL){
        	qfContainer = new QueryFieldBoostContainer(*(right.qfContainer));
        }
        isHighlightOn=right.isHighlightOn;
        hasRoleCore = right.hasRoleCore;
        attrAclOn = right.attrAclOn;
        responseAttributesList = right.responseAttributesList;
        messages = right.messages;
        PhraseKeyWordsInfoMap = right.PhraseKeyWordsInfoMap;
        parametersInQuery = right.parametersInQuery;
        queryStringWithTermsAndOpsOnly = right.queryStringWithTermsAndOpsOnly;
        roleId = right.roleId;
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
        if(geoParameterContainer != NULL){
        	delete geoParameterContainer;
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
    bool isHighlightOn;
    std::string roleId;  // if acl-id is given in the query, we will return results that has this id in their access list.
    bool hasRoleCore;
    // attrAclOn flag is populated from the search query. If it "false", then it means that we ignore
    // attribute ACL during the search process, but still do attribute ACL when generating the JSON results.
    // If it is "true", then attribute ACL is ON for searching as well as generating the results.
    bool attrAclOn;

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
    // query field boost parser parameters
    QueryFieldBoostContainer * qfContainer; 

    // different search type specific parameters
    TopKParameterContainer * topKParameterContainer; // contains all Top k only parameters. currently none.
    GetAllResultsParameterContainer * getAllResultsParameterContainer; //getAllResults specefic params. currently none.
    // both of the above members are just place holders. They are actually used now.
    GeoParameterContainer * geoParameterContainer; // Geo specific params

    /// messages for the query processing pipeline
    // msgString to be added to a vector
    std::vector<std::pair<MessageType, string> > messages; // stores the messages related to warnings and errors.

    string queryStringWithTermsAndOpsOnly; // used for querying feedback index.

    /*
     * function create and return the message string.
     */
    std::string getMessageString() {
        std::string result = "";
        for (std::vector<std::pair<MessageType, std::string> >::iterator m =
                messages.begin(); m != messages.end(); ++m) {
            switch (m->first) {
            case MessageError:
                result += "\"ERROR \":\"" + m->second + "\"";
                break;
            case MessageWarning:
                result += "\"WARNING \":\"" + m->second + "\"";
                break;
            case MessageNotice:
            	result += "\"NOTICE \":\"" + m->second + "\"";
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
    std::map<string, PhraseInfo> PhraseKeyWordsInfoMap;

};

}
}

#endif // _WRAPPER_PARSEDPARAMETERCONTAINER_H_
