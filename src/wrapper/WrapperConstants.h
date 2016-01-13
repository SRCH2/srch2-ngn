
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
    AccessControl,
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

}

}

namespace srch2http = srch2::httpwrapper;

#endif  // __INCLUDE_INSTANTSEARCH__WRAPPERCONSTANTS_H__
