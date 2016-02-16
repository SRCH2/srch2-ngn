#ifndef __INCLUDE_INSTANTSEARCH__WRAPPERCONSTANTS_H__
#define __INCLUDE_INSTANTSEARCH__WRAPPERCONSTANTS_H__

#include <instantsearch/Constants.h>
#include <boost/assign/list_of.hpp>
#include <map>
#include <string>
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


typedef enum {
	HTTP_JSON_Parse_Error,
	HTTP_JSON_Empty_Body,
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
	HTTP_JSON_Merge_DISABLED,
	HTTP_JSON_Commit_Already_Done_Info,
	HTTP_JSON_Cluster_Not_Ready_Error,
	HTTP_JSON_Merge_Parameter_Not_Recognized,
	HTTP_JSON_Request_Rejected_Due_To_Load_Balancing,
	HTTP_JSON_Request_ACL_Not_Available,
	HTTP_Json_DUP_PRIMARY_KEY,
	HTTP_Json_Partition_Is_Locked,
	HTTP_Json_No_Data_Shard_Available_For_Write,
	HTTP_JSON_Core_Does_Not_Exist,
	HTTP_Json_Role_Id_Does_Not_Exist,
	HTTP_Json_Failed_Due_To_Node_Failure,
	HTTP_JSON_PK_Does_Not_Exist,
	HTTP_Json_Data_File_Does_Not_Exist,
	HTTP_Json_Node_Failure,
	HTTP_Json_Cannot_Acquire_Locks, // OPERATION NOT SUPPORTED TO BE ADDED TODO
	HTTP_Json_Cannot_Save_Data,
	HTTP_Json_NOT_SUPPORTED,
	HTTP_Json_General_Error
} JsonMessageCode;



const std::string getJsonMessageCodeStr(const JsonMessageCode code);

}
}
namespace srch2http = srch2::httpwrapper;

#endif  // __INCLUDE_INSTANTSEARCH__WRAPPERCONSTANTS_H__
