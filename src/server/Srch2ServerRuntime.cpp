#include "Srch2ServerRuntime.h"
#include "HTTPJsonResponse.h"
#include "core/analyzer/AnalyzerContainers.h"
#include <event2/event-config.h>
#include <event2/thread.h>

namespace srch2
{
namespace httpwrapper
{

Srch2ServerRuntime * Srch2ServerRuntime::singletonInstance = NULL;

int Srch2ServerRuntime::initializeHttpServerMetadata() {
	// TODO : we should use default port if no port is provided. or just get rid of default port and move it to node
	// as a <defaultPort>
	std::set<short> ports;

	// global host name
	globalHostName = serverConf->getHTTPServerListeningHostname();

	// add the default port
    globalDefaultPort = serverConf->getHTTPServerDefaultListeningPort();
	if (globalDefaultPort > 0) {
		ports.insert(globalDefaultPort);
	}

	Cluster_Writeview * writeview = srch2::httpwrapper::ShardManager::getWriteview();
	unsigned short port;
	// loop over cores and extract all port numbers of current node to use
	for(map<unsigned, CoreInfo_t *>::iterator coresItr = writeview->cores.begin();
			coresItr != writeview->cores.end(); ++coresItr){
		const CoreInfo_t * coreInfo = coresItr->second;
		for (srch2http::PortType_t portType = (srch2http::PortType_t) 0;
				portType < srch2http::EndOfPortType; portType = srch2http::incrementPortType(portType)) {
			if(srch2http::GlobalPortsStart == portType){
				continue;
			}
			port = coreInfo->getPort(portType);
			if(port > 0){
				ports.insert(port);
			}
		}
	}

	// bind once each port defined for use by this core
	for(std::set<short>::const_iterator port = ports.begin();
			port != ports.end() ; ++port) {
		int socketFd = bindSocket(globalHostName.c_str(), *port);
		if(socketFd < 0) {
			perror("socket bind error");
			return 255;
		}
		globalPortSocketMap[*port] = socketFd;
	}
	return 0;
}


int Srch2ServerRuntime::initializeThreadsAndServers(){


	setNumberOfExternalThreads(serverConf->getNumberOfThreads());
	setNumberOfInternalThreads(serverConf->getNumberOfInternalThreads());

    evthread_use_pthreads();


    /***************
     * External threads, event bases and servers
     */
	// for each thread, we bind an evbase and http_server object
	threadsToHandleExternalRequests = new pthread_t[maxExternalThreadCount];
	for (int i = 0; i < maxExternalThreadCount; i++) {
		struct event_base *evbase = event_base_new();
		if(!evbase) {
			perror("event_base_new");
			return 255;
		}
		evBasesForExternalRequests.push_back(evbase);

		struct evhttp *http_server = evhttp_new(evbase);
		if(!http_server) {
			perror("evhttp_new");
			return 255;
		}
		evServersForExternalRequests.push_back(http_server);
	}


	/*****************
	 * Internal threads and event bases
	 */
	// for each thread, we bind an evbase and http_server object
	threadsToHandleInternalRequests = new pthread_t[maxInternalThreadCount];
	for (int i = 0; i < maxInternalThreadCount; i++) {
		struct event_base *evbase = event_base_new();
		if(!evbase) {
			perror("event_base_new");
			return 255;
		}
		evBasesForInternalRequests.push_back(evbase);
	}

	return 0;

}

void Srch2ServerRuntime::initializeTransportManager(){
	// create Transport Module
	srch2http::TransportConfig transportConfig;
	transportConfig.interfaceAddress = serverConf->getTransport().getIpAddress();
	transportConfig.internalCommunicationPort = serverConf->getTransport().getPort();

	transportManager = new srch2http::TransportManager(evBasesForInternalRequests, transportConfig);
}
TransportManager * Srch2ServerRuntime::getTransportManager() const{
	return this->transportManager;
}

void Srch2ServerRuntime::initializeConfigManager(const string & fileName){
	serverConf = new ConfigManager(fileName);
}
ConfigManager * Srch2ServerRuntime::getConfigManager() const{
	return serverConf;
}

ResourceMetadataManager * Srch2ServerRuntime::getMetadataManager() const{
	return metadataManager;
}
//
void Srch2ServerRuntime::initializeShardManager(){
	shardManager = srch2http::ShardManager::createShardManager(serverConf, metadataManager);
}
ShardManager * Srch2ServerRuntime::getShardManager() const{
	return shardManager;
}

void Srch2ServerRuntime::initializeSyncManager(){
	syncManager = new srch2http::SyncManager(*serverConf, *(transportManager));
}
SyncManager * Srch2ServerRuntime::getSyncManager() const{
	return syncManager;
}

void Srch2ServerRuntime::initializeDataProcessorUnits(){
	// create DP Internal
	dpInternal = new srch2http::DPInternalRequestHandler(serverConf);

	// create DP external
	dpExternal = new srch2http::DPExternalRequestHandler(*serverConf, *transportManager, *dpInternal);
}

int Srch2ServerRuntime::startInternalCommunication(){
	// start threads for internal messages.
	// Note: eventbases are not assigned any event yet.
	for(int j=0; j < evBasesForInternalRequests.size(); ++j){
		if (pthread_create(&threadsToHandleInternalRequests[j],
				NULL, dispatchInternalEvent,
				evBasesForInternalRequests[j]) != 0){
			return 255;
		}
	}
	return 0;
}

int Srch2ServerRuntime::startSynchManagerThread(){
	pthread_create(synchronizerThread, NULL, srch2http::bootSynchronizer, (void *)syncManager);
	return 0;
}

int Srch2ServerRuntime::openExternalChannels(){
	// get read view to use for system startup
	boost::shared_ptr<const srch2::httpwrapper::ClusterResourceMetadata_Readview> clusterReadview;
	srch2::httpwrapper::ShardManager::getReadview(clusterReadview);

	Srch2ServerGateway::init(serverConf);
	// bound http_server and evbase and core objects together
	for(int j=0; j < evServersForExternalRequests.size(); ++j) {
		setCallBacksonHTTPServer(evServersForExternalRequests[j], clusterReadview);
		startListeningToRequest(evServersForExternalRequests[j]);

		if (pthread_create(&threadsToHandleExternalRequests[j], NULL, dispatchExternalEvent, evBasesForExternalRequests[j]) != 0){
			return 255;
		}
	}
	return 0;
}

int Srch2ServerRuntime::startSrch2ServerHeartBeat(){
    unsigned int heartBeatTimer = serverConf->getHeartBeatTimer();
    if (heartBeatTimer > 0){
        global_heart_beat_thread = new pthread_t();
        if(pthread_create(global_heart_beat_thread, NULL, heartBeatHandler, &heartBeatTimer) != 0){
        	return 255;
        }
    }
    return 0;
}

void Srch2ServerRuntime::waitForKillSignal(){
	/* Set signal handlers */
	sigset_t sigset;
	sigemptyset(&sigset);

	// handle signal of Ctrl-C interruption
	sigaddset(&sigset, SIGINT);
	// handle signal of terminate(kill)
	sigaddset(&sigset, SIGTERM);
	struct sigaction siginfo;
	// add the handler to be killServer, sa_sigaction and sa_handler are union type, so we don't need to assign sa_sigaction to be NULL
	siginfo.sa_handler = srch2http::Srch2ServerRuntime::killServer;
	siginfo.sa_mask = sigset;
	// If a blocked call to one of the following interfaces is interrupted by a signal handler,
	// then the call will be automatically restarted after the signal handler returns if the SA_RESTART flag was used;
	// otherwise the call will fail with the error EINTR, check the detail at http://man7.org/linux/man-pages/man7/signal.7.html
	siginfo.sa_flags = SA_RESTART;
	sigaction(SIGINT, &siginfo, NULL);
	sigaction(SIGTERM, &siginfo, NULL);

	for (int i = 0; i < maxExternalThreadCount; i++) {
		pthread_join(threadsToHandleExternalRequests[i], NULL);
		Logger::console("Thread = <%u> stopped", threadsToHandleExternalRequests[i]);
	}

	for (int i = 0; i < maxInternalThreadCount; i++) {
		pthread_join(threadsToHandleInternalRequests[i], NULL);
		Logger::console("Thread = <%u> stopped", threadsToHandleInternalRequests[i]);
	}

	syncManager->stop();
	pthread_join(*synchronizerThread, NULL);
	Logger::console("synch thread stopped.");

	if (global_heart_beat_thread) {
		pthread_join(*global_heart_beat_thread, NULL);
		Logger::console("Hear beat thread stopped.");
	}

	pthread_join(*(shardManager->getLoadbalancingThread()), NULL);
	Logger::console("Load balancing thread stopped.");
}

void Srch2ServerRuntime::gracefulExit(){


//    TODO : left from merge, must be fixed when we bring in adaptors
//    //Call the save function implemented by each database connector.
//    DataConnectorThread::saveConnectorTimestamps();

    if (synchronizerThread != NULL){
        delete synchronizerThread;
        synchronizerThread = NULL;
    }
    if (global_heart_beat_thread != NULL){
        delete global_heart_beat_thread;
        global_heart_beat_thread = NULL;
    }
    delete[] threadsToHandleExternalRequests;
    delete[] threadsToHandleInternalRequests;

    for (unsigned int i = 0; i < maxExternalThreadCount; i++) {
        event_base_free(evBasesForExternalRequests[i]);
    }
    for (unsigned int i = 0; i < maxInternalThreadCount; i++) {
        event_base_free(evBasesForInternalRequests[i]);
    }

    AnalyzerContainer::freeAll();
    delete serverConf;
	delete metadataManager ;
	//ShardManager::deleteShardManager();
	delete syncManager ;
	delete transportManager;
    // use global port map to close each file descriptor just once
    for (PortSocketMap_t::iterator iterator = globalPortSocketMap.begin(); iterator != globalPortSocketMap.end(); iterator++) {
        shutdown(iterator->second, SHUT_RD);
    }
	delete dpInternal ;
	delete dpExternal;

    Logger::console("Server stopped successfully");
    Logger::close();
}


unsigned Srch2ServerRuntime::getNumberOfExternalThreads() const{
	return maxExternalThreadCount;
}
void Srch2ServerRuntime::setNumberOfExternalThreads(const unsigned count){
	ASSERT(count > 0);
	maxExternalThreadCount = count;
}

unsigned Srch2ServerRuntime::getNumberOfInternalThreads() const{
	return maxInternalThreadCount;
}
void Srch2ServerRuntime::setNumberOfInternalThreads(const unsigned count){
	ASSERT(count > 0);
	maxInternalThreadCount = count;
}

unsigned short Srch2ServerRuntime::getGlobalDefaultPort() const{
	return globalDefaultPort;
}
void Srch2ServerRuntime::setGlobalDefaultPort(const unsigned short port) {
	this->globalDefaultPort = port;
}
string Srch2ServerRuntime::getGlobalHostName() const {
	return globalHostName;
}
void Srch2ServerRuntime::setGlobalHostName(const string & hostName) {
	this->globalHostName = hostName;
}

void Srch2ServerRuntime::setHeartBeatPulse(){
	this->has_one_pulse = true;
}
void Srch2ServerRuntime::resetHasOnePulse(){
	this->has_one_pulse = false;
}

void* Srch2ServerRuntime::dispatchInternalEvent(void *arg) {
	while(! getInstance()->transportManager->isEventAdded()) {
		sleep(1);
	}
	event_base_dispatch((struct event_base*) arg);
	return NULL;
}

void* Srch2ServerRuntime::dispatchExternalEvent(void *arg) {
	// ask libevent to loop on events
    setup_sigusr2_to_exit();
	event_base_dispatch((struct event_base*) arg);
	return NULL;
}

int Srch2ServerRuntime::startListeningToRequest(evhttp *const http_server) {
	/* 4). accept bound socket */
	for(PortSocketMap_t::iterator iterator = globalPortSocketMap.begin();
			iterator != globalPortSocketMap.end(); iterator++) {
		Logger::console("Port %d added to HTTP listener for external requests.", iterator->first);
		if(evhttp_accept_socket(http_server, iterator->second) != 0) {
			perror("evhttp_accept_socket");
			return 255;
		}
		//  Logger::debug("Socket accept by thread %d on port %d", i, iterator->first);
	}
	return 0;
}


/*
 * This function sets the callback functions to be used for different types of
 * user request URLs.
 * For URLs w/o core name we use the default core (only if default core is actually defined in config manager)
 * For URLs with core names we use the specified core object.
 */
int Srch2ServerRuntime::setCallBacksonHTTPServer(evhttp *const http_server,
		boost::shared_ptr<const srch2::httpwrapper::ClusterResourceMetadata_Readview> & clusterReadview) {

	/// default callback : not found
	evhttp_set_gencb(http_server, Srch2ServerGateway::cb_notfound, NULL);

	for(int j = 0; externalCallbacks.globalPorts[j].path != NULL; j++){
		Srch2ServerGateway::CallbackArgs * global_args = new Srch2ServerGateway::CallbackArgs();
		global_args->dpExternal = dpExternal;
		global_args->coreId = (unsigned)-1;
		global_args->portType = externalCallbacks.globalPorts[j].portType;
		global_args->runtime = this;
		string path = string(externalCallbacks.globalPorts[j].path);
		evhttp_set_cb(http_server, path.c_str(), externalCallbacks.globalPorts[j].callback, global_args);
		// just for print
		unsigned short port = clusterReadview->getCoreByName(serverConf->getDefaultCoreName())->
				getPort(externalCallbacks.globalPorts[j].portType);
		if (port < 1) port = globalDefaultPort;
		Logger::debug("Routing port %d route %s for all cores.", port, externalCallbacks.globalPorts[j].path);
	}
	// setup default core callbacks for queries without a core name
	// only if default core is available.
	if(serverConf->getDefaultCoreSetFlag() == true) {
		// iterate on all operations and map the path (w/o core info)
		// to a callback function.
		// we pass DPExternalCoreHandle as the argument of callbacks
		for (int j = 0; externalCallbacks.coreSpecificPorts[j].path != NULL; j++) {
			// prepare arguments to pass to external commands for default core
			Srch2ServerGateway::CallbackArgs * defaultArgs = new Srch2ServerGateway::CallbackArgs();
			defaultArgs->dpExternal = dpExternal;
			defaultArgs->coreId = clusterReadview->getCoreByName(serverConf->getDefaultCoreName())->getCoreId();
			defaultArgs->runtime = this;
			defaultArgs->portType = externalCallbacks.coreSpecificPorts[j].portType;

			evhttp_set_cb(http_server, externalCallbacks.coreSpecificPorts[j].path,
					externalCallbacks.coreSpecificPorts[j].callback, defaultArgs);
			// just for print
			unsigned short port = clusterReadview->getCoreByName(serverConf->getDefaultCoreName())->
					getPort(externalCallbacks.coreSpecificPorts[j].portType);
			if (port < 1) port = globalDefaultPort;
			Logger::debug("Routing port %d route %s to default core %s",
					port, externalCallbacks.coreSpecificPorts[j].path, serverConf->getDefaultCoreName().c_str());
		}
	}


	// for every core, for every OTHER port that core uses, do accept
	// NOTE : CoreInfoMap is a typedef of std::map<const string, CoreInfo_t *>
	std::vector<const srch2http::CoreInfo_t * > allCores;
	clusterReadview->getAllCores(allCores);
	for(unsigned i = 0 ; i < allCores.size(); ++i) {
		string coreName = allCores.at(i)->getName();
		unsigned coreId = allCores.at(i)->getCoreId();

		if (allCores.at(i)->isAclCore())
			continue;

		for(unsigned int j = 0; externalCallbacks.coreSpecificPorts[j].path != NULL; j++) {
			// prepare external command argument
			Srch2ServerGateway::CallbackArgs * args = new Srch2ServerGateway::CallbackArgs();
			args->dpExternal = dpExternal;
			args->coreId = coreId;
			args->runtime = this;
			args->portType = externalCallbacks.coreSpecificPorts[j].portType;
			string path = string("/") + coreName + string(externalCallbacks.coreSpecificPorts[j].path);
			evhttp_set_cb(http_server, path.c_str(),
					externalCallbacks.coreSpecificPorts[j].callback, args);

			// just for print
			unsigned short port = allCores.at(i)->getPort(externalCallbacks.coreSpecificPorts[j].portType);
			if(port < 1){
				port = globalDefaultPort;
			}
			Logger::debug("Adding port %d route %s to core %s",
					port, path.c_str(), coreName.c_str());
		}
	}

	return 0;
}

void Srch2ServerRuntime::handleException(evhttp_request *req){
    const string INTERNAL_SERVER_ERROR_MSG =
            "{\"error:\" Ooops!! The engine failed to process this request. Please check srch2 server logs for more details. If the problem persists please contact srch2 inc.}";
    bmhelper_evhttp_send_reply2(req, HTTP_INTERNAL, "INTERNAL SERVER ERROR",
            INTERNAL_SERVER_ERROR_MSG);
}


void* Srch2ServerRuntime::heartBeatHandler(void *arg){
    setup_sigusr2_to_exit();
    unsigned seconds = *(unsigned*) arg;
    if (seconds > 0){
        while(true){
        	getInstance()->has_one_pulse = false;
            sleep(seconds);
            if (!getInstance()->has_one_pulse){
                kill(getpid(), SIGTERM);
            }
        }
    }
    return NULL;
}

void Srch2ServerRuntime::setup_sigusr2_to_exit(){
#ifdef ANDROID
    // for the Android, there is no pthread_cancel function,
    // alternatively we use the pthread_kill(thread, SIGUSR2)
    // to send the SIGUSR2 to exit the pthread.
    // reference: http://stackoverflow.com/questions/46SIGUSR2086/pthread-cancel-alternatives-in-android-ndk
    sigset_t sigset;
    sigemptyset(&sigset);

    // handle signal of SIGUSR2, for whatever reason the SIGUSR1 doesn't work well on our Android phone,
    // The SIGUSR2 works fine.
    sigaddset(&sigset, SIGUSR2);
    struct sigaction siginfo;
    siginfo.sa_handler = thread_exit_handler;
    siginfo.sa_mask = sigset;
    // If a blocked call to one of the following interfaces is interrupted by a signal handler,
    // then the call will be automatically restarted after the signal handler returns if the SA_RESTART flag was used;
    // otherwise the call will fail with the error EINTR, check the detail at http://man7.org/linux/man-pages/man7/signal.7.html
    siginfo.sa_flags = SA_RESTART;
    sigaction(SIGUSR2, &siginfo, NULL);
#endif
}


/*
 * TODO : needs comments
 */
int Srch2ServerRuntime::bindSocket(const char * hostname, unsigned short port) {
	int r;
	int nfd;
	nfd = socket(AF_INET, SOCK_STREAM, 0);
	if (nfd < 0)
		return -1;

	int one = 1;
	r = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(int));
	if(r < 0){
		cerr << "Set socket option failed." << endl;  // TODO : change it to Logger
		return 0;
	}

