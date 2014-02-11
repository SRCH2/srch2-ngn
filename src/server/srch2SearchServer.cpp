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
#ifdef __MACH__
// This header is used only in mac osx related code
#include <arpa/inet.h>
#endif
#include "HTTPRequestHandler.h"
#include "Srch2Server.h"
#include "license/LicenseVerifier.h"
#include "util/Logger.h"
#include "util/Version.h"
#include <event2/http.h>
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

#define MAX_USER_LEN  20
#define MAX_MESSAGE_LEN  100
#define MAX_MESSAGES 5
#define MAX_SESSIONS 2
#define SESSION_TTL 120

// map from port numbers (shared among cores) to socket file descriptors
typedef std::map<unsigned short, int> PortMap_t;

/*
 * Container for HTTP servers and related information.  Need one of these per thread, per core, per port.
 */
struct Listener_t {
    Srch2Server *srch2Server;
    int fd;
    srch2http::PortType_t portType;

    int init(struct event_base *evbase, int socketFd, srch2http::PortType_t type, Srch2Server *searchServer);
};

int Listener_t::init(struct event_base *evbase, int socketFd, srch2http::PortType_t type, Srch2Server *searchServer)
{
    srch2Server = searchServer;
    portType = type;
    fd = socketFd;

    return 1;
}

// named access to multiple "cores" (ala Solr)
typedef std::map<const std::string, srch2http::Srch2Server *> CoreMap_t;

// helper maps to find listeners by port number and core name
typedef map<const string /*coreName*/, Listener_t *> CoreNameMap_t;
typedef map<unsigned short /*portNumber*/, CoreNameMap_t> ListenersMap_t;

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

/*
 * Helper function to parse HTTP port from it's input headers.  Assumes a little knowledge of http_request
 * internals in libevent.
 */
static short int getLibeventHttpRequestPort(struct evhttp_request *req)
{
    const char *host = NULL;
    const char *p;
    short int port = -1;

    host = evhttp_find_header(req->input_headers, "Host");
    /* The Host: header may include a port. Look for it here, else return -1 as an error. */
    if (host) {
        p = strchr(host,  ':');
        if (p != NULL) {
            p++; // skip past colon
            port = 0;
            while (isdigit(*p)) {
                port = (10 * port) + (*p++ - '0');
            }
            if (*p != '\000') {
                Logger::error("Did not reach end of Host input header");
                port = -1;
            }
        }
    }

    return port;
}

