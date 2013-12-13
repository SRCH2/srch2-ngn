//$Id: Srch2Server.h 3456 2013-06-14 02:11:13Z jiaying $

#ifndef _HTTPSERVERUTIL_H_
#define _HTTPSERVERUTIL_H_

#include <instantsearch/Analyzer.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/GlobalCache.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/QueryResults.h>

#include "ConfigManager.h"
#include "util/mypthread.h"

#include "IndexWriteUtil.h"
#include "json/json.h"
#include "util/Logger.h"
#include "util/FileOps.h"
#include "index/IndexUtil.h"
#include "MongodbAdapter.h"
#include <string>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <fstream>
#include <sstream>

namespace srch2is = srch2::instantsearch;
using std::string;
using srch2is::IndexSearcher;
using srch2is::QueryResults;
using srch2is::Indexer;
using srch2is::GlobalCache;

namespace srch2
{
namespace httpwrapper
{

class Srch2Server
{
public:
	Indexer *indexer;
	const CoreInfo_t *indexDataConfig;

	/* Fields used only for stats */
	time_t stat_starttime;          /* Server start time */
	long long stat_numcommands;     /* Number of processed commands */
	long long stat_numconnections;  /* Number of connections received */
	long long stat_expiredkeys;     /* Number of expired keys */
	long long stat_evictedkeys;     /* Number of evicted keys (maxmemory) */
	long long stat_keyspace_hits;   /* Number of successful lookups of keys */
	long long stat_keyspace_misses; /* Number of failed lookups of keys */
	size_t stat_peak_memory;        /* Max used memory record */
	long long stat_fork_time;       /* Time needed to perform latets fork() */
	long long stat_rejected_conn;   /* Clients rejected because of maxclients */

	Srch2Server()
	{
		this->indexer = NULL;
		this->indexDataConfig = NULL;
	}

	void init(const ConfigManager *config)
	{
	    indexDataConfig = config->getCoreSettings(getCoreName());
	    createAndBootStrapIndexer();
	}


	// Check if index files already exist.
	bool checkIndexExistence(const CoreInfo_t *indexDataConfig)
	{
	    const string &directoryName = indexDataConfig->getIndexPath();
	    if(!checkDirExistence((directoryName + "/" + IndexConfig::analyzerFileName).c_str()))
	        return false;
	    if(!checkDirExistence((directoryName + "/" + IndexConfig::trieFileName).c_str()))
	        return false;
	    if(!checkDirExistence((directoryName + "/" + IndexConfig::forwardIndexFileName).c_str()))
	        return false;
	    if(!checkDirExistence((directoryName + "/" + IndexConfig::schemaFileName).c_str()))
	        return false;
	    if (indexDataConfig->getIndexType() == srch2::instantsearch::DefaultIndex){
	        // Check existence of the inverted index file for basic keyword search ("A1")
	        if(!checkDirExistence((directoryName + "/" + IndexConfig::invertedIndexFileName).c_str()))
	            return false;
	    }else{
	        // Check existence of the quadtree index file for geo keyword search ("M1")
	        if(!checkDirExistence((directoryName + "/" + IndexConfig::quadTreeFileName).c_str()))
	            return false;
	    }
	    return true;
	}

	IndexMetaData *createIndexMetaData(const CoreInfo_t *indexDataConfig)
	{
		//Create a cache
		srch2is::GlobalCache *cache = srch2is::GlobalCache::create(indexDataConfig->getCacheSizeInBytes(), 200000);

		// Create an IndexMetaData
		srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData( cache,
				indexDataConfig->getMergeEveryNSeconds(),
				indexDataConfig->getMergeEveryMWrites(),
				indexDataConfig->getUpdateHistogramEveryPMerges(),
				indexDataConfig->getUpdateHistogramEveryQWrites(),
				indexDataConfig->getIndexPath());

		return indexMetaData;
	}

