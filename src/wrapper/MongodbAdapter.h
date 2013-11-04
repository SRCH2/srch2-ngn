//$Id$
/*
 * MongodbAdapter.h
 *
 *  Created on: Sep 3, 2013
 *      Author: sbisht
 */

#ifndef __WRAPPER_MONGODBADAPTER_H__
#define __WRAPPER_MONGODBADAPTER_H__

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
    static unsigned createNewIndexes(srch2is::Indexer* indexer, const ConfigManager *configManager);
    static void spawnUpdateListener( Srch2Server * server);
    static time_t bulkLoadEndTime;
private:
    static void parseOpLogObject(mongo::BSONObj& bobj,string , Srch2Server * server, mongo::DBClientBase& connection);
    static void * runUpdateListener(void *cm);
    static pthread_t * mongoListenerThread;
};

class BSONParser {
public:
    static bool parse(srch2is::Record * record, const mongo::BSONObj& bsonObj, const ConfigManager *configManager);
};

}
}

#endif /* __WRAPPER_MONGODBADAPTER_H__ */
