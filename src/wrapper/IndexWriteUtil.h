//$Id: IndexWriteUtil.h 3083 2012-12-11 19:57:06Z oliverax $

#ifndef _INDEXWRITEUTIL_H_
#define _INDEXWRITEUTIL_H_

#include "json/json.h"
#include "JSONRecordParser.h"
#include "BimapleServerConf.h"

#include "evhttp.h"

namespace bimaple
{
namespace httpwrapper
{

struct IndexWriteUtil
{
    static void _insertCommand(Indexer *indexer, const BimapleServerConf *indexDataContainerConf, const Json::Value &root, const uint64_t offset, Record *record, std::stringstream &log_str)
    {
    	Json::FastWriter writer;
    	JSONRecordParser::_JSONValueObjectToRecord(record, writer.write(root), root, indexDataContainerConf, log_str);
    	//add the record to the index

    	if ( indexer->getNumberOfDocumentsInIndex() < indexDataContainerConf->getDocumentLimit() )
    	{
    		bimaple::instantsearch::INDEXWRITE_RETVAL ret = indexer->addRecord(record, offset);

    		switch( ret )
			{
				case bimaple::instantsearch::OP_SUCCESS:
				{
					log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"success\"}";
					break;
				}
				case bimaple::instantsearch::OP_KEYWORDID_SPACE_PROBLEM:
				{
					log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"The keywordid space problem.\"}";
					break;
				}
				case bimaple::instantsearch::OP_FAIL:
				{
					log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"The record with same primary key already exists\"}";
					break;
				}
			};
    	}
    	else
    	{
    		log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"document limit reached. Email support@bimaple.com for account upgrade.\"}";
    	}

    	//std::cout << "INSERT request received. New number of documents = " << indexer->getNumberOfDocumentsInIndex() << "; Limit = " << indexDataContainerConf->getDocumentLimit() << "." << std::endl;
    }

    //TODO: NO way to tell if delete failed on bimaple index
    static void _deleteCommand(Indexer *indexer, const BimapleServerConf *indexDataContainerConf, const Json::Value &root, const uint64_t offset, std::stringstream &log_str)
    {
    	//set the primary key of the record we want to delete
    	std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();
    	const std::string primaryKeyStringValue = root.get(primaryKeyName, "NULL" ).asString();

    	log_str << "{\"rid\":\"" << primaryKeyStringValue << "\",\"delete\":\"";

    	if (primaryKeyStringValue.compare("NULL") != 0)
    	{
    		//delete the record from the index

    		switch(indexer->deleteRecord(primaryKeyStringValue, 0))
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

    	//std::cout << "DELETE request received. New number of documents = " << indexer->getNumberOfDocumentsInIndex() << "; Limit = " << indexDataContainerConf->getDocumentLimit() << "." << std::endl;
    }

    static void _deleteCommand_QueryURI(Indexer *indexer, const BimapleServerConf *indexDataContainerConf, const evkeyvalq &headers, const uint64_t offset, std::stringstream &log_str)
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
			switch(indexer->deleteRecord(primaryKeyStringValue, 0))
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

    static void _updateCommand(Indexer *indexer, const BimapleServerConf *indexDataContainerConf, const evkeyvalq &headers, const Json::Value &root, const uint64_t offset, Record *record, std::stringstream &log_str)
    {
        /// step 1, delete old record

		//set the primary key of the record we want to delete
    	std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();

    	const char *pKeyParamName = evhttp_find_header(&headers, primaryKeyName.c_str());

        unsigned deletedInternalRecordId;
        std::string primaryKeyStringValue;

    	Json::FastWriter writer;
    	JSONRecordParser::_JSONValueObjectToRecord(record, writer.write(root), root, indexDataContainerConf, log_str);

		if (pKeyParamName)
		{
			size_t sz;
			char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);

			//std::cout << "[" << termBoostsParamName_cstar << "]" << std::endl;
			primaryKeyStringValue = string(pKeyParamName_cstar);
			delete pKeyParamName_cstar;

			log_str << "{\"rid\":\"" << primaryKeyStringValue << "\",\"update\":\"";

            if (record->getPrimaryKey() != primaryKeyStringValue)
            {
                log_str << "failed\",\"reason\":\"new record has a different primary key\"}";
                return;
            }

			//delete the record from the index
			switch(indexer->deleteRecordGetInternalId(primaryKeyStringValue, 0, deletedInternalRecordId))
			{
				case OP_FAIL:
				{
                    // record to update doesn't exit, will insert it
                    break;
				}
				default: // OP_SUCCESS.
				{
					//log_str << "success\"}";
				}
			};
		}
		else
		{
            log_str << "{\"rid\":\"NULL\",\"update\":\"failed\",\"reason\":\"delete: no record with given primary key\"}";
            return;
		}

        /// step 2, insert new record

    	//add the record to the index

    	if ( indexer->getNumberOfDocumentsInIndex() < indexDataContainerConf->getDocumentLimit() )
    	{
    		bimaple::instantsearch::INDEXWRITE_RETVAL ret = indexer->addRecord(record, offset);

    		switch( ret )
			{
				case bimaple::instantsearch::OP_SUCCESS:
				{
					log_str << "success\"}";
					return;
				}
				case bimaple::instantsearch::OP_KEYWORDID_SPACE_PROBLEM:
				{
					log_str << "failed\",\"reason\":\"insert: The keywordid space problem.\",";
					break;
				}
				case bimaple::instantsearch::OP_FAIL:
				{
					log_str << "failed\",\"reason\":\"insert: The record with same primary key already exists\",";
					break;
				}
			};
    	}
    	else
    	{
    		log_str << "failed\",\"reason\":\"insert: Document limit reached. Email support@bimaple.com for account upgrade.\",";
    	}

        /// reaching here means the insert failed, need to resume the deleted old record
        
        bimaple::instantsearch::INDEXWRITE_RETVAL ret = indexer->recoverRecord(primaryKeyStringValue, offset, deletedInternalRecordId);

        switch ( ret )
        {
            case OP_FAIL:
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

    static void _commitCommand(Indexer *indexer, const BimapleServerConf *indexDataContainerConf, const uint64_t offset, std::stringstream &log_str)
    {
    	//commit the index.
    	if ( indexer->commit() == OP_SUCCESS)
    	{
	  
	  // CHENLI: do not save indexes to disk since we can always rebuild them from
	  // the log messages in Kafka
	  // indexer->save();
	  std::cout << "_commitCommand(): Do NOT save indexes to the disk." << std::endl;
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
