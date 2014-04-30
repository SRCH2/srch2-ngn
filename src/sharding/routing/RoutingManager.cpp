#include "RoutingManager.h"
#include "server/MongodbAdapter.h"

RoutingManager::RoutingManager(ConfigManager&  cm, TransportManager& tm)  : 
    configurationManager(cm),  tm(tm) dpInternal(cm, *this) {

 // create a server (core) for each data source in config file
 for(ConfigManager::CoreInfoMap_t::const_iterator iterator = 
     config->coreInfoIterateBegin(); iterator != config->coreInfoIterateEnd();
     iterator++) {
    Srch2Server *core = new Srch2Server;
    core->setCoreName(iterator->second->getName());
    shardToIndex[iterator->second->coreId] = core;
    
    if(iterator->second->getDataSourceType() == 
        srch2::httpwrapper::DATA_SOURCE_MONGO_DB) {
      // set current time as cut off time for further updates
      // this is a temporary solution. TODO
      MongoDataSource::bulkLoadEndTime = time(NULL);
      //require srch2Server
      MongoDataSource::spawnUpdateListener(core);
    }
 }

 //load the index from the data source
 try{
   for(CoreNameServerMap_t::iterator iterator = coreNameServerMap->begin();
       iterator != coreNameServerMap->end(); iterator++) {
     iterator->second->init(config);
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
    Logger::error(ex.what());
    exit(-1);
 }
}

//save indexes in deconstructor
