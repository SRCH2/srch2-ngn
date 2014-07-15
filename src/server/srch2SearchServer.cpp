// $Id: chat.cpp 1217 2011-06-15 20:59:17Z srch2.vijay $
/*
 * This file is part of the Mongoose project, http://code.google.com/p/mongoose
 * It implements an online chat server. For more details,
 * see the documentation on the project web site.
 * To test the application,
 *  1. type "make" in the directory where this file lives
 *  2. point your browser to http://127.0.0.1:8081
 *
 * NOTE(lsm): this file follows Google style, not BSD style as the rest of
 * Mongoose code.
 */

#include <stdio.h>
#include <stdlib.h>
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
#include "HTTPRequestHandler.h"
#include "Srch2Server.h"
#include "license/LicenseVerifier.h"
#include "util/Logger.h"
#include "util/Version.h"
#include <event2/http.h>
#include <event2/thread.h>
#include <signal.h>

#include <sys/types.h>
#include <map>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "util/FileOps.h"
#include "analyzer/AnalyzerContainers.cpp"
#include "MongodbAdapter.h"
#include "WrapperConstants.h"
namespace po = boost::program_options;
namespace srch2is = srch2::instantsearch;
namespace srch2http = srch2::httpwrapper;

using srch2http::Srch2Server;
using srch2http::HTTPRequestHandler;
using srch2http::ConfigManager;
using namespace srch2::util;

using std::string;
using srch2http::CoreNameServerMap_t;

#define MAX_USER_LEN  20
#define MAX_MESSAGE_LEN  100
#define MAX_MESSAGES 5
#define MAX_SESSIONS 2
#define SESSION_TTL 120

// map from port numbers (shared among cores) to socket file descriptors
// IETF RFC 6335 specifies port number range is 0 - 65535: http://tools.ietf.org/html/rfc6335#page-11
typedef std::map<unsigned short /*portNumber*/, int /*fd*/> PortSocketMap_t;

pthread_t *threads = NULL;
unsigned int MAX_THREADS = 0;

// These are global variables that store host and port information for srch2 engine
unsigned short globalDefaultPort;
const char *globalHostName;


/* Convert an amount of bytes into a human readable string in the form
 * of 100B, 2G, 100M, 4K, and so forth.
 * Thanks Redis */
void bytesToHuman(char *s, unsigned long long n) {
    double d;

    if (n < 1024) {
        /* Bytes */
        sprintf(s, "%lluB", n);
        return;
    } else if (n < (1024 * 1024)) {
        d = (double) n / (1024);
        sprintf(s, "%.2fK", d);
    } else if (n < (1024LL * 1024 * 1024)) {
        d = (double) n / (1024 * 1024);
        sprintf(s, "%.2fM", d);
    } else if (n < (1024LL * 1024 * 1024 * 1024)) {
        d = (double) n / (1024LL * 1024 * 1024);
        sprintf(s, "%.2fG", d);
    }
}

std::string getCurrentVersion() {
    return Version::getCurrentVersion();
}

/**
 * NotFound event handler.
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_notfound(evhttp_request *req, void *arg)
{
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        evhttp_send_reply(req, HTTP_NOTFOUND, "Not found", NULL);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }
}

/**
 * Wrong authorization key event handler.
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_wrongauthorizationkey(evhttp_request *req, void *arg)
{
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        evhttp_send_reply(req, HTTP_BADREQUEST, "Wrong authorization key", NULL);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }
}


/*
 * Helper function to parse HTTP port from it's input headers.  Assumes a little knowledge of http_request
 * internals in libevent.
 *
 * Typical usage:
 * void cb_search(evhttp_request *req, void *arg)
 * {
 *     int portNumber = getLibeventHttpRequestPort(req);
 * }
 *
 * where cb_search() is a callback invoked by libevent.
 * If "req" is from "localhost:8082/core1/search", then this function will extract "8082" from "req".
 */
