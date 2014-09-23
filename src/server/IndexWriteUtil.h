//$Id: IndexWriteUtil.h 3456 2013-06-14 02:11:13Z jiaying $

#ifndef _INDEXWRITEUTIL_H_
#define _INDEXWRITEUTIL_H_

#include "json/value.h"
#include "JSONRecordParser.h"
#include "ConfigManager.h"
#include "AnalyzerFactory.h"
#include "evhttp.h"
#include "thirdparty/snappy-1.0.4/snappy.h"
#include "URLParser.h"
#include "util/RecordSerializerUtil.h"
using namespace snappy;

namespace srch2
{
namespace httpwrapper
{

struct IndexWriteUtil
{
    static Json::Value _insertCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const Json::Value &root, Record *record);

     static Json::Value _deleteCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const Json::Value &root);

    static Json::Value _deleteCommand_QueryURI(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const evkeyvalq &headers);

    static Json::Value _updateCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const evkeyvalq &headers, const Json::Value &root, Record *record);

    static Json::Value _saveCommand(Indexer *indexer);

    static Json::Value _exportCommand(Indexer *indexer, const char* exportedDataFileName);

    static Json::Value _commitCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const uint64_t offset);

    static Json::Value _aclRecordModifyRoles(Indexer *indexer, string &primaryKeyID, vector<string> &roleIds, srch2::instantsearch::RecordAclCommandType commandType);

    static Json::Value _aclModifyRecordsOfRole(Indexer *indexer, string &roleId, vector<string> &resourceIds, srch2::instantsearch::RecordAclCommandType commandType);

};

}}

#endif // _INDEXWRITEUTIL_H_