	void createAndBootStrapIndexer()
	{
		// create IndexMetaData
		IndexMetaData *indexMetaData = createIndexMetaData(this->indexDataConfig);
		IndexCreateOrLoad indexCreateOrLoad;
		if(checkIndexExistence(indexDataConfig))
		    indexCreateOrLoad = srch2http::INDEXLOAD;
		else
		    indexCreateOrLoad = srch2http::INDEXCREATE;

		switch (indexCreateOrLoad)
		{
			case srch2http::INDEXCREATE:
			{
				AnalyzerHelper::initializeAnalyzerResource(this->indexDataConfig);
				// Create a schema to the data source definition in the Srch2ServerConf
				srch2is::Schema *schema = JSONRecordParser::createAndPopulateSchema(indexDataConfig);
				Analyzer *analyzer = AnalyzerFactory::createAnalyzer(this->indexDataConfig);
				indexer = Indexer::create(indexMetaData, analyzer, schema);
				delete analyzer;
				delete schema;
				switch(indexDataConfig->getDataSourceType())
				{
					case srch2http::DATA_SOURCE_JSON_FILE:
					{
						// Create from JSON and save to index-dir
						Logger::console("Creating indexes from JSON file...");
						unsigned indexedCounter = DaemonDataSource::createNewIndexFromFile(indexer, indexDataConfig);
						/*
						 *  commit the indexes once bulk load is done and then save it to the disk only
						 *  if number of indexed record is > 0.
						 */
					    indexer->commit();
					    if (indexedCounter > 0) {
					    	indexer->save();
					    	Logger::console("Indexes saved.");
					    }
						break;
					}
					case srch2http::DATA_SOURCE_MONGO_DB:
					{
						Logger::console("Creating indexes from a MongoDb instance...");
						unsigned indexedCounter = MongoDataSource::createNewIndexes(indexer, indexDataConfig);
				        indexer->commit();
				        if (indexedCounter > 0) {
				            indexer->save();
				            Logger::console("Indexes saved.");
				        }
						break;
					}
					default:
					{
	                    Logger::console("Creating new empty index");
					}
				};
				AnalyzerHelper::saveAnalyzerResource(this->indexDataConfig);
				break;
			}
			case srch2http::INDEXLOAD:
			{
				// Load from index-dir directly, skip creating an index initially.
				indexer = Indexer::load(indexMetaData);
				// Load Analayzer data from disk
				AnalyzerHelper::loadAnalyzerResource(this->indexDataConfig);
				indexer->getSchema()->setSupportSwapInEditDistance(indexDataConfig->getSupportSwapInEditDistance());
				bool isAttributeBasedSearch = false;
				if (indexer->getSchema()->getPositionIndexType() == srch2::instantsearch::POSITION_INDEX_FIELDBIT ||
				    indexer->getSchema()->getPositionIndexType() == srch2::instantsearch::POSITION_INDEX_FULL) {
					isAttributeBasedSearch =true;
				}
				if(isAttributeBasedSearch != indexDataConfig->getSupportAttributeBasedSearch())
				{
					cout << "[Warning] support-attribute-based-search changed in config file, remove all index files and run it again!"<< endl;
				}
				break;
			}
		}
	    // start merger thread
		indexer->createAndStartMergeThreadLoop();
	}

	void setCoreName(const string &name);
	const string &getCoreName();

	virtual ~Srch2Server(){}

 protected:
	string coreName;
};

class HTTPServerEndpoints
{
public:
	static const char *index_search_url;
	static const char *cache_clear_url;
	static const char *index_info_url;
	static const char *index_insert_url;
	static const char *index_delete_url;
	static const char *index_save_url;
	static const char *index_merge_url;
	static const char *index_stop_url;
	static const char *ajax_reply_start;
	//	static const char *ajax_search_pass; // CHENLI
	static const char *ajax_search_fail;
	static const char *ajax_insert_pass;
	static const char *ajax_insert_fail;
	static const char *ajax_insert_fail_403;
	static const char *ajax_insert_fail_500;
	static const char *ajax_save_pass;
	static const char *ajax_merge_pass;
	static const char *ajax_delete_pass;
	static const char *ajax_delete_fail;
	static const char *ajax_delete_fail_500;

	//TODO: NO way to tell if save failed on srch2 index
	static const char *ajax_save_fail;

};

}
}

#endif // _HTTPSERVERUTIL_H_