static unsigned short int getLibeventHttpRequestPort(struct evhttp_request *req)
{
    const char *host = NULL;
    const char *p;
    unsigned short int port = 0; // IETF RFC 6335 specifies port number range is 0 - 65535: http://tools.ietf.org/html/rfc6335#page-11


    host = evhttp_find_header(req->input_headers, "Host");
    /* The Host: header may include a port. Look for it here, else return -1 as an error. */
    if (host) {
        p = strchr(host,  ':');
        if (p != NULL) {
            p++; // skip past colon
            port = 0;
            while (isdigit(*p)) {
                unsigned int newValue = (10 * port) + (*p++ - '0'); // check for overflow (must be int)
                if (newValue <= USHRT_MAX) {
                    port = newValue;
                } else {
                    port = 0;
                    break;
                }
            }
            if (*p != '\000') {
                Logger::error("Did not reach end of Host input header");
                port = 0;
            }
        }
    }

    return port;
}


/*
 * example: OAuth=Hey
 * if the authorization key matches with the key written in the file, only then request will be served
 * If the config file doesn't have a key, then this check always passes.
 */
static bool checkAuthorizationKey(evhttp_request *req){

    if(ConfigManager::getAuthorizationKey() == ""){
        return true;
    }

    evkeyvalq headers;
    evhttp_parse_query(req->uri, &headers);

    const char * authorizationKey = evhttp_find_header(&headers, ConfigManager::OAuthParam);

    //If the URL doesn't have a key, the test fails.
    if(!authorizationKey){
         Logger::console("Authorization key not present in the URL");
         return false;
    }

    // If the key in the URL matches the key in the config file, the test passes.
    if(authorizationKey == ConfigManager::getAuthorizationKey())
         return true;

    // If the key in the URL doesn't match the key in the config file, the test fails.
    Logger::console("Wrong authorization key");
        return false;
}


static bool checkGlobalOperationPermission(evhttp_request *req, CoreNameServerMap_t *coreNameServerMap, const char* action)  
{
    if (checkAuthorizationKey(req) == false) {
        cb_wrongauthorizationkey(req, static_cast<void *> (coreNameServerMap));
        return false;
    }

    unsigned short arrivalPort = getLibeventHttpRequestPort(req);

    if (arrivalPort == 0) {
        Logger::warn("Unable to ascertain arrival port from request headers.");
        return false;
    }

    // compare arrival port to globalDefaultPort
    if (globalDefaultPort!= arrivalPort) {
        Logger::warn("/%s request for global operation arriving on port %d denied (port %d will permit)", action,  arrivalPort, globalDefaultPort);
        cb_notfound(req, static_cast<void *> (coreNameServerMap));
        return false;
    }

    return true;

}

static bool checkOperationPermission(evhttp_request *req, Srch2Server *srch2Server, srch2http::PortType_t portType)
{
	 if (checkAuthorizationKey(req) == false) {
	        cb_wrongauthorizationkey(req, static_cast<void *> (srch2Server));
	        return false;
	 }


    struct portMap_t {
        srch2http::PortType_t portType;
        const char *operationName;
    };
    struct portMap_t portTypeOperationMap[] = {
        { srch2http::SearchPort, "search" },
        { srch2http::SuggestPort, "suggest" },
        { srch2http::InfoPort, "info" },
        { srch2http::DocsPort, "docs" },
        { srch2http::UpdatePort, "update" },
        { srch2http::SavePort, "save" },
        { srch2http::ExportPort, "export" },
        { srch2http::ResetLoggerPort, "resetlogger" },
        { srch2http::EndOfPortType, NULL },
    };

    if (portType >= srch2http::EndOfPortType) {
        Logger::error("Illegal port type: %d", static_cast<int> (portType));
        cb_notfound(req, static_cast<void *> (srch2Server));
        return false;
    }

    const srch2http::CoreInfo_t *coreInfo = srch2Server->indexDataConfig;
    // IETF RFC 6335 specifies port number range is 0 - 65535: http://tools.ietf.org/html/rfc6335#page-11
    unsigned short configuredPort = coreInfo->getPort(portType);
    unsigned short arrivalPort = getLibeventHttpRequestPort(req);

    if (arrivalPort == 0) {
        Logger::warn("Unable to ascertain arrival port from request headers.");
        return false;
    }

    if (configuredPort == 0) {
        // this operation not set to a specific port so only accepted on the default port
        configuredPort = atoi(srch2Server->indexDataConfig->getHTTPServerListeningPort().c_str());
    }

    // compare arrival port to configuration file port
    if (configuredPort != arrivalPort) {
        string coreName = srch2Server->getCoreName();
        Logger::warn("/%s request for %s core arriving on port %d denied (port %d will permit)", portTypeOperationMap[portType].operationName, coreName.c_str(), arrivalPort, configuredPort);
        cb_notfound(req, static_cast<void *> (srch2Server));
        return false;
    }

    return true;
}

