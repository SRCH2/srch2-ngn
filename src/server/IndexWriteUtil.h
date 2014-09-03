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
    static Json::Value _insertCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const Json::Value &root, Record *record)
    {
        Json::Value response(Json::objectValue);
        const char* c_rid = "rid";
        const char* c_action = "insert";
        const char* c_reason = "reason";
        const char* c_detail= "details";

        Schema * storedSchema = Schema::create();
        RecordSerializerUtil::populateStoredSchema(storedSchema, indexer->getSchema());
        RecordSerializer recSerializer = RecordSerializer(*storedSchema);

        std::stringstream errorStream; 
        if(!JSONRecordParser::_JSONValueObjectToRecord(record, root, indexDataContainerConf, errorStream, recSerializer)){
            response[c_rid] = record->getPrimaryKey();
            response[c_action] = "failed";
            response[c_reason] = "parse: The record is not in a correct json format";
            response[c_detail] = errorStream.str();

            delete storedSchema;
            return response;
        }
        //add the record to the index

        if ( indexer->getNumberOfDocumentsInIndex() < indexDataContainerConf->getDocumentLimit() )
        {
            // Do NOT delete analyzer because it is thread specific. It will be reused for
            // search/update/delete operations.
            srch2::instantsearch::Analyzer * analyzer = AnalyzerFactory::getCurrentThreadAnalyzerWithSynonyms(indexDataContainerConf);
            srch2::instantsearch::INDEXWRITE_RETVAL ret = indexer->addRecord(record, analyzer);

            switch( ret )
            {
                case srch2::instantsearch::OP_SUCCESS:
                    {
                        response[c_rid] = record->getPrimaryKey();
                        response[c_action] = "success";
                        break;
                    }
                case srch2::instantsearch::OP_FAIL:
                    {
                        response[c_rid] = record->getPrimaryKey();
                        response[c_action] = "failed";
                        response[c_reason] = "The record with same primary key already exists";
                        break;
                    }
            };
        }
        else
        {
            response[c_rid]= record->getPrimaryKey();
            response[c_action] = "failed";
            response[c_reason] = "document limit reached. Email support@srch2.com for account upgrade.";
        }
        delete storedSchema;
        return response;
    }

    static Json::Value _deleteCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const Json::Value &root)
    {
        Json::Value response(Json::objectValue);
        const char* c_rid = "rid";
        const char* c_action = "delete";
        const char* c_reason = "reason";
        //set the primary key of the record we want to delete
        std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();
        const std::string primaryKeyStringValue = root.get(primaryKeyName, "NULL" ).asString();

        response[c_rid] = primaryKeyStringValue;

        if (primaryKeyStringValue.compare("NULL") != 0)
        {
            //delete the record from the index

            switch(indexer->deleteRecord(primaryKeyStringValue))
            {
                case OP_FAIL:
                    {
                        response[c_action] = "failed";
                        response[c_reason] = "no record with given primary key";
                        break;
                    }
                default: // OP_SUCCESS.
                    {
                        response[c_action] = "success";
                    }
            };
        }
        else
        {
            response[c_action] = "failed";
            response[c_reason] = "no record with given primary key";
        }
        return response;
    }

    static Json::Value _deleteCommand_QueryURI(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const evkeyvalq &headers)
    {
        Json::Value response(Json::objectValue);
        const char* c_rid = "rid";
        const char* c_action = "delete";
        const char* c_reason = "reason";
        //set the primary key of the record we want to delete
        std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();

        const char *pKeyParamName = evhttp_find_header(&headers, primaryKeyName.c_str());

        if (pKeyParamName)
        {
            size_t sz;
            char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);

            //std::cout << "[" << termBoostsParamName_cstar << "]" << std::endl;
            const std::string primaryKeyStringValue = string(pKeyParamName_cstar);
            free(pKeyParamName_cstar);

            response[c_rid] = primaryKeyStringValue;

            //delete the record from the index
            switch(indexer->deleteRecord(primaryKeyStringValue))
            {
                case OP_FAIL:
                    {
                        response[c_action] = "failed";
                        response[c_reason] = "no record with given primary key";
                        break;
                    }
                default: // OP_SUCCESS.
                    {
                        response[c_action] = "success";
                    }
            };
        }
        else
        {
            response[c_rid] = "NULL";
            response[c_action] = "failed";
            response[c_reason] = "no record with given primary key";
        }
        return response;
    }

    static Json::Value _updateCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const evkeyvalq &headers, const Json::Value &root, Record *record)
    {
        Json::Value response(Json::objectValue);
        const char* c_rid = "rid";
        const char* c_action = "update";
        const char* c_reason = "reason";
        const char* c_detail = "details";
        const char* c_resume = "resume";
        /// step 1, delete old record

        //set the primary key of the record we want to delete
        std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();

        unsigned deletedInternalRecordId;
        std::string primaryKeyStringValue;

        Schema * storedSchema = Schema::create();
        RecordSerializerUtil::populateStoredSchema(storedSchema, indexer->getSchema());
        RecordSerializer recSerializer = RecordSerializer(*storedSchema);

        std::stringstream errorStream ;
        bool parseJson = JSONRecordParser::_JSONValueObjectToRecord(record, root,
                indexDataContainerConf, errorStream, recSerializer);
        delete storedSchema;
        if(!parseJson ) {
            response[c_rid] = record->getPrimaryKey() ;
            response[c_action] = "failed";
            response[c_reason] = "parse: The record is not in a correct json format";
            response[c_detail] = errorStream.str();
            return response;
        }

        primaryKeyStringValue = record->getPrimaryKey();
        response[c_rid] =  primaryKeyStringValue; 

        //delete the record from the index
        bool recordExisted = false;
        switch(indexer->deleteRecordGetInternalId(primaryKeyStringValue, deletedInternalRecordId))
        {
            case srch2::instantsearch::OP_FAIL:
                {
                    // record to update doesn't exit, will insert it
                    break;
                }
            default: // OP_SUCCESS.
                {
                    recordExisted = true;
                }
        };


        /// step 2, insert new record

        //add the record to the index

        if ( indexer->getNumberOfDocumentsInIndex() < indexDataContainerConf->getDocumentLimit() )
        {
            // Do NOT delete analyzer because it is thread specific. It will be reused for
            // search/update/delete operations.
            Analyzer* analyzer = AnalyzerFactory::getCurrentThreadAnalyzer(indexDataContainerConf);
            srch2::instantsearch::INDEXWRITE_RETVAL ret = indexer->addRecord(record, analyzer);
            switch( ret )
            {
                case srch2::instantsearch::OP_SUCCESS:
                    {
                        if (recordExisted){
                            response[c_action] = "Existing record updated successfully";
                        } else {
                            response[c_action] = "New record inserted successfully";
                        }
                        return response;
                    }
                case srch2::instantsearch::OP_FAIL:
                    {
                        response[c_action] = "failed";
                        response[c_reason] = "insert: The record with same primary key already exists";
                        break;
                    }
            };
        }
        else
        {
            response[c_action] = "failed";
            response[c_reason] = "insert: Document limit reached. Email support@srch2.com for account upgrade.";
        }

        /// reaching here means the insert failed, need to resume the deleted old record

        srch2::instantsearch::INDEXWRITE_RETVAL ret = indexer->recoverRecord(primaryKeyStringValue, deletedInternalRecordId);

        switch ( ret )
        {
            case srch2::instantsearch::OP_FAIL:
                {
                    response[c_resume] = "no record with given primary key";
                    break;
                }
            default: // OP_SUCCESS.
                {
                    response[c_resume] = "success";
                }
        };
        return response;
    }


    static Json::Value _saveCommand(Indexer *indexer)
    {
        Json::Value response(Json::objectValue);
        indexer->save();
        response["save"] = "success";
        return response;
    }

    // save the exported data to exported_data.json
    static Json::Value _exportCommand(Indexer *indexer, const char* exportedDataFileName)
    {
        Json::Value response(Json::objectValue);
        indexer->exportData(exportedDataFileName);
        response["export"] = "success";
        return response;
    }

    static Json::Value _commitCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const uint64_t offset)
    {
        Json::Value response(Json::objectValue);
        const char* c_action = "commit";
        //commit the index.
        if ( indexer->commit() == srch2::instantsearch::OP_SUCCESS)
        {
            response[c_action] = "success";

            // do not save indexes to disk since we can always rebuild them from
            // indexer->save();
        }
        else
        {
            response[c_action] = "failed";
        }
        return response;
    }

};

}}

#endif // _INDEXWRITEUTIL_H_
