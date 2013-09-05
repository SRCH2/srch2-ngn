/*
 * MongodbAdapter.h
 *
 *  Created on: Sep 3, 2013
 *      Author: sbisht
 */

#ifndef MONGODBADAPTER_H_
#define MONGODBADAPTER_H_

#include <instantsearch/Indexer.h>
#include <instantsearch/Record.h>
#include <instantsearch/Schema.h>
#include "ConfigManager.h"
#include "mongo/client/dbclient.h"
#include "mongo/bson/bsonobj.h"
#include <time.h>

namespace srch2is = srch2::instantsearch;

namespace mongo{
class DBClientConnection;
}

namespace srch2 {
namespace httpwrapper {

class Srch2Server;
class MongoDataSource {
public:
    static void createNewIndexes(srch2is::Indexer* indexer, const ConfigManager *configManager);
    static void spawnUpdateListener( Srch2Server * server);
private:
	static void parseOpLogObject(mongo::BSONObj& bobj,string , Srch2Server * server);
    static void * runUpdateListener(void *cm);
    static pthread_t * mongoListnerThread;
    static time_t maxRecTime;
    static mongo::DBClientConnection * pooledConnection; // This is not actual pooling ..TODO
};

class BSONParser {
public:
	static bool parse(srch2is::Record * record, const mongo::BSONObj& bsonObj, const ConfigManager *configManager);
};

}
}

#endif /* MONGODBADAPTER_H_ */
