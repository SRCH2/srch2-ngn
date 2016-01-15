/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
