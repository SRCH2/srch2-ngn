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





namespace srch2 {
namespace instantsearch {




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

typedef enum
{
	Count
} FacetedSearchAggregationType;


/// Indexer constants

typedef enum {
    OP_FAIL,
    OP_SUCCESS,
    OP_KEYWORDID_SPACE_PROBLEM
} INDEXWRITE_RETVAL;

/// Query constants
// TODO : change getAllResults in the code to FindAllResults
typedef enum
{
    SearchTypeTopKQuery ,
    SearchTypeGetAllResultsQuery ,
    SearchTypeMapQuery
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
    ATTRIBUTE_TYPE_TIME // Time is kept as a long integer in the core.
         // The meaning of this long integer is the number of seconds past from January 1st, 1970
} FilterType;

/*typedef enum
{
    LUCENESCORE = 0,
    ABSOLUTESCORE = 1
} RecordScoreType;*/


typedef enum
{
    FULLPOSITIONINDEX , // the index of keyword in the record
    FIELDBITINDEX ,// keeps the attribute in which a keyword appears in
    NOPOSITIONINDEX // For stemmer to work, positionIndex must be enabled.
} PositionIndexType;

/// Term constants

typedef enum
{
    TERM_TYPE_PREFIX ,
    TERM_TYPE_COMPLETE ,
    TERM_TYPE_NOT_SPECIFIED
} TermType;

/// response type

typedef enum
{
    RESPONSE_WITH_RECORD,
    RESPONSE_BASIC,
    RESPONSE_WITH_SPECIFIED_ATTRIBUTES
} ResponseType;


}
}

namespace srch2is = srch2::instantsearch;

#endif // __INCLUDE_INSTANTSEARCH__CONSTANTS_H__