	// ignore a SIGPIPE signal (http://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly)
	signal(SIGPIPE, SIG_IGN);

	struct sockaddr_in addr;
	struct hostent * host = gethostbyname(hostname);
	if (host == NULL)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	bcopy((char *) host->h_addr,
			(char *)&addr.sin_addr.s_addr,
			host->h_length);
	addr.sin_port = htons(port);

	r = bind(nfd, (struct sockaddr*) &addr, sizeof(addr));
	if (r < 0)
		return -1;
	r = listen(nfd, 10240);
	if (r < 0)
		return -1;

	int flags;
	if ((flags = fcntl(nfd, F_GETFL, 0)) < 0
			|| fcntl(nfd, F_SETFL, flags | O_NONBLOCK) < 0)
		return -1;

	return nfd;
}



/**
 * Kill the server.  This function can be called from another thread to kill the server
 */

void Srch2ServerRuntime::killServer(int signal) {
    Logger::console("Stopping server.");
    for (int i = 0; i < getInstance()->maxExternalThreadCount; i++) {
    	event_base_loopbreak(getInstance()->evBasesForExternalRequests[i]);
    }

    // The one line below breaks the pseudo dispatch loop in "dispatchInternalEvent"
    getInstance()->transportManager->setEventAdded();

    for (int i = 0; i < getInstance()->maxInternalThreadCount; i++) {
    	event_base_loopbreak(getInstance()->evBasesForInternalRequests[i]);
    }


    if ( getInstance()->global_heart_beat_thread != NULL ){
#ifdef ANDROID
        pthread_kill(*getInstance()->global_heart_beat_thread, SIGUSR2);
#else
        pthread_cancel(*getInstance()->global_heart_beat_thread);
#endif
    }

    getInstance()->shardManager->setCancelled(); // stops the load balancing thread

    // Surendra -May be the code below is not required
#ifdef ANDROID
    pthread_kill(*(getInstance()->shardManager->getLoadbalancingThread()), SIGUSR2);
#else
    pthread_cancel(*(getInstance()->shardManager->getLoadbalancingThread()));
#endif

}

}

}
