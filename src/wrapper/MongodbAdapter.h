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

namespace srch2is = srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class MongoDataSource {
public:
    static void createNewIndexes(srch2is::Indexer* indexer, const ConfigManager *configManager);
    static void spawnUpdateListener(const ConfigManager *configManager);
};

class BSONParser {
public:
	static bool parse(srch2is::Record * record, const mongo::BSONObj& bsonObj, const ConfigManager *configManager);
};

}
}

#endif /* MONGODBADAPTER_H_ */