/**
 * 'search' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_search(evhttp_request *req, void *arg)
{
    Srch2Server *srch2Server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (checkOperationPermission(req, srch2Server, srch2http::SearchPort) == false) {
        return;
    }

    try {
        srch2http::HTTPRequestHandler::searchCommand(req, srch2Server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }
}

/**
 * 'suggest' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_suggest(evhttp_request *req, void *arg)
{
    Srch2Server *srch2Server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (checkOperationPermission(req, srch2Server, srch2http::SuggestPort) == false) {
        return;
    }

    try {
        srch2http::HTTPRequestHandler::suggestCommand(req, srch2Server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }
}

/**
 * 'info' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_info(evhttp_request *req, void *arg)
{
    Srch2Server *srch2Server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (checkOperationPermission(req, srch2Server, srch2http::InfoPort) == false) {
        return;
    }

    string versioninfo = getCurrentVersion();
    try {
        HTTPRequestHandler::infoCommand(req, srch2Server, versioninfo);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

/**
 * 'write/v1/' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_write(evhttp_request *req, void *arg)
{
    Srch2Server *srch2Server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (checkOperationPermission(req, srch2Server, srch2http::DocsPort) == false) {
        return;
    }

    try {
        HTTPRequestHandler::writeCommand(req, srch2Server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

static void cb_update(evhttp_request *req, void *arg)
{
    Srch2Server *srch2Server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (checkOperationPermission(req, srch2Server, srch2http::UpdatePort) == false) {
        return;
    }

    try {
        HTTPRequestHandler::updateCommand(req, srch2Server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

static void cb_save(evhttp_request *req, void *arg)
{
    Srch2Server *srch2Server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (checkOperationPermission(req, srch2Server, srch2http::SavePort) == false) {
        return;
    }

    try {
        HTTPRequestHandler::saveCommand(req, srch2Server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

static void cb_shutdown(evhttp_request *req, void *arg)
{
    CoreNameServerMap_t *coreNameServerMap = reinterpret_cast<CoreNameServerMap_t*>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    
    // The call should only send from the default port
    if (checkGlobalOperationPermission(req, coreNameServerMap, "shutdown") == false ) {
        return ;
    }
	
    try {
        HTTPRequestHandler::shutdownCommand(req, coreNameServerMap);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

/*
 * 'search_all' callback function. It will search each of the core and wrapup the result
 * in a json format
 */
static void cb_search_all (evhttp_request *req, void * arg)
{
    CoreNameServerMap_t *coreNameServerMap = reinterpret_cast<CoreNameServerMap_t*>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    
    // The call should only send from the default port
    if (checkGlobalOperationPermission(req, coreNameServerMap, "search_all") == false ) {
        return ;
    }
	
    try {
        srch2http::HTTPRequestHandler::searchAllCommand(req, coreNameServerMap);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }
   
}



static void cb_export(evhttp_request *req, void *arg)
{
    Srch2Server *srch2Server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
                      "application/json; charset=UTF-8");

    if (checkOperationPermission(req, srch2Server, srch2http::ExportPort) == false) {
        return;
    }

    HTTPRequestHandler::exportCommand(req, srch2Server);
}

/**
 *  'reset logger file' callback function
 *  @param req evhttp request object
 *  @param arg optional argument
 */
