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

#ifndef __INCLUDE_INSTANTSEARCH__WRAPPERCONSTANTS_H__
#define __INCLUDE_INSTANTSEARCH__WRAPPERCONSTANTS_H__

#include <instantsearch/Constants.h>

namespace srch2 {
namespace httpwrapper {

/// Filter query constants

/// Parse related constants

typedef enum {

    RawQueryKeywords,
    IsFuzzyFlag,
    LengthBoostFlag,
    PrefixMatchPenaltyFlag,
    KeywordSimilarityThreshold,
    KeywordBoostLevel,
    FieldFilter,
    QueryPrefixCompleteFlag,
    IsPhraseKeyword,
    IsDebugEnabled,
    ReponseAttributesList,
    ResultsStartOffset,
    NumberOfResults,
    MaxTimeAllowed,
    IsOmitHeader,
    ResponseFormat,
    GeoSearchFlag,
    FilterQueryEvaluatorFlag,
    RetrieveByIdSearchType,
    TopKSearchType,
    GetAllResultsSearchType,
    // values related to search type specific parameters
    FacetQueryHandler,
    SortQueryHandler,
    GeoTypeRectangular,
    GeoTypeCircular,
    QueryFieldBoostFlag
} ParameterName;

typedef enum {
    TimingDebug, QueryDebug, ResultsDebug, CompleteDebug
} QueryDebugLevel;

typedef enum {
    JSON
} ResponseResultsFormat;

typedef enum {
    MessageError, MessageWarning, MessageNotice
} MessageType;

/// Configuration related constants

typedef enum {
	HTTPWRITEAPI
} WriteApiType;

typedef enum {
	INDEXCREATE ,
	INDEXLOAD
} IndexCreateOrLoad;

typedef enum {
	DATA_SOURCE_NOT_SPECIFIED,
	DATA_SOURCE_JSON_FILE,
	DATA_SOURCE_DATABASE
} DataSourceType;


typedef enum {
	HTTP_JSON_Parse_Error,
	HTTP_JSON_All_Shards_Down_Error,
	HTTP_JSON_Custom_Error,
	HTTP_JSON_Node_Timeout_Warning,
	HTTP_JSON_PK_Exists_Error,
	HTTP_JSON_Doc_Limit_Reached_Error,
	HTTP_JSON_Existing_Record_Update_Info,
	HTTP_JSON_Update_Failed_Error,
	HTTP_JSON_Recover_Failed_Error,
	HTTP_JSON_Delete_Record_Not_Found_Error,
	HTTP_JSON_ResetLogger_Reopen_Failed_Error,
	HTTP_JSON_PK_NOT_PROVIDED,
	HTTP_JSON_Search_Res_Format_Wrong_Error,
	HTTP_JSON_Merge_Already_Done_Info,
	HTTP_JSON_Commit_Already_Done_Info,
	HTTP_JSON_Cluster_Not_Ready_Error
} HTTPJsonMessageCode;

}

}

namespace srch2http = srch2::httpwrapper;

#endif  // __INCLUDE_INSTANTSEARCH__WRAPPERCONSTANTS_H__
