//$Id: IndexWriteUtil.h 3456 2013-06-14 02:11:13Z jiaying $

#ifndef _INDEXWRITEUTIL_H_
#define _INDEXWRITEUTIL_H_

#include "json/json.h"
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
    static void _insertCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const Json::Value &root, Record *record, std::stringstream &log_str)
    {
    	Schema * storedSchema = Schema::create();
    	RecordSerializerUtil::populateStoredSchema(storedSchema, indexer->getSchema());
    	RecordSerializer recSerializer = RecordSerializer(*storedSchema);
    	Json::FastWriter writer;
    	if(JSONRecordParser::_JSONValueObjectToRecord(record, writer.write(root), root, indexDataContainerConf, log_str, recSerializer) == false){
    		log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"failed\"}";
    		return;
    	}
    	//add the record to the index

    	if ( indexer->getNumberOfDocumentsInIndex() < indexDataContainerConf->getDocumentLimit() )
    	{
            srch2::instantsearch::Analyzer * analyzer = AnalyzerFactory::createAnalyzer(indexDataContainerConf);
    		srch2::instantsearch::INDEXWRITE_RETVAL ret = indexer->addRecord(record, analyzer);
            delete analyzer;

    		switch( ret )
			{
				case srch2::instantsearch::OP_SUCCESS:
				{
					log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"success\"}";
					break;
				}
				case srch2::instantsearch::OP_FAIL:
				{
					log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"The record with same primary key already exists\"}";
					break;
				}
			};
    	}
    	else
    	{
    		log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"document limit reached. Email support@srch2.com for account upgrade.\"}";
    	}

    	//std::cout << "INSERT request received. New number of documents = " << indexer->getNumberOfDocumentsInIndex() << "; Limit = " << indexDataContainerConf->getDocumentLimit() << "." << std::endl;
    }

    static void _deleteCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const Json::Value &root, std::stringstream &log_str)
    {
    	//set the primary key of the record we want to delete
    	std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();
    	const std::string primaryKeyStringValue = root.get(primaryKeyName, "NULL" ).asString();

    	log_str << "{\"rid\":\"" << primaryKeyStringValue << "\",\"delete\":\"";

    	if (primaryKeyStringValue.compare("NULL") != 0)
    	{
    		//delete the record from the index

    		switch(indexer->deleteRecord(primaryKeyStringValue))
    		{
				case OP_FAIL:
    		    {
    		    	log_str << "failed\",\"reason\":\"no record with given primary key\"}";
    		    	break;
    		    }
    		    default: // OP_SUCCESS.
    		    {
    		    	log_str << "success\"}";
    		    }
			};
    	}
    	else
    	{
    		log_str << "failed\",\"reason\":\"no record with given primary key\"}";
    	}

    }

    static void _deleteCommand_QueryURI(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const evkeyvalq &headers, std::stringstream &log_str)
	{
		//set the primary key of the record we want to delete
    	std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();

    	const char *pKeyParamName = evhttp_find_header(&headers, primaryKeyName.c_str());

		if (pKeyParamName)
		{
			size_t sz;
			char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);

			//std::cout << "[" << termBoostsParamName_cstar << "]" << std::endl;
			const std::string primaryKeyStringValue = string(pKeyParamName_cstar);
			delete pKeyParamName_cstar;

			log_str << "{\"rid\":\"" << primaryKeyStringValue << "\",\"delete\":\"";

			//delete the record from the index
			switch(indexer->deleteRecord(primaryKeyStringValue))
			{
				case OP_FAIL:
				{
					log_str << "failed\",\"reason\":\"no record with given primary key\"}";
					break;
				}
				default: // OP_SUCCESS.
				{
					log_str << "success\"}";
				}
			};
		}
		else
		{
            log_str << "{\"rid\":\"NULL\",\"delete\":\"failed\",\"reason\":\"no record with given primary key\"}";
		}
	}

    static void _updateCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const evkeyvalq &headers, const Json::Value &root, Record *record, std::stringstream &log_str)
    {
        /// step 1, delete old record

		//set the primary key of the record we want to delete
    	std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();

        unsigned deletedInternalRecordId;
        std::string primaryKeyStringValue;

        Schema * storedSchema = Schema::create();
        RecordSerializerUtil::populateStoredSchema(storedSchema, indexer->getSchema());
        RecordSerializer recSerializer = RecordSerializer(*storedSchema);
    	Json::FastWriter writer;
    	bool parseJson = JSONRecordParser::_JSONValueObjectToRecord(record, writer.write(root), root,
    			indexDataContainerConf, log_str, recSerializer);
        if(parseJson == false) {
            log_str << "failed\",\"reason\":\"parse: The record is not in a correct json format\",";
            return;
        }

    	primaryKeyStringValue = record->getPrimaryKey();
		log_str << "{\"rid\":\"" << primaryKeyStringValue << "\",\"update\":\"";

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
            Analyzer* analyzer = AnalyzerFactory::getCurrentThreadAnalyzer(indexDataContainerConf);
    		srch2::instantsearch::INDEXWRITE_RETVAL ret = indexer->addRecord(record, analyzer);

    		switch( ret )
			{
				case srch2::instantsearch::OP_SUCCESS:
				{
					if (recordExisted)
					  log_str << "Existing record updated successfully\"}";
					else
					  log_str << "New record inserted successfully\"}";

					return;
				}
				case srch2::instantsearch::OP_FAIL:
				{
					log_str << "failed\",\"reason\":\"insert: The record with same primary key already exists\",";
					break;
				}
			};
    	}
    	else
    	{
    		log_str << "failed\",\"reason\":\"insert: Document limit reached. Email support@srch2.com for account upgrade.\",";
    	}

        /// reaching here means the insert failed, need to resume the deleted old record
        
        srch2::instantsearch::INDEXWRITE_RETVAL ret = indexer->recoverRecord(primaryKeyStringValue, deletedInternalRecordId);

        switch ( ret )
        {
            case srch2::instantsearch::OP_FAIL:
            {
                log_str << "\"resume\":\"no record with given primary key\"}";
                break;
            }
            default: // OP_SUCCESS.
            {
                log_str << "\"resume\":\"success\"}";
            }
        };
    }

    static void _saveCommand(Indexer *indexer, std::stringstream &log_str)
    {
    	indexer->save();
	    log_str << "{\"save\":\"success\"}";
    }

    // save the exported data to exported_data.json
    static void _exportCommand(Indexer *indexer, const char* exportedDataFileName, std::stringstream &log_str)
    {
        indexer->exportData(exportedDataFileName);
        log_str << "{\"export\":\"success\"}";
    }

    static void _commitCommand(Indexer *indexer, const CoreInfo_t *indexDataContainerConf, const uint64_t offset, std::stringstream &log_str)
    {
    	//commit the index.
    	if ( indexer->commit() == srch2::instantsearch::OP_SUCCESS)
    	{
	  
	  // do not save indexes to disk since we can always rebuild them from
	  // indexer->save();
	  log_str << "{\"commit\":\"success\"}";
    	}
    	else
    	{
    		log_str << "{\"commit\":\"failed\"}";
    	}
    }

};

}}

#endif // _INDEXWRITEUTIL_H_
