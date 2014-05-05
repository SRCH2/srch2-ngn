//$Id: Analyzer.h 3456 2013-06-14 02:11:13Z jiaying $

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

#ifndef __INCLUDE_INSTANTSEARCH__CONSTANTS_H__
#define __INCLUDE_INSTANTSEARCH__CONSTANTS_H__


#include "string"


namespace srch2 {
namespace instantsearch {

typedef unsigned CharType;
#define Byte char

const std::string MULTI_VALUED_ATTRIBUTES_VALUE_DELIMITER = ",";

/*
 * Example: tags is a multi valued attribute. Suppose one record has this value for tags :
 * ['C++ Coding Style','Java Concept Encapsulation','Programming Principles']
 * The value of this variable is used when the position of each token is assigned. We add this bump to tokens of each value of a multi-valued attribute
 * so that they are not matched in phrase-search. For example, adding this bump to 'Java' will prevent "Style Java" to match this record.
 */
const unsigned MULTI_VALUED_ATTRIBUTE_POSITION_BUMP = 100000;

static const char * MULTI_VAL_ATTR_DELIMITER = " $$ ";
const unsigned MULTI_VAL_ATTR_DELIMITER_LEN = 4;
/// Analyzer related constants
typedef enum {
    // there is no numbering for this enum. By default the numbers start from 0
    DISABLE_STEMMER_NORMALIZER, // Disables stemming
    ENABLE_STEMMER_NORMALIZER, // Enables stemming
    ONLY_NORMALIZER
} StemmerNormalizerFlagType;

typedef enum {
    DICTIONARY_PORTER_STEMMER
// We can add other kinds of stemmer here, like MIRROR_STEMMER

} StemmerType; // TODO: I should remove the '_' from the name, (it is temporary)

typedef enum {
    SYNONYM_KEEP_ORIGIN, // Disables stemming
    SYNONYM_DONOT_KEEP_ORIGIN   // Enables stemming
} SynonymKeepOriginFlag;


typedef enum {
    STANDARD_ANALYZER,    // StandardAnalyzer
    SIMPLE_ANALYZER,       // SimpleAnalyzer
    CHINESE_ANALYZER    // ChineseAnalyzer
} AnalyzerType;



/// Faceted search filter

typedef enum{
	FacetTypeCategorical,
	FacetTypeRange,
	FacetTypeNonSpecified
} FacetType;



/// Indexer constants

typedef enum {
    OP_FAIL,
    OP_SUCCESS
} INDEXWRITE_RETVAL;

/// Query constants
// TODO : change getAllResults in the code to FindAllResults
typedef enum
{
    SearchTypeTopKQuery ,
    SearchTypeGetAllResultsQuery ,
    SearchTypeMapQuery,
    SearchTypeRetrieveById
} QueryType;

//TODO add prefix OP
typedef enum
{
	LESS_THAN ,
	EQUALS,
	GREATER_THAN,
	LESS_THAN_EQUALS,
	GREATER_THAN_EQUALS,
	NOT_EQUALS

} AttributeCriterionOperation;


typedef enum
{
    SortOrderAscending ,
    SortOrderDescending,
    SortOrderNotSpecified
} SortOrder;

typedef enum{
	BooleanOperatorAND,
	BooleanOperatorOR,
	OP_NOT_SPECIFIED
} BooleanOperation;

///  Ranker constants



/// Record constants

typedef enum {
    LU_PRESENT_IN_READVIEW_AND_WRITEVIEW,
    LU_TO_BE_INSERTED,
    LU_ABSENT_OR_TO_BE_DELETED
} INDEXLOOKUP_RETVAL;

/// Schema constants

typedef enum
{
    DefaultIndex,
    LocationIndex
} IndexType;

// change the names, they are too general
typedef enum
{
    ATTRIBUTE_TYPE_UNSIGNED,
    ATTRIBUTE_TYPE_FLOAT ,
    ATTRIBUTE_TYPE_TEXT ,
    ATTRIBUTE_TYPE_TIME ,// Time is kept as a long integer in the core.
         // The meaning of this long integer is the number of seconds past from January 1st, 1970
    // TypedValue class uses these constants to understand if it is dealing with a single-valued attribute
    // or a multi-valued one.
    ATTRIBUTE_TYPE_MULTI_UNSIGNED,
    ATTRIBUTE_TYPE_MULTI_FLOAT,
    ATTRIBUTE_TYPE_MULTI_TEXT,
    ATTRIBUTE_TYPE_MULTI_TIME,
    ATTRIBUTE_TYPE_DURATION
} FilterType;

/*typedef enum
{
    LUCENESCORE = 0,
    ABSOLUTESCORE = 1
} RecordScoreType;*/


typedef enum
{
    POSITION_INDEX_FULL,
    POSITION_INDEX_WORD , // the word offset of keyword in the record
    POSITION_INDEX_CHAR , // the character offset of keyword in the record
    POSITION_INDEX_FIELDBIT ,// keeps the attribute in which a keyword appears in
    POSITION_INDEX_NONE // For stemmer to work, positionIndex must be enabled.
} PositionIndexType;

bool inline isEnabledAttributeBasedSearch(PositionIndexType positionIndexType) {
	return (positionIndexType == POSITION_INDEX_FIELDBIT
			|| positionIndexType == POSITION_INDEX_WORD
			|| positionIndexType == POSITION_INDEX_CHAR
			|| positionIndexType == POSITION_INDEX_FULL);

}

bool inline isEnabledWordPositionIndex(PositionIndexType positionIndexType) {
	return (positionIndexType == POSITION_INDEX_WORD
			|| positionIndexType == POSITION_INDEX_FULL);
}

bool inline isEnabledCharPositionIndex(PositionIndexType positionIndexType) {
	return (positionIndexType == POSITION_INDEX_CHAR
			|| positionIndexType == POSITION_INDEX_FULL);
}
/// Term constants

typedef enum
{
    TERM_TYPE_PREFIX ,
    TERM_TYPE_COMPLETE ,
    TERM_TYPE_NOT_SPECIFIED,
    TERM_TYPE_PHRASE
} TermType;

///
enum DateTimeType{
    DateTimeTypeNow,
    DateTimeTypePointOfTime,
    DateTimeTypeDurationOfTime
};

/// response type
typedef enum
{
    RESPONSE_WITH_STORED_ATTR,
    RESPONSE_WITH_NO_STORED_ATTR,
    RESPONSE_WITH_SELECTED_ATTR
} ResponseType;

///
typedef enum
{
	FacetAggregationTypeCount
}FacetAggregationType;

typedef enum
{
	HistogramAggregationTypeSummation,
	HistogramAggregationTypeJointProbability
} HistogramAggregationType;

typedef enum {
	LogicalPlanNodeTypeAnd,
	LogicalPlanNodeTypeOr,
	LogicalPlanNodeTypeTerm,
	LogicalPlanNodeTypeNot,
	LogicalPlanNodeTypePhrase
} LogicalPlanNodeType;

typedef enum {
	PhysicalPlanNode_NOT_SPECIFIED,
	PhysicalPlanNode_SortById,
	PhysicalPlanNode_SortByScore,
	PhysicalPlanNode_MergeTopK,
	PhysicalPlanNode_MergeSortedById,
	PhysicalPlanNode_MergeByShortestList,
	PhysicalPlanNode_UnionSortedById,
	PhysicalPlanNode_UnionLowestLevelTermVirtualList,
	PhysicalPlanNode_UnionLowestLevelSimpleScanOperator,
	PhysicalPlanNode_UnionLowestLevelSuggestion,
	PhysicalPlanNode_RandomAccessTerm,
	PhysicalPlanNode_RandomAccessAnd,
	PhysicalPlanNode_RandomAccessOr,
	PhysicalPlanNode_RandomAccessNot,
	PhysicalPlanNode_Facet,
	PhysicalPlanNode_SortByRefiningAttribute,
	PhysicalPlanNode_FilterQuery,
	PhysicalPlanNode_PhraseSearch,
	PhysicalPlanNode_KeywordSearch
} PhysicalPlanNodeType;

typedef enum {
	PhysicalPlanIteratorProperty_SortById,
	PhysicalPlanIteratorProperty_SortByScore,
	// This value should not be included in output properties of any operator. It's a mechanism
	/// to always push down TVL operator and NULL operator.
	PhysicalPlanIteratorProperty_LowestLevel
} PhysicalPlanIteratorProperty;

}
}

namespace srch2is = srch2::instantsearch;

#endif // __INCLUDE_INSTANTSEARCH__CONSTANTS_H__
