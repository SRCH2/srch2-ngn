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

// named access to multiple "cores" (ala Solr)
typedef std::map<const std::string, srch2http::Srch2Server *> ServerMap_t;
ServerMap_t servers;
srch2http::Srch2Server *defaultCore = NULL;

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

// Uses spinlock and volatile to increment count.
#define PREFIX_SIZE (sizeof(size_t))
/*
 static size_t used_memory = 0;
 pthread_mutex_t used_memory_mutex = PTHREAD_MUTEX_INITIALIZER;

 struct MemCounter
 {
 static void increment(size_t size)
 {
 pthread_mutex_lock(&used_memory_mutex);
 used_memory += size;
 pthread_mutex_unlock(&used_memory_mutex);
 }

 static void decrement(size_t size)
 {
 pthread_mutex_lock(&used_memory_mutex);
 used_memory -= size;
 pthread_mutex_unlock(&used_memory_mutex);
 }

 static size_t getUsedMemory()
 {
 return used_memory;
 }
 };*/

// http://stackoverflow.com/questions/852072/simple-c-implementation-to-track-memory-malloc-free
// http://eli.thegreenplace.net/2011/02/17/the-many-faces-of-operator-new-in-c/
/*void* operator new(size_t size) throw (std::bad_alloc)
 {
 //cerr << "allocating " << sz << " bytes\n";
 void *ptr = malloc(size+PREFIX_SIZE);

 *((size_t*)ptr) = size;
 MemCounter::increment(size + PREFIX_SIZE);


 if (ptr)
 return (char*)ptr+PREFIX_SIZE;
 else
 throw std::bad_alloc();
 }

 void operator delete(void* ptr) throw()
 {
 void *realptr;
 size_t oldsize;

 if (ptr == NULL) return;

 realptr = (char*)ptr-PREFIX_SIZE;
 oldsize = *((size_t*)realptr);

 MemCounter::decrement(oldsize+PREFIX_SIZE);

 free(realptr);
 }*/
/*
 void getMemoryInfo(std::string &meminfo)
 {
 char hmem[64];
 //char peak_hmem[64];

 bytesToHuman(hmem, MemCounter::getUsedMemory());
 //bytesToHuman(peak_hmem,server.stat_peak_memory);

 //if (sections++) info = sdscat(info,"\r\n");
 stringstream mem_info;
 mem_info
 << "{" << "\"used_memory\":" << MemCounter::getUsedMemory()
 << ",\"used_memory_human\":\"" << hmem << "\"}";

 meminfo = mem_info.str();
 }*/

std::string getCurrentVersion() {
    return Version::getCurrentVersion();
}

/*
 // A handler for the ajax get message endpoint.
 static void ajax_search_command(struct mg_connection *conn,
 const struct mg_request_info *request_info,
 Srch2Server *server)
 {
 HTTPRequestHandler::searchCommand(conn, request_info, server);
 }
 */

/*// A handler for the /ajax/send_message endpoint.
 static void ajax_health_command(struct mg_connection *conn,
 const struct mg_request_info *request_info,
 Srch2Server *server)
 {
 std::stringstream str;
 str << HTTPServerEndpoints::ajax_search_pass
 << server->indexer->getIndexHealth()
 << "Memory usage:"
 << get_memory_usage(getpid())/1024;
 mg_write(conn, str.str().c_str(), str.str().length() );
 }*/