static void cb_resetLogger(evhttp_request *req, void *arg)
{
    Srch2Server *srch2Server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (checkOperationPermission(req, srch2Server, srch2http::ResetLoggerPort) == false) {
        return;
    }

    try {
    	HTTPRequestHandler::resetLoggerCommand(req, srch2Server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }
}


/**
 * Busy 409 event handler.
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_busy_indexing(evhttp_request *req, void *arg)
{
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        evhttp_send_reply(req, 409, "Indexer busy", NULL);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }
}

void printVersion() {
    std::cout << "SRCH2 server version:" << getCurrentVersion() << std::endl;
}

int bindSocket(const char * hostname, unsigned short port) {
    int r;
    int nfd;
    nfd = socket(AF_INET, SOCK_STREAM, 0);
    if (nfd < 0)
        return -1;

    int one = 1;
    r = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(int));

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

// entry point for each thread
void* dispatch(void *arg) {
    // ask libevent to loop on events
    event_base_dispatch((struct event_base*) arg);
    return NULL;
}

void parseProgramArguments(int argc, char** argv,
        po::options_description& description,
        po::variables_map& vm_command_line_args) {
    description.add_options()("config-file",po::value<string>(), "Path to the config file")
            ("help", "Prints help message")
            ("version", "Prints version number of the engine");
    try {
        po::store(po::parse_command_line(argc, argv, description),
                vm_command_line_args);
        po::notify(vm_command_line_args);
    } catch (exception &ex) {
        cout << "error while parsing the arguments : " << endl << ex.what()
                << endl;
        cout << "Usage: <SRCH2_HOME>/bin/srch2-engine" << endl;
        cout << description << endl;
        exit(-1);
    }
}

#ifdef __MACH__
/*
 *  dummy request handler for make_http_request function below.
 */
void dummyRequestHandler(struct evhttp_request *req, void *state) {
	if (req == NULL) {
		Logger::debug("timed out!\n");
	} else if (req->response_code == 0) {
		Logger::debug("connection refused!\n");
	} else if (req->response_code != 200) {
		Logger::debug("error: %u %s\n", req->response_code, req->response_code_line);
	} else {
		Logger::debug("success: %u %s\n", req->response_code, req->response_code_line);
	}
}
/*
 *  Creates a http request for /info Rest API of srch2 engine.
 */
void makeHttpRequest(){
    struct evhttp_connection *conn;
    struct evhttp_request *req;
    /*
     * evhttp_connection_new does not perform dns lookup and expects numeric ip address
     * Hence we should do explicit coversion before calling evhttp_connection_new
     */
    char hostIpAddr[20];
	memset(hostIpAddr, 0, sizeof(hostIpAddr));
    struct hostent * host = gethostbyname(globalHostName);
    if (host == NULL) {
    	// nothing much can be done..let us try 0.0.0.0
    	strncpy(hostIpAddr, "0.0.0.0", 7);
    } else {
    	// convert binary ip address to human readable ip address.
    	struct in_addr **addr_list = (struct in_addr **) host->h_addr_list;
    	strcpy(hostIpAddr, inet_ntoa(*addr_list[0]));
    }
    conn = evhttp_connection_new( hostIpAddr, globalDefaultPort);
    evhttp_connection_set_timeout(conn, 1);
    req = evhttp_request_new(dummyRequestHandler, (void*)NULL);
    evhttp_make_request(conn, req, EVHTTP_REQ_GET, "/info");
}
#endif

/**
 * Kill the server.  This function can be called from another thread to kill the server
 */

static void killServer(int signal) {
    Logger::console("Stopping server.");
    for (int i = 0; i < MAX_THREADS; i++) {
#ifdef ANDROID
    	// Android thread implementation does not have pthread_cancel()
    	// use pthread_kill instead.
    	pthread_kill(threads[i], signal);
#else
        pthread_cancel(threads[i]);
#endif
    }
#ifdef ANDROID
    exit(0);
#endif

#ifdef __MACH__
    /*
     *  In MacOS, pthread_cancel could not cancel a thread when the thread is executing kevent syscall
     *  which is a blocking call. In other words, our engine threads are not cancelled while they
     *  are waiting for http requests. So we send a dummy http request to our own engine from within
     *  the engine. This request allows the threads to come out of blocking syscall and get killed.
     */
    makeHttpRequest();
#endif
}

/*
 * Start the srch2 servers, one per core in the config
 */
static int startServers(ConfigManager *config, vector<struct event_base *> *evBases, vector<struct evhttp *> *evServers, CoreNameServerMap_t *coreNameServerMap, PortSocketMap_t *globalPortSocketMap)
{
    // Step 1: Waiting server
    // http://code.google.com/p/imhttpd/source/browse/trunk/MHttpd.c
    /* 1). event initialization */
    globalDefaultPort = atoi(config->getHTTPServerListeningPort().c_str());
    globalHostName = config->getHTTPServerListeningHostname().c_str(); //"127.0.0.1";

    // bind the default port
    if (globalDefaultPort > 0 && globalPortSocketMap->find(globalDefaultPort) == globalPortSocketMap->end()) {
        int socketFd = bindSocket(globalHostName, globalDefaultPort);
        if (socketFd < 0) {
            perror("socket bind error");
            return 255;
        }
        (*globalPortSocketMap)[globalDefaultPort] = socketFd;
    }

    // create a server (core) for each data source in config file
    for (ConfigManager::CoreInfoMap_t::const_iterator iterator = config->coreInfoIterateBegin();
         iterator != config->coreInfoIterateEnd(); iterator++) {

        srch2http::Srch2Server *core = new srch2http::Srch2Server;
        core->setCoreName(iterator->second->getName());
        (*coreNameServerMap)[iterator->second->getName()] = core;
    }

    // make sure we have identified the default core
    srch2http::Srch2Server *defaultCore = NULL;
    if (coreNameServerMap->find(config->getDefaultCoreName()) != coreNameServerMap->end()) {
        defaultCore = (*coreNameServerMap)[config->getDefaultCoreName()];
    }
    if (defaultCore == NULL)
    {
        perror("Null default core");
        return 255;
    }

    //load the index from the data source
    try{
        for (CoreNameServerMap_t::iterator iterator = coreNameServerMap->begin(); iterator != coreNameServerMap->end(); iterator++) {
            iterator->second->init(config);
        }
    }catch(exception& ex) {
    	/*
    	 *  We got some fatal error during server initialization. Print the error message and
    	 *  exit the process. Note: Other internal modules should make sure that no recoverable
    	 *  exception reaches this point. All exceptions that reach here are considered fatal
    	 *  and the server will stop.
    	 */
    	Logger::error(ex.what());
    	return 255;
    }
    //cout << "srch2 server started." << endl;

    // loop over cores setting up mongodb and binding all ports to use
    for (CoreNameServerMap_t::const_iterator iterator = coreNameServerMap->begin(); iterator != coreNameServerMap->end(); iterator++) {
        const srch2http::CoreInfo_t *coreInfo = config->getCoreInfo(iterator->second->getCoreName());
        if (coreInfo != NULL) {
#ifndef ANDROID
            if (coreInfo->getDataSourceType() == srch2::httpwrapper::DATA_SOURCE_MONGO_DB) {
                // set current time as cut off time for further updates
                // this is a temporary solution. TODO
                srch2http::MongoDataSource::bulkLoadEndTime = time(NULL);
                srch2http::MongoDataSource::spawnUpdateListener(iterator->second);
            }
#endif
            // bind once each port defined for use by this core
            for (enum srch2http::PortType_t portType = static_cast<srch2http::PortType_t> (0); portType < srch2http::EndOfPortType; portType = srch2http::incrementPortType(portType)) {
                // IETF RFC 6335 specifies port number range is 0 - 65535: http://tools.ietf.org/html/rfc6335#page-11
                unsigned short port = coreInfo->getPort(portType);
                if (port > 0 && (globalPortSocketMap->find(port) == globalPortSocketMap->end() || (*globalPortSocketMap)[port] < 0)) {
                    int socketFd = bindSocket(globalHostName, port);
                    if ((*globalPortSocketMap)[port] < 0) {
                        perror("socket bind error");
                        return 255;
                    }
                    (*globalPortSocketMap)[port] = socketFd;
                }
            }
        } else {
            Logger::warn("Did not find config for core %s", iterator->second->getCoreName().c_str());
        }
    }

    MAX_THREADS = config->getNumberOfThreads();
    Logger::console("Starting Srch2 server with %d serving threads at %s:%d",
            MAX_THREADS, globalHostName, globalDefaultPort);

    evthread_use_pthreads();
    // Step 2: Serving server
    threads = new pthread_t[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {

        struct event_base *evbase = event_base_new();
        if (NULL == evbase) {
            perror("event_base_new");
            return 255;
        }
        evBases->push_back(evbase);

        struct evhttp *http_server = evhttp_new(evbase);
        if (NULL == http_server) {
            perror("evhttp_new");
            return 255;
        }
        evServers->push_back(http_server);

        // helper array - loop instead of repetitous code
        struct PortList_t {
            const char *path;
            srch2http::PortType_t portType;
            void (*callback)(struct evhttp_request *, void *);
        };
        PortList_t portList[] = {
            { "/search", srch2http::SearchPort, cb_search },
            { "/suggest", srch2http::SuggestPort, cb_suggest },
            { "/info", srch2http::InfoPort, cb_info },
            { "/docs", srch2http::DocsPort, cb_write },
            { "/update", srch2http::UpdatePort, cb_update },
            { "/save", srch2http::SavePort, cb_save },
            { "/export", srch2http::ExportPort, cb_export },
            { "/resetLogger", srch2http::ResetLoggerPort, cb_resetLogger },
            { NULL, srch2http::EndOfPortType, NULL }
        };

        // setup default core callbacks for queries that don't specify a core name
        for (int j = 0; portList[j].path != NULL; j++) {
            unsigned short port = config->getDefaultCoreInfo()->getPort(portList[j].portType);
            if (port == 1) {
                port = globalDefaultPort;
            }
            evhttp_set_cb(http_server, portList[j].path, portList[j].callback, defaultCore);
            Logger::debug("Routing port %d route %s to default core %s", port, portList[j].path, defaultCore->getCoreName().c_str());
        }
        evhttp_set_gencb(http_server, cb_notfound, NULL);

        if (config->getDefaultCoreSetFlag() == true) {
            // for every core, for every OTHER port that core uses, do accept
            for (CoreNameServerMap_t::const_iterator iterator = coreNameServerMap->begin(); iterator != coreNameServerMap->end(); iterator++) {
                string coreName = iterator->second->getCoreName();
                for (unsigned int j = 0; portList[j].path != NULL; j++) {
                    string path = string("/") + coreName + string(portList[j].path);

                    // look if listener already exists for this core on this port
                    unsigned short port = config->getCoreInfo(coreName)->getPort(portList[j].portType);
                    if (port < 1) {
                        port = globalDefaultPort;
                    }
                    evhttp_set_cb(http_server, path.c_str(), portList[j].callback, iterator->second);

                    Logger::debug("Adding port %d route %s to core %s", port, path.c_str(), coreName.c_str());
                }
            }
        }

        // set the global callbacks for all the indexes
        PortList_t global_PortList[] = {
            { "/_all/search", srch2http::EndOfPortType, cb_search_all },
            { "/_all/shutdown", srch2http::EndOfPortType, cb_shutdown },
            { NULL , srch2http::EndOfPortType, NULL }
        };

        for(int j = 0; global_PortList[j].path != NULL; j++){
            string path = string(global_PortList[j].path);
            evhttp_set_cb(http_server, path.c_str(), global_PortList[j].callback, coreNameServerMap);
        }

        /* 4). accept bound socket */
        for (PortSocketMap_t::iterator iterator = globalPortSocketMap->begin(); iterator != globalPortSocketMap->end(); iterator++) {
            if (evhttp_accept_socket(http_server, iterator->second) != 0) {
                perror("evhttp_accept_socket");
                return 255;
            }
            Logger::debug("Socket accept by thread %d on port %d", i, iterator->first);
        }

        //fprintf(stderr, "Server started on port %d\n", globalDefaultPort);
        if (pthread_create(&threads[i], NULL, dispatch, evbase) != 0)
            return 255;
    }

    return 0;
}

int main(int argc, char** argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "--version") == 0) {
            printVersion();
            return 0;
        }
    }
    // Parse command line arguments
    po::options_description description("Valid Arguments");
    po::variables_map vm_command_line_args;
    parseProgramArguments(argc, argv, description, vm_command_line_args);

    if (vm_command_line_args.count("help")) {
        cout << "Usage: <SRCH2_HOME>/bin/srch2-engine" << endl;
        cout << description << endl;
        return 0;
    }

    std::string srch2_config_file = "";
    if (vm_command_line_args.count("config-file")) {
        srch2_config_file = vm_command_line_args["config-file"].as<string>();
        int status = ::access(srch2_config_file.c_str(), F_OK);
        if (status != 0) {
            std::cout << "config file = '" << srch2_config_file
                    << "' not found or could not be read" << std::endl;
            return -1;
        }
    } else {
        cout << "Usage: <SRCH2_HOME>/bin/srch2-engine" << endl;
        cout << description << endl;
        exit(-1);
    } 

    ConfigManager *serverConf = new ConfigManager(srch2_config_file);
    serverConf->loadConfigFile();

    //LicenseVerifier::testFile(serverConf->getLicenseKeyFileName());
    string logDir = getFilePath(serverConf->getHTTPServerAccessLogFile());
    // If the path does't exist, try to create it.
    if (!logDir.empty() && !checkDirExistence(logDir.c_str())) {
        if (createDir(logDir.c_str()) == -1) {
            exit(1);
        }
    }
    FILE *logFile = fopen(serverConf->getHTTPServerAccessLogFile().c_str(),
            "a");
    if (logFile == NULL) {
        Logger::setOutputFile(stdout);
        Logger::error("Open Log file %s failed.",
                serverConf->getHTTPServerAccessLogFile().c_str());
    } else {
        Logger::setOutputFile(logFile);
    }
    Logger::setLogLevel(serverConf->getHTTPServerLogLevel());

    vector<struct event_base *> evBases; // all libevent base objects (one per thread)
    vector<struct evhttp *> evServers;
    PortSocketMap_t globalPortSocketMap;  // map of all ports across all cores to shared socket file descriptors
    CoreNameServerMap_t coreNameServerMap; // map from core names to Srch2Servers
    int start = startServers(serverConf, &evBases, &evServers, &coreNameServerMap, &globalPortSocketMap);
    if (start != 0) {
        Logger::close();
        return start; // startup failed
    }

    /* Set signal handlers */
    sigset_t sigset;
    sigemptyset(&sigset);

    // handle signal of Ctrl-C interruption
    sigaddset(&sigset, SIGINT);
    // handle signal of terminate(kill)
    sigaddset(&sigset, SIGTERM);
    struct sigaction siginfo;
    // add the handler to be killServer, sa_sigaction and sa_handler are union type, so we don't need to assign sa_sigaction to be NULL
    siginfo.sa_handler = killServer;
    siginfo.sa_mask = sigset;
    // If a blocked call to one of the following interfaces is interrupted by a signal handler,
    // then the call will be automatically restarted after the signal handler returns if the SA_RESTART flag was used;
    // otherwise the call will fail with the error EINTR, check the detail at http://man7.org/linux/man-pages/man7/signal.7.html
    siginfo.sa_flags = SA_RESTART;
    sigaction(SIGINT, &siginfo, NULL);
    sigaction(SIGTERM, &siginfo, NULL);

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
        Logger::console("Thread = <%u> stopped", threads[i]);
    }

    delete[] threads;

    for (CoreNameServerMap_t::iterator iterator = coreNameServerMap.begin(); iterator != coreNameServerMap.end(); iterator++) {
        iterator->second->indexer->save();
        delete iterator->second;
    }

    for (unsigned int i = 0; i < MAX_THREADS; i++) {
        event_base_free(evBases[i]);
    }

    // use global port map to close each file descriptor just once
    for (PortSocketMap_t::iterator iterator = globalPortSocketMap.begin(); iterator != globalPortSocketMap.end(); iterator++) {
        shutdown(iterator->second, SHUT_RD);
    }

    AnalyzerContainer::freeAll();
    delete serverConf;

    Logger::console("Server stopped successfully");
    Logger::close();
    return EXIT_SUCCESS;
}
