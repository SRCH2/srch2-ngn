#ifndef __SERVER_HTTP_JSON_RESPONSE_CONSTANTS_H_
#define __SERVER_HTTP_JSON_RESPONSE_CONSTANTS_H_

#include "string"
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>
#include "server/util/CustomizableJsonWriter.h"
#include "wrapper/WrapperConstants.h"

using namespace std;

namespace srch2 {
namespace httpwrapper {

static const pair<string, string> global_internal_record("srch2_internal_record_123456789", "record");
static const pair<string, string> global_internal_snippet("srch2_internal_snippet_123456789", "snippet");
static const pair<string, string> internal_data[] = { global_internal_record, global_internal_snippet};

// The below objects are used to format the global_customized_writer.
// This global_customized_writer is created once and then can be used multiple times.
static const vector<pair<string, string> > global_internal_skip_tags(internal_data, internal_data+2);
static const ::CustomizableJsonWriter global_customized_writer (&global_internal_skip_tags);
static Json::Value nullJsonValue(Json::nullValue);

static const char * JSON_MESSAGE = "message";
static const char * JSON_ERROR = "error";
static const char * JSON_WARNING = "warning";
static const char * JSON_DETAILS = "details";
static const char * JSON_LOG= "log";

static const char * HTTP_INVALID_REQUEST_MESSAGE = "The request has an invalid or missing argument. See Srch2 API documentation for details.";

static const char* c_message_code = "message_code";
static const char* c_message = "message";
static const char* c_error = "error";
static const char* c_warning = "warning";
static const char* c_info = "info";
static const char* c_action = "action";
static const char* c_action_insert = "insert";
static const char* c_action_delete = "delete";
static const char* c_action_update = "update";
static const char* c_action_acl_record_add = "acl-record-replace";
static const char* c_action_acl_record_append = "acl-record-append";
static const char* c_action_acl_record_delete = "acl-record-delete";
static const char* c_action_acl_attribute_replace = "acl-attribute-replace";
static const char* c_action_acl_attribute_append = "acl-attribute-append";
static const char* c_action_acl_attribute_delete = "acl-attribute-delete";
static const char* c_action_user_feedback = "user-feedback";
static const char* c_action_save   = "save";
static const char* c_action_save_data   = "save_data";
static const char* c_action_save_metadata   = "save_metadata";
static const char* c_action_export = "export";
static const char* c_action_commit = "commit";
static const char* c_action_merge = "merge";
static const char* c_action_merge_on = "merge_on";
static const char* c_action_merge_off = "merge_off";
static const char* c_action_reset_logger = "reset_logger";
static const char* c_logger_file = "logger_file";

static const char* c_rid = "rid";
static const char* c_reason = "reason";
static const char* c_detail = "details";
static const char* c_resume = "resume";

static const char* c_status = "status";
static const char* c_failed = "failed";
static const char* c_success = "success";

static const char* c_core_data_info = "core-data-info";
static const char* c_cores = "cores";
static const char* c_core_name = "coreName";
static const char* c_core_primary_shards = "primaryShards";
static const char* c_core_replica_shards = "replicas";
static const char* c_core_total_num_docs = "totalNumberOfDocuments";
static const char* c_core_total_num_writes = "totalNumberOfWrites";
static const char* c_core_total_num_reads = "totalNumberOfReads";
static const char* c_core_last_merg_time = "lastMergeTime";
static const char* c_core_cluster_shards = "primary-shards";
static const char* c_core_node_shards = "static-shards";
static const char* c_shard_id = "shardId";
static const char* c_shard_num_docs = "numberOfDocuments";
static const char* c_shard_total_num_writes = "totalNumberOfWrites";
static const char* c_shard_total_num_reads = "totalNumberOfReads";
static const char* c_shard_last_merg_time = "lastMergeTime";
static const char* c_aggregated_stats = "aggregated-stats";
static const char* c_partitions = "partitions";
static const char* c_all_cluster_shards = "all-cluster-shards";
static const char* c_node_shards = "static-shards";
static const char* c_doc_count = "doc-count";
static const char* c_write_count = "write-count";
static const char* c_read_count = "read-count";
static const char* c_last_merge_time = "last-merge-time";
static const char* c_merged_needed = "merge-needed";
static const char* c_commit_done = "commit-done";
static const char* c_nodes = "nodes";
static const char* c_shards_of_node = "shards-of-node";
static const char* c_nodes_shards = "shards";
static const char* c_nodes_count = "count";
static const char* c_node_version = "node-version";
static const char* c_node_location = "node-location";
static const char* c_node_name = "name";
static const char* c_node_name2 = "node-name";
static const char* c_node_listening_host_name = "listeningHostName";
static const char* c_cluster_name = "clusterName";
static const char* c_cluster_total_number_of_documnets = "totalNumberOfDocumentsInCluster";



}
}


#endif // __SERVER_HTTP_JSON_RESPONSE_CONSTANTS_H_
