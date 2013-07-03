//$Id: BimapleServer.h 3433 2013-06-11 03:13:10Z jiaying $

#ifndef _HTTPSERVERUTIL_H_
#define _HTTPSERVERUTIL_H_

#include <instantsearch/Analyzer.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/GlobalCache.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/QueryResults.h>

#include "BimapleServerConf.h"
#include "BimapleServerLogger.h"
#include "BimapleKafkaConsumer.h"

#include <pthread.h>

namespace bmis = bimaple::instantsearch;
using std::string;
using bmis::IndexSearcher;
using bmis::QueryResults;
using bmis::Indexer;
using bmis::GlobalCache;

namespace bimaple
{
namespace httpwrapper
{


class BimapleServer
{
public:
	Indexer *indexer;
    const BimapleServerLogger *bimapleServerLogger;
	const BimapleServerConf *indexDataContainerConf;
	BimapleKafkaConsumer *kafkaConsumer;

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

	BimapleServer()
	{
		this->indexer = NULL;
		this->bimapleServerLogger= NULL;
		this->indexDataContainerConf = NULL;
		this->kafkaConsumer = NULL;
	}

	void init(const BimapleServerConf *indexDataContainerConf, const BimapleServerLogger *bimapleServerLogger)
	{
        this->bimapleServerLogger = bimapleServerLogger;
		this->indexDataContainerConf = indexDataContainerConf;
		this->kafkaConsumer = new BimapleKafkaConsumer(this->indexDataContainerConf, this->bimapleServerLogger);
		this->indexer = this->kafkaConsumer->getIndexer();
	}

	virtual ~BimapleServer(){}

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

	//TODO: NO way to tell if save failed on bimaple index
	static const char *ajax_save_fail;

};

}
}

#endif // _HTTPSERVERUTIL_H_
