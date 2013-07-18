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

#include "Srch2ServerConf.h"
#include "Srch2KafkaConsumer.h"

#include <pthread.h>

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
	const Srch2ServerConf *indexDataContainerConf;
	Srch2KafkaConsumer *kafkaConsumer;

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
		this->indexDataContainerConf = NULL;
		this->kafkaConsumer = NULL;
	}

	void init(const Srch2ServerConf *indexDataContainerConf)
	{
		this->indexDataContainerConf = indexDataContainerConf;
		this->kafkaConsumer = new Srch2KafkaConsumer(this->indexDataContainerConf);
		this->indexer = this->kafkaConsumer->getIndexer();
	}

	virtual ~Srch2Server(){}

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
