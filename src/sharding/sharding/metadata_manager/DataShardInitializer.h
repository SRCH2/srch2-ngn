#ifndef __SHARDING_SHARDING_DATA_SHARD_INITIALIZER_H__
#define __SHARDING_SHARDING_DATA_SHARD_INITIALIZER_H__

#include "Shard.h"
#include "Cluster_Writeview.h"
#include "../ShardManager.h"
#include "../../configuration/CoreInfo.h"
#include "server/Srch2Server.h"
#include "core/util/Logger.h"
#include "core/operation/IndexData.h"//for debug purpose

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
	virtual void prepare(bool shouldLockWriteview = true) = 0 ;
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
	void prepare(bool shouldLockWriteview = true){
		boost::shared_lock<boost::shared_mutex> sLock;
		const CoreInfo_t * indexDataConfig = NULL;
		if(shouldLockWriteview){
			indexDataConfig = ShardManager::getWriteview_read(sLock)->cores.at(shardId->coreId);
		}else{
			indexDataConfig = ShardManager::getWriteview_nolock()->cores.at(shardId->coreId);
		}

		server = boost::shared_ptr<Srch2Server>(new Srch2Server(indexDataConfig, indexDirectory, ""));

		//load the index from the data source
	    try{
	        server->init();
	        if(server->__debugShardingInfo != NULL){
				server->__debugShardingInfo->shardName = shardId->toString();
				stringstream ss;
				ss << "result of shard loader, indexDirectory : " << indexDirectory << " with " <<
            			server->getIndexer()->getNumberOfDocumentsInIndex() << " records.";
				server->__debugShardingInfo->explanation = ss.str();
	        }
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
	void prepare(bool shouldLockWriteview = true){
		boost::shared_lock<boost::shared_mutex> sLock;
		const CoreInfo_t * indexDataConfig = NULL;
		if(shouldLockWriteview){
			indexDataConfig = ShardManager::getWriteview_read(sLock)->cores.at(shardId->coreId);
		}else{
			indexDataConfig = ShardManager::getWriteview_nolock()->cores.at(shardId->coreId);
		}

		server = boost::shared_ptr<Srch2Server>(new Srch2Server(indexDataConfig, indexDirectory, jsonFileCompletePath));

		//load the index from the data source
	    try{
	        server->init();
	        if(server->__debugShardingInfo != NULL){
				server->__debugShardingInfo->shardName = shardId->toString();
				stringstream ss;
				ss << "result of initial shard builder, indexDirectory : " << indexDirectory << " with " <<
            			server->getIndexer()->getNumberOfDocumentsInIndex() << " records.";
				server->__debugShardingInfo->explanation = ss.str();

	        }
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
	const string indexDirectory;
	const string jsonFileCompletePath ;
};

class EmptyShardBuilder : public InitialShardHandler{
public:
	EmptyShardBuilder(ShardId * shardId, const string & indexDirectory):
		InitialShardHandler(shardId),
		indexDirectory(indexDirectory){};
	void prepare(bool shouldLockWriteview = true){
		boost::shared_lock<boost::shared_mutex> sLock;
		const CoreInfo_t * indexDataConfig = NULL;
		if(shouldLockWriteview){
			indexDataConfig = ShardManager::getWriteview_read(sLock)->cores.at(shardId->coreId);
		}else{
			indexDataConfig = ShardManager::getWriteview_nolock()->cores.at(shardId->coreId);
		}

		server = boost::shared_ptr<Srch2Server>(new Srch2Server(indexDataConfig, indexDirectory, ""));

		//load the index from the data source
	    try{
	        server->init();
	        if(server->__debugShardingInfo != NULL){
				server->__debugShardingInfo->shardName = shardId->toString();
				stringstream ss;
				ss << "result of empty shard builder, indexDirectory : " << indexDirectory << " .";
				server->__debugShardingInfo->explanation = ss.str();
	        }
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