static bool operationPermitted(evhttp_request *req, Listener_t *listener, srch2http::PortType_t operationType)
{
    struct operationMap_t {
        srch2http::PortType_t operationType;
        const char *operationName;
    };
    struct operationMap_t opMap[] = {
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

    if (operationType >= srch2http::EndOfPortType) {
        Logger::error("Illegal operation type: %d", static_cast<int> (operationType));
        cb_notfound(req, static_cast<void *> (listener));
        return false;
    }

    string coreName = listener->srch2Server->getCoreName();
    const srch2http::CoreInfo_t *coreInfo = listener->srch2Server->indexDataConfig;
    unsigned short configuredPort = coreInfo->getPort(operationType);
    short arrivalPort = getLibeventHttpRequestPort(req);

    if (arrivalPort < 0) {
        Logger::warn("Unable to ascertain arrival port from request headers.");
    }

    // this operation configured for a specific port which doesn't match actual
    if (configuredPort > 0 && configuredPort != arrivalPort) {
        Logger::warn("/%s request for %s core arriving on port %d denied (port %d will permit)", opMap[operationType].operationName, coreName.c_str(), arrivalPort, configuredPort);
        cb_notfound(req, static_cast<void *> (listener));
        return false;
    }

    // this operation not set to a specific port so sonly accepted on the default port
    if (configuredPort == 0 && arrivalPort != atoi(listener->srch2Server->indexDataConfig->getHTTPServerListeningPort().c_str())) {
        Logger::warn("/%s request for %s core arriving on port %d denied (default port %d will permit)", opMap[operationType].operationName, coreName.c_str(), arrivalPort, configuredPort);
        cb_notfound(req, static_cast<void *> (listener));
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
    Listener_t *listener = reinterpret_cast<Listener_t *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (operationPermitted(req, listener, srch2http::SearchPort) == false) {
        return;
    }

    try {
        srch2http::HTTPRequestHandler::searchCommand(req, listener->srch2Server);
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
    Listener_t *listener = reinterpret_cast<Listener_t *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (operationPermitted(req, listener, srch2http::SuggestPort) == false) {
        return;
    }

    try {
        srch2http::HTTPRequestHandler::suggestCommand(req, listener->srch2Server);
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
    Listener_t *listener = reinterpret_cast<Listener_t *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    /*string meminfo;
     getMemoryInfo(meminfo);*/

    if (operationPermitted(req, listener, srch2http::InfoPort) == false) {
        return;
    }

    string versioninfo = getCurrentVersion();
    try {
        HTTPRequestHandler::infoCommand(req, listener->srch2Server, versioninfo);
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
    Listener_t *listener = reinterpret_cast<Listener_t *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (operationPermitted(req, listener, srch2http::DocsPort) == false) {
        return;
    }

    try {
        HTTPRequestHandler::writeCommand(req, listener->srch2Server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

static void cb_update(evhttp_request *req, void *arg)
{
    Listener_t *listener = reinterpret_cast<Listener_t *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (operationPermitted(req, listener, srch2http::UpdatePort) == false) {
        return;
    }

    try {
        HTTPRequestHandler::updateCommand(req, listener->srch2Server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

static void cb_save(evhttp_request *req, void *arg)
{
    Listener_t *listener = reinterpret_cast<Listener_t *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (operationPermitted(req, listener, srch2http::SavePort) == false) {
        return;
    }

    try {
        HTTPRequestHandler::saveCommand(req, listener->srch2Server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

static void cb_export(evhttp_request *req, void *arg)
{
    Listener_t *listener = reinterpret_cast<Listener_t *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
                      "application/json; charset=UTF-8");

    if (operationPermitted(req, listener, srch2http::ExportPort) == false) {
        return;
    }

    HTTPRequestHandler::exportCommand(req, listener->srch2Server);
}

/**
 *  'reset logger file' callback function
 *  @param req evhttp request object
 *  @param arg optional argument
 */
static void cb_resetLogger(evhttp_request *req, void *arg)
{
    Listener_t *listener = reinterpret_cast<Listener_t *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");

    if (operationPermitted(req, listener, srch2http::ResetLoggerPort) == false) {
        return;
    }

    try {
    	HTTPRequestHandler::resetLoggerCommand(req, listener->srch2Server);
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

int bindSocket(const char * hostname, int port) {
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

void* dispatch(void *arg) {
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

pthread_t *threads = NULL;
unsigned int MAX_THREADS = 0;

// These are global variables that store host and port information for srch2 engine
short http_port;
const char *http_addr;

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
    struct hostent * host = gethostbyname(http_addr);
    if (host == NULL) {
    	// nothing much can be done..let us try 0.0.0.0
    	strncpy(hostIpAddr, "0.0.0.0", 7);
    } else {
    	// convert binary ip address to human readable ip address.
    	struct in_addr **addr_list = (struct in_addr **) host->h_addr_list;
    	strcpy(hostIpAddr, inet_ntoa(*addr_list[0]));
    }
    conn = evhttp_connection_new( hostIpAddr, http_port);
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
        pthread_cancel(threads[i]);
    }
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

static Listener_t *findListener(ListenersMap_t *listeners, unsigned short port, const string &coreName)
{
    ListenersMap_t::iterator listenator = listeners->find(port);
    if (listenator != listeners->end() && listenator->second.find(coreName) != listenator->second.end()) {
        return listenator->second.find(coreName)->second;
    }
    return NULL;
}

static Listener_t *createListener(ListenersMap_t *listeners, vector<Listener_t *> *allListeners, unsigned short port, srch2http::PortType_t portType, const string &coreName, struct event_base *evbase, int socketFd, struct evhttp *http_server, Srch2Server *srch2Server)
{
    Listener_t *listener = new Listener_t;
    if (listener == NULL) {
        perror("listener allocation failure");
        return NULL;
    }
    if (listener->init(evbase, socketFd, portType, srch2Server) == 0) {
        delete listener;
        perror("listener initialization failure");
        return NULL;
    }
    listener->srch2Server = srch2Server;
    evhttp_set_gencb(http_server, cb_notfound, NULL);
    (*listeners)[port][coreName] = listener; // for unique callback argument per port per core
    allListeners->push_back(listener); // for free when main() exits

    return listener;
}

/*
 * Start the srch2 servers, one per core in the config
 */
static int startServers(ConfigManager *config, vector<struct event_base *> *evBases, vector<struct evhttp *> *evServers, CoreMap_t *cores, PortMap_t *globalPortMap, vector<Listener_t *> *allListeners)
{
    // Step 1: Waiting server
    // http://code.google.com/p/imhttpd/source/browse/trunk/MHttpd.c
    /* 1). event initialization */
    http_port = atoi(config->getHTTPServerListeningPort().c_str());
    http_addr = config->getHTTPServerListeningHostname().c_str(); //"127.0.0.1";

    // bind the default port
    if (http_port > 0 && globalPortMap->find(http_port) == globalPortMap->end()) {
        (*globalPortMap)[http_port] = bindSocket(http_addr, http_port);
        if ((*globalPortMap)[http_port] < 0) {
            perror("socket bind error");
            return 255;
        }
    }

    // create a server (core) for each data source in config file
    for (ConfigManager::CoreInfoMap_t::const_iterator iterator = config->coreInfoIterateBegin();
         iterator != config->coreInfoIterateEnd(); iterator++) {

        srch2http::Srch2Server *core = new srch2http::Srch2Server;
        core->setCoreName(iterator->second->getName());
        (*cores)[iterator->second->getName()] = core;
    }

    // make sure we have identified the default core
    srch2http::Srch2Server *defaultCore = NULL;
    if (cores->find(config->getDefaultCoreName()) != cores->end()) {
        defaultCore = (*cores)[config->getDefaultCoreName()];
    }
    if (defaultCore == NULL)
    {
        perror("Null default core");
        return 255;
    }

    //load the index from the data source
    try{
        for (CoreMap_t::iterator iterator = cores->begin(); iterator != cores->end(); iterator++) {
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

    for (CoreMap_t::const_iterator iterator = cores->begin(); iterator != cores->end(); iterator++) {
        const srch2http::CoreInfo_t *coreInfo = config->getCoreInfo(iterator->second->getCoreName());
        if (coreInfo != NULL) {
            if (coreInfo->getDataSourceType() == srch2::httpwrapper::DATA_SOURCE_MONGO_DB) {
                // set current time as cut off time for further updates
                // this is a temporary solution. TODO
                srch2http::MongoDataSource::bulkLoadEndTime = time(NULL);
                srch2http::MongoDataSource::spawnUpdateListener(iterator->second);
            }

            // bind once each port defined for use by this core
            for (enum srch2http::PortType_t portType = static_cast<srch2http::PortType_t> (0); portType < srch2http::EndOfPortType; portType = srch2http::incrementPortType(portType)) {
                int port = coreInfo->getPort(portType);
                if (port > 0 && (globalPortMap->find(port) == globalPortMap->end() || (*globalPortMap)[port] < 0)) {
                    (*globalPortMap)[port] = bindSocket(http_addr, port);
                    if ((*globalPortMap)[port] < 0) {
                        perror("socket bind error");
                        return 255;
                    }
                }
            }
        }
    }

    MAX_THREADS = config->getNumberOfThreads();
    Logger::console("Starting Srch2 server with %d serving threads at %s:%d",
            MAX_THREADS, http_addr, http_port);

    // Step 2: Serving server
    threads = new pthread_t[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        ListenersMap_t listeners;

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

        Listener_t *defaultListener = new Listener_t;
        if (defaultListener == NULL || defaultListener->init(evbase, (*globalPortMap)[http_port], srch2http::EndOfPortType, defaultCore) == 0) {
            perror("listener allocation failure");
            return 255;
        }
        listeners[http_port][string("")] = defaultListener;
        allListeners->push_back(defaultListener);

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
            Listener_t *listener;
            unsigned int port = config->getDefaultCoreInfo()->getPort(portList[j].portType);
            if (port < 1) {
                listener = defaultListener;
                port = http_port;
            } else {
                listener = findListener(&listeners, port, string(""));
                if (listener == NULL) {
                    listener = createListener(&listeners, allListeners, port, portList[j].portType, string(""), evbase, (*globalPortMap)[port], http_server, defaultCore);
                    if (listener == NULL) {
                        return 255;
                    }
                }
            }
            evhttp_set_cb(http_server, portList[j].path, portList[j].callback, listener);
            Logger::info("Routing port %d route %s to default core %s", port, portList[j].path, listener->srch2Server->getCoreName().c_str());
        }
        evhttp_set_gencb(http_server, cb_notfound, NULL);

        if (config->getDefaultCoreSet() == true) {
            // for every core, for every OTHER port that core uses, do accept, each with their own Listener_t so the
            // callback can check if the URL (path/operation) is permitted        
            for (CoreMap_t::const_iterator iterator = cores->begin(); iterator != cores->end(); iterator++) {

                string coreName = iterator->second->getCoreName();
                for (unsigned int j = 0; portList[j].path != NULL; j++) {
                    string path = string("/") + coreName + string(portList[j].path);
                    Listener_t *listener = NULL;

                    // look if listener already exists for this core on this port
                    unsigned int port = config->getCoreInfo(coreName)->getPort(portList[j].portType);
                    if (port < 1) {
                        port = http_port;
                    }
                    listener = findListener(&listeners, port, coreName);
                    if (listener == NULL) {
                        listener = createListener(&listeners, allListeners, port, portList[j].portType, coreName, evbase, (*globalPortMap)[port], http_server, iterator->second);
                        if (listener == NULL) {
                            return 255;
                        }
                    } else {
                        // listener already exists - just need to add new path/request type
                        listener = listeners[port][coreName];
                    }
                    evhttp_set_cb(http_server, path.c_str(), portList[j].callback, listener);

                    Logger::info("Adding port %d route %s to core %s", port, path.c_str(), listener->srch2Server->getCoreName().c_str());
                }
            }
        }

        /* 4). accept bound socket */
        for (ListenersMap_t::iterator listenator = listeners.begin(); listenator != listeners.end(); listenator++) {
            if (evhttp_accept_socket(http_server, (*globalPortMap)[listenator->first]) != 0) {
                perror("evhttp_accept_socket");
                return 255;
            }
            Logger::debug("Socket accept by thread %d on port %d", i, listenator->first);
        }

        //fprintf(stderr, "Server started on port %d\n", http_port);
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

    LicenseVerifier::testFile(serverConf->getLicenseKeyFileName());
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
    PortMap_t portMap;  // map of all ports across all cores to shared socket file descriptors
    CoreMap_t cores; // map from core names to Srch2Servers
    vector<Listener_t *> listeners; // array of all listeners for all ports, all cores, all threads
    int start = startServers(serverConf, &evBases, &evServers, &cores, &portMap, &listeners);
    if (start != 0) {
        if (logFile)
            fclose(logFile);
        return start; // startup failed
    }

    /* Set signal handlers */
    sigset_t sigset;
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

    for (CoreMap_t::const_iterator iterator = cores.begin(); iterator != cores.end(); iterator++) {
        iterator->second->indexer->save();
    }

    for (unsigned int i = 0; i < MAX_THREADS; i++) {
        event_base_free(evBases[i]);
        evhttp_free(evServers[i]);
    }

    // free resources before we exit
    for (unsigned int i = 0; i < listeners.size(); i++) {
        delete listeners[i];
    }

    // use global port map to close each file descriptor just once
    for (PortMap_t::iterator iterator = portMap.begin(); iterator != portMap.end(); iterator++) {
        shutdown(iterator->second, SHUT_RD);
    }

    AnalyzerContainer::freeAll();

    Logger::console("Server stopped successfully");
    // if no log file is set in config file. This variable should be null.
    // Hence, we should do null check before calling fclose
    if (logFile)
        fclose(logFile);
    return EXIT_SUCCESS;
}