/**
 * 'search' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
void cb_bmsearch(evhttp_request *req, void *arg) {
    Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        srch2http::HTTPRequestHandler::searchCommand(req, server);
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
void cb_bmsuggest(evhttp_request *req, void *arg) {
    Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        srch2http::HTTPRequestHandler::suggestCommand(req, server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }
}

/**
 * 'lookup' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
void cb_bmlookup(evhttp_request *req, void *arg) {
    Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        srch2http::HTTPRequestHandler::lookupCommand(req, server);
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
void cb_bminfo(evhttp_request *req, void *arg) {
    Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    /*string meminfo;
     getMemoryInfo(meminfo);*/
    string versioninfo = getCurrentVersion();
    try {
        HTTPRequestHandler::infoCommand(req, server, versioninfo);
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
void cb_bmwrite(evhttp_request *req, void *arg) {
    Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        HTTPRequestHandler::writeCommand(req, server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

void cb_bmupdate(evhttp_request *req, void *arg) {
    Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        HTTPRequestHandler::updateCommand(req, server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

void cb_bmsave(evhttp_request *req, void *arg) {
    Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        HTTPRequestHandler::saveCommand(req, server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

void cb_bmexport(evhttp_request *req, void *arg)
{
    Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
                      "application/json; charset=UTF-8");
    HTTPRequestHandler::exportCommand(req, server);
}

/**
 *  'reset logger file' callback function
 *  @param req evhttp request object
 *  @param arg optional argument
 */
void cb_bmresetLogger(evhttp_request *req, void *arg) {
    Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
    	HTTPRequestHandler::resetLoggerCommand(req, server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }
}


/**
 * 'activate' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
void cb_bmactivate(evhttp_request *req, void *arg) {
    Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        HTTPRequestHandler::activateCommand(req, server);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
    }

}

/**
 * NotFound event handler.
 * @param req evhttp request object
 * @param arg optional argument
 */
void cb_notfound(evhttp_request *req, void *arg) {
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
 * Busy 409 event handler.
 * @param req evhttp request object
 * @param arg optional argument
 */
void cb_busy_indexing(evhttp_request *req, void *arg) {
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

static bool areAllServersCommitted(const ServerMap_t *servers)
{
    bool committed = true;
    for (ServerMap_t::const_iterator iterator = servers->begin(); iterator != servers->end(); iterator++) {
        if (! iterator->second->indexer->isCommited()) {
            committed = false;
        }
    }
    return committed;
}

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

/*
 * Start the srch2 servers, one per core in the config
 */
static int startServers(ConfigManager *config, ServerMap_t *servers, vector<struct event_base *> *evbases, vector<struct evhttp *> *http_servers)
{
    // Step 1: Waiting server
    // http://code.google.com/p/imhttpd/source/browse/trunk/MHttpd.c
    /* 1). event initialization */
    http_port = atoi(config->getHTTPServerListeningPort().c_str());
    http_addr =
            config->getHTTPServerListeningHostname().c_str(); //"127.0.0.1";
    struct evhttp *http_server = NULL;
    struct event_base *evbase = NULL;

    evbase = event_init();
    if (NULL == evbase) {
        perror("event_base_new");
        return 1;
    }

    /* 2). event http initialization */
    http_server = evhttp_new(evbase);
    //evhttp_set_max_body_size(http_server, (size_t)server.indexDataContainerConf->getWriteReadBufferInBytes() );

    if (NULL == http_server) {
        perror("evhttp_new");
        return 2;
    }

    /* 3). set general callback of http request */
    evhttp_set_gencb(http_server, cb_busy_indexing, NULL);

    // create a server (core) for each data source in config file
    for (ConfigManager::CoreInfoMap_t::const_iterator iterator = config->coreInfoIterateBegin();
         iterator != config->coreInfoIterateEnd(); iterator++) {

        srch2http::Srch2Server *core = new srch2http::Srch2Server;
        core->setCoreName(iterator->second->getName());
        (*servers)[iterator->second->getName()] = core;
    }

    // make sure we have identified the default core
    if (defaultCore == NULL && config->getDefaultCoreName().compare("") != 0) {
        defaultCore = (*servers)[config->getDefaultCoreName()];
    }
    ASSERT(defaultCore != NULL);

    /* 4). bind socket */
    if (0 != evhttp_bind_socket(http_server, http_addr, http_port)) {
        perror("evhttp_bind_socket");
        return 3;
    }

    /* 6). free resource before exit */
    evhttp_free(http_server);
    event_base_free(evbase);

    //load the index from the data source
    try{
        for (ServerMap_t::iterator iterator = servers->begin(); iterator != servers->end(); iterator++) {
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
    	return(-1);
    }
    //cout << "srch2 server started." << endl;

    for (ServerMap_t::const_iterator iterator = servers->begin(); iterator != servers->end(); iterator++) {
        const srch2http::CoreInfo_t *coreInfo = config->getCoreInfo(iterator->second->getCoreName());
        if (coreInfo != NULL && coreInfo->getDataSourceType() == srch2::httpwrapper::DATA_SOURCE_MONGO_DB) {
            // set current time as cut off time for further updates
            // this is a temporary solution. TODO
            srch2http::MongoDataSource::bulkLoadEndTime = time(NULL);
            srch2http::MongoDataSource::spawnUpdateListener(iterator->second);
        }
    }

    //std::cout << "Started Srch2 server:" << http_addr << ":" << http_port << std::endl;

    MAX_THREADS = config->getNumberOfThreads();

    Logger::console("Starting Srch2 server with %d serving threads at %s:%d",
            MAX_THREADS, http_addr, http_port);

    //string meminfo;
    //getMemoryInfo(meminfo);

    //std::cout << meminfo << std::endl;
    // Step 2: Serving server

    int fd = bindSocket(http_addr, http_port);
    threads = new pthread_t[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        evbase = event_init();
        evbases->push_back(evbase);
        if (NULL == evbase) {
            perror("event_base_new");
            return 1;
        }

        http_server = evhttp_new(evbase);
        http_servers->push_back(http_server);
        //evhttp_set_max_body_size(http_server, (size_t)defaultCore->indexDataConfig->getWriteReadBufferInBytes() );

        if (NULL == http_server) {
            perror("evhttp_new");
            return 2;
        }

        // setup default core callbacks
        //http_server = evhttp_start(http_addr, http_port);
        evhttp_set_cb(http_server, "/search", cb_bmsearch, defaultCore);
        evhttp_set_cb(http_server, "/suggest", cb_bmsuggest, defaultCore);

        //evhttp_set_cb(http_server, "/lookup", cb_bmlookup, defaultCore);
        evhttp_set_cb(http_server, "/info", cb_bminfo, defaultCore);

        evhttp_set_cb(http_server, "/docs", cb_bmwrite, defaultCore);
        evhttp_set_cb(http_server, "/update", cb_bmupdate, defaultCore);
        evhttp_set_cb(http_server, "/save", cb_bmsave, defaultCore);
        evhttp_set_cb(http_server, "/export", cb_bmexport, defaultCore);
        evhttp_set_cb(http_server, "/activate", cb_bmactivate, defaultCore);
        evhttp_set_cb(http_server, "/resetLogger", cb_bmresetLogger, defaultCore);

        // setup named core callbacks
        for (ServerMap_t::const_iterator iterator = servers->begin(); iterator != servers->end(); iterator++) {

            string path = string("/") + iterator->second->getCoreName() + string("/search");
            evhttp_set_cb(http_server, path.c_str(), cb_bmsearch, iterator->second);

            path = string("/") + iterator->second->getCoreName() + string("/suggest");
            evhttp_set_cb(http_server, path.c_str(), cb_bmsuggest, iterator->second);

            //evhttp_set_cb(http_server, "/lookup", cb_bmlookup, iterator->second);

            path = string("/") + iterator->second->getCoreName() + string("/info");
            evhttp_set_cb(http_server, path.c_str(), cb_bminfo, iterator->second);

            path = string("/") + iterator->second->getCoreName() + string("/docs");
            evhttp_set_cb(http_server, path.c_str(), cb_bmwrite, iterator->second);

            path = string("/") + iterator->second->getCoreName() + string("/update");
            evhttp_set_cb(http_server, path.c_str(), cb_bmupdate, iterator->second);

            path = string("/") + iterator->second->getCoreName() + string("/save");
            evhttp_set_cb(http_server, path.c_str(), cb_bmsave, iterator->second);

            path = string("/") + iterator->second->getCoreName() + string("/export");
            evhttp_set_cb(http_server, path.c_str(), cb_bmexport, iterator->second);

            path = string("/") + iterator->second->getCoreName() + string("/activate");
            evhttp_set_cb(http_server, path.c_str(), cb_bmactivate, iterator->second);

            path = string("/") + iterator->second->getCoreName() + string("/resetLogger");
            evhttp_set_cb(http_server, path.c_str(), cb_bmresetLogger, iterator->second);
        }

        evhttp_set_gencb(http_server, cb_notfound, NULL);

        /* 4). bind socket */
        //if(0 != evhttp_bind_socket(http_server, http_addr, http_port))
        if (0 != evhttp_accept_socket(http_server, fd)) {
            perror("evhttp_bind_socket");
            return 3;
        }

        //fprintf(stderr, "Server started on port %d\n", http_port);
        //event_base_dispatch(evbase);
        if (pthread_create(&threads[i], NULL, dispatch, evbase) != 0)
            return -1;
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

    vector<struct event_base *> evbases;
    vector<struct evhttp *> http_servers;

    int start = startServers(serverConf, &servers, &evbases, &http_servers);
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

    for (ServerMap_t::const_iterator iterator = servers.begin(); iterator != servers.end(); iterator++) {
        iterator->second->indexer->save();
    }

// free resources before we exit
    for (int i = 0; i < MAX_THREADS; i++) {
        evhttp_free(http_servers[i]);
        event_base_free(evbases[i]);
    }

    AnalyzerContainer::freeAll();

    Logger::console("Server stopped successfully");
    // if no log file is set in config file. This variable should be null.
    // Hence, we should do null check before calling fclose
    if (logFile)
        fclose(logFile);
    return EXIT_SUCCESS;
}
