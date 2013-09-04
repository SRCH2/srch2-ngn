/*
 * MongodbAdapter.cpp
 *
 *  Created on: Sep 3, 2013
 *      Author: sbisht
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include "mongo/client/dbclient.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/client/dbclientcursor.h"
#include "util/Logger.h"
#include "json/json.h"
#include "MongodbAdapter.h"
#include "AnalyzerFactory.h"
#include "JSONRecordParser.h"
#include "boost/algorithm/string.hpp"

using namespace std;

namespace srch2 {
namespace httpwrapper {

void MongoDataSource::createNewIndexes(srch2is::Indexer* indexer, const ConfigManager *configManager) {

    string dbNameWithCollection = configManager->getMongoDbName() +
    							  "."  + configManager->getMongoCollection();
    string host = configManager->getMongoServerHost();
    string port = configManager->getMongoServerPort();
    boost::algorithm::trim(host);
    try {
        mongo::DBClientConnection c;
        c.connect(host);
        Logger::console("connected to Mongo Db Instance %s-%s", host.c_str(), port.c_str());
        unsigned collectionCount = c.count(dbNameWithCollection);
        if (collectionCount > 0) {
        	srch2is::Record *record = new srch2is::Record(indexer->getSchema());
        	srch2is::Analyzer *analyzer = AnalyzerFactory::createAnalyzer(configManager);
            // Todo: query in batch ..or cursor should batch it automatically ??
            auto_ptr<mongo::DBClientCursor> cursor = c.query(dbNameWithCollection, mongo::BSONObj());
            //unsigned long long maxTS;
            while (cursor->more()) {
                mongo::BSONObj bsonObj = cursor->next();
                //cout << bsonObj.jsonString() << endl;
                mongo::BSONElement bsonElmt;
                bsonObj.getObjectID(bsonElmt);
                unsigned long long ts = bsonElmt.timestampInc();
                //if (maxTS < ts)
                //	maxTS = ts;
                //cout << ts << endl;
                bool result = BSONParser::parse(record, bsonObj, configManager);
                if (result)
                	indexer->addRecord(record, analyzer, 0);
                record->clear();
            }
            Logger::console("Indexed %d records.", collectionCount);
            //cout << "maximum ts = " <<maxTS << endl;

            delete analyzer;
            delete record;

        } else {
        	Logger::info("No data found in the collection %s", dbNameWithCollection.c_str());
        }

        indexer->commit();
        Logger::console("Saving Index.....");
        indexer->save();
        Logger::console("Index saved.");

    } catch( const mongo::DBException &e ) {
    	Logger::console("MongoDb Exception %s", e.what());
    	exit(-1);
    } catch (const exception& ex){
    	Logger::console("Unkown exception %s", ex.what());
    }
}

void MongoDataSource::spawnUpdateListener(const ConfigManager *configManager){
    /*string dbNameWithCollection = configManager->getMongoDbName() +
    							  "."  + configManager->getMongoCollection();
    string host = configManager->getMongoServerHost();
    string port = configManager->getMongoServerPort();*/
}

bool BSONParser::parse(srch2is::Record * record, const mongo::BSONObj& bsonObj, const ConfigManager* configManager){

	stringstream error;
	bool result = JSONRecordParser::populateRecordFromJSON(bsonObj.jsonString(), configManager, record, error);
	if(!result)
		cout << error.str() << endl;
	return result;
}


/*int main() {
    string host = "calvin.calit2.uci.edu";
    string db = "imdb";
    string collection = "movies";
    string db_collection = db + "." + collection;
    MongoDataSource::createNewIndexes(db_collection);
    return EXIT_SUCCESS;
}*/

}
}

