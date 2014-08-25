#ifndef __SHARDING_SHARDING_DATA_SHARD_INITIALIZER_H__
#define __SHARDING_SHARDING_DATA_SHARD_INITIALIZER_H__

#include "sharding/configuration/CoreInfo.h"
#include "server/Srch2Server.h"
#include "Shard.h"
#include "core/util/Logger.h"
#include "../ShardManager.h"
#include "Cluster_Writeview.h"

#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class InitialShardHandler{
public:
	InitialShardHandler(ShardId * shardId){
		this->shardId = shardId;
		successFlag = false;
	}
	virtual void prepare() = 0 ;
	virtual ~InitialShardHandler(){};
	bool isSuccessful() const{
		return this->successFlag;
	}
	boost::shared_ptr<Srch2Server> getShardServer() const{
		return server;
	}
protected:
	bool successFlag;
	boost::shared_ptr<Srch2Server> server;
	ShardId * shardId;
};
class InitialShardLoader : public InitialShardHandler{
public:
	InitialShardLoader(ShardId * shardId, const string & indexDirectory):InitialShardHandler(shardId),
	indexDirectory(indexDirectory){};
	void prepare(){

		CoreInfo_t * indexDataConfig = ShardManager::getWriteview()->cores[shardId->coreId];

		server = boost::shared_ptr<Srch2Server>(new Srch2Server(indexDataConfig, indexDirectory, ""));

	    if(indexDataConfig->getDataSourceType() ==
	            srch2::httpwrapper::DATA_SOURCE_MONGO_DB) {
	        // set current time as cut off time for further updates
	        // this is a temporary solution. TODO
	        MongoDataSource::bulkLoadEndTime = time(NULL);
	        //require srch2Server
	        MongoDataSource::spawnUpdateListener(server.get());
	    }
		//load the index from the data source
	    try{
	        server->init();
	    } catch(exception& ex) {
	        /*
	         *  We got some fatal error during server initialization. Print the error
	         *  message and exit the process.
	         *
	         *  Note: Other internal modules should make sure that no recoverable
	         *        exception reaches this point. All exceptions that reach here are
	         *        considered fatal and the server will stop.
	         */
	        srch2::util::Logger::error(ex.what());
	        exit(-1);
	    }
	    this->successFlag = true;
	}
private:
	const string indexDirectory;
};

class InitialShardBuilder : public InitialShardHandler{
public:
	InitialShardBuilder(ShardId * shardId, const string & indexDirectory,
			const string & jsonFileCompletePath):
		InitialShardHandler(shardId),
		indexDirectory(indexDirectory), jsonFileCompletePath(jsonFileCompletePath){};
	void prepare(){

		CoreInfo_t * indexDataConfig = ShardManager::getWriteview()->cores[shardId->coreId];

		server = boost::shared_ptr<Srch2Server>(new Srch2Server(indexDataConfig, indexDirectory, jsonFileCompletePath));

	    if(indexDataConfig->getDataSourceType() ==
	            srch2::httpwrapper::DATA_SOURCE_MONGO_DB) {
	        // set current time as cut off time for further updates
	        // this is a temporary solution. TODO
	        MongoDataSource::bulkLoadEndTime = time(NULL);
	        //require srch2Server
	        MongoDataSource::spawnUpdateListener(server.get());
	    }
		//load the index from the data source
	    try{
	        server->init();
	    } catch(exception& ex) {
	        /*
	         *  We got some fatal error during server initialization. Print the error
	         *  message and exit the process.
	         *
	         *  Note: Other internal modules should make sure that no recoverable
	         *        exception reaches this point. All exceptions that reach here are
	         *        considered fatal and the server will stop.
	         */
	        srch2::util::Logger::error(ex.what());
	        exit(-1);
	    }
	    this->successFlag = true;
	}
	string getJsonFileCompletePath() const{
		return jsonFileCompletePath;
	}
	string getIndexDirectory() const{
		return indexDirectory;
	}
private:
	const string jsonFileCompletePath ;
	const string indexDirectory;
};

class EmptyShardBuilder : public InitialShardHandler{
public:
	EmptyShardBuilder(ShardId * shardId, const string & indexDirectory):
		InitialShardHandler(shardId),
		indexDirectory(indexDirectory){};
	void prepare(){
		CoreInfo_t * indexDataConfig = ShardManager::getWriteview()->cores[shardId->coreId];

		server = boost::shared_ptr<Srch2Server>(new Srch2Server(indexDataConfig, indexDirectory, ""));

	    if(indexDataConfig->getDataSourceType() ==
	            srch2::httpwrapper::DATA_SOURCE_MONGO_DB) {
	        // set current time as cut off time for further updates
	        // this is a temporary solution. TODO
	        MongoDataSource::bulkLoadEndTime = time(NULL);
	        //require srch2Server
	        MongoDataSource::spawnUpdateListener(server.get());
	    }
		//load the index from the data source
	    try{
	        server->init();
	    } catch(exception& ex) {
	        /*
	         *  We got some fatal error during server initialization. Print the error
	         *  message and exit the process.
	         *
	         *  Note: Other internal modules should make sure that no recoverable
	         *        exception reaches this point. All exceptions that reach here are
	         *        considered fatal and the server will stop.
	         */
	        srch2::util::Logger::error(ex.what());
	        exit(-1);
	    }
	    this->successFlag = true;
	}
	string getIndexDirectory() const{
		return indexDirectory;
	}
private:
	const string indexDirectory;
};

}
}

#endif // __SHARDING_SHARDING_DATA_SHARD_INITIALIZER_H__