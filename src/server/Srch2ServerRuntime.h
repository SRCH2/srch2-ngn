//$Id: HTTPResponse.h 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

#ifndef __SERVER_SRCH2SERVERRUNTIME_H__
#define __SERVER_SRCH2SERVERRUNTIME_H__


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#if defined( __MACH__) || defined(ANDROID)
// This header is used only in mac osx related code
#include <arpa/inet.h>
#endif
#include "license/LicenseVerifier.h"
#include "util/Logger.h"
#include "util/Version.h"
#include <event2/http.h>
#include <event2/thread.h>
#include "event2/event.h"
#include <signal.h>
#include <sys/types.h>
#include <map>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include "util/FileOps.h"

#include "WrapperConstants.h"
#include "ServerInterfaceInternal.h"
#include "DataConnectorThread.h"
#include "transport/TransportManager.h"
#include "processor/DistributedProcessorExternal.h"
#include "configuration/ConfigManager.h"
#include "synchronization/SynchronizerManager.h"
#include "sharding/sharding/ShardManager.h"
#include "sharding/sharding/metadata_manager/MetadataInitializer.h"
#include "sharding/sharding/metadata_manager/ResourceMetadataManager.h"
#include "discovery/DiscoveryManager.h"
#include "migration/MigrationManager.h"
#include "Srch2ServerExternalGateway.h"



namespace po = boost::program_options;
namespace srch2is = srch2::instantsearch;
namespace srch2http = srch2::httpwrapper;

using srch2http::ConfigManager;
using namespace srch2::util;
using std::string;


#define MAX_USER_LEN  20
#define MAX_MESSAGE_LEN  100
#define MAX_MESSAGES 5
#define MAX_SESSIONS 2
#define SESSION_TTL 120

// map from port numbers (shared among cores) to socket file descriptors
// IETF RFC 6335 specifies port number range is 0 - 65535: http://tools.ietf.org/html/rfc6335#page-11
typedef std::map<unsigned short /*portNumber*/, int /*fd*/> PortSocketMap_t;

namespace srch2
{
namespace httpwrapper
{



class Srch2ServerRuntime{
public:

	static Srch2ServerRuntime * getInstance(){
		if(singletonInstance == NULL){
			singletonInstance = new Srch2ServerRuntime();
		}
		return singletonInstance;
	}

	//
	int initializeHttpServerMetadata();
	//
	int initializeThreadsAndServers();
	//
	void initializeTransportManager();
	TransportManager * getTransportManager() const;
	//
	void initializeConfigManager(const string & fileName);
	ConfigManager * getConfigManager() const;
	//
	ResourceMetadataManager * getMetadataManager() const;
	//
	void initializeShardManager();
	ShardManager * getShardManager() const;
	//
	void initializeSyncManager();
	SyncManager * getSyncManager() const;
	//
	void initializeDataProcessorUnits();
	//
	int startInternalCommunication();
	//
	int startSynchManagerThread();
	//
	int openExternalChannels();
	//
	int startSrch2ServerHeartBeat();
	//
	void waitForKillSignal();
	//
	void gracefulExit();
	//

	//////
	unsigned getNumberOfExternalThreads() const;
	void setNumberOfExternalThreads(const unsigned count);
	//
	unsigned getNumberOfInternalThreads() const;
	void setNumberOfInternalThreads(const unsigned count);
	//
	unsigned short getGlobalDefaultPort() const;
	void setGlobalDefaultPort(const unsigned short port) ;
	//
	string getGlobalHostName() const ;
	void setGlobalHostName(const string & hostName) ;
	//
	void setHeartBeatPulse();
	void resetHasOnePulse();

	static void handleException(evhttp_request *req);

private:
	static Srch2ServerRuntime * singletonInstance;
	Srch2ServerRuntime(){
		threadsToHandleExternalRequests = NULL;
		threadsToHandleInternalRequests = NULL;
		maxExternalThreadCount = 1;
		maxInternalThreadCount = 1;
		serverConf = NULL;
		metadataManager = new srch2http::ResourceMetadataManager();
		shardManager = NULL;
		transportManager = NULL;
		syncManager = NULL;
		synchronizerThread = new pthread_t;
		global_heart_beat_thread = NULL;
		resetHasOnePulse(); // set to false
	}
	Srch2ServerGateway externalCallbacks;


	// These are global variables that store host and port information for srch2 engine
	unsigned short globalDefaultPort;
	string globalHostName;
	PortSocketMap_t globalPortSocketMap;

	// external requests
	pthread_t *threadsToHandleExternalRequests;
	unsigned int maxExternalThreadCount;
	vector<struct event_base *> evBasesForExternalRequests;
	vector<struct evhttp *> evServersForExternalRequests;

	// external communication
	pthread_t *threadsToHandleInternalRequests;
	unsigned int maxInternalThreadCount;
	vector<struct event_base *> evBasesForInternalRequests;

	pthread_t *synchronizerThread ;

	pthread_t * global_heart_beat_thread;
	volatile bool has_one_pulse;

	/********************
	 * Runtime modules
	 */
	ConfigManager *serverConf;
	srch2http::ResourceMetadataManager * metadataManager ;
	srch2http::ShardManager * shardManager ;
	srch2http::SyncManager  * syncManager ;
	srch2http::TransportManager *transportManager;
	srch2http::DPInternalRequestHandler * dpInternal ;
	srch2http::DPExternalRequestHandler *dpExternal;

	static void* dispatchInternalEvent(void *arg) ;
	static void* dispatchExternalEvent(void *arg) ;

	int startListeningToRequest(evhttp *const http_server) ;

	/*
	 * This function sets the callback functions to be used for different types of
	 * user request URLs.
	 * For URLs w/o core name we use the default core (only if default core is actually defined in config manager)
	 * For URLs with core names we use the specified core object.
	 */
	int setCallBacksonHTTPServer(evhttp *const http_server,
			boost::shared_ptr<const srch2::httpwrapper::ClusterResourceMetadata_Readview> & clusterReadview) ;


	// This function is the handler of the heart beat thread.
	// 1. The heartbeat thread sets the SIGUSR2 signal to handle the pthread_exit() function.
	//    We have to use this alternate approach because the Android don't have th pthread_cancel()
	// 2. This thread will sleep every certain seconds, if there is no activity happens during
	//    that time, this thread will send the "SIGTERM" to the process.
	static void* heartBeatHandler(void *arg);
	/**
	 * Kill the server.  This function can be called from another thread to kill the server
	 */
	static void killServer(int signal) ;

	static void setup_sigusr2_to_exit();

	#ifdef ANDROID
	static void thread_exit_handler(int sig){
	    pthread_exit(0);
	}
	#endif

	/*
	 * TODO : needs comments
	 */
	int bindSocket(const char * hostname, unsigned short port) ;



#ifdef __MACH__
	/*
	 *  dummy request handler for make_http_request function below.
	 */
	static void dummyRequestHandler(struct evhttp_request *req, void *state) ;
	/*
	 *  Creates a http request for /info Rest API of srch2 engine.
	 */
	static void makeHttpRequest();
#endif

};



}

}


#endif // __SERVER_SRCH2SERVERRUNTIME_H__
