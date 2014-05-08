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
#ifdef __MACH__
// This header is used only in mac osx related code
#include <arpa/inet.h>
#endif
#include "HTTPRequestHandler.h"
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
#include "WrapperConstants.h"

#include "transport/TransportManager.h"
#include "routing/RoutingManager.h"
#include "processor/DistributedProcessorExternal.h"
#include "configuration/ConfigManager.h"


namespace po = boost::program_options;
namespace srch2is = srch2::instantsearch;
namespace srch2http = srch2::httpwrapper;

using srch2http::HTTPRequestHandler;
using srch2http::ConfigManager;
using namespace srch2::util;

using std::string;

#define MAX_USER_LEN  20
#define MAX_MESSAGE_LEN  100
#define MAX_MESSAGES 5
#define MAX_SESSIONS 2
#define SESSION_TTL 120


pthread_t *threads = NULL;
unsigned int MAX_THREADS = 0;
srch2http::TransportManager *subway;
// These are global variables that store host and port information for srch2 engine
unsigned short globalDefaultPort;
const char *globalHostName;

// map from port numbers (shared among cores) to socket file descriptors
// IETF RFC 6335 specifies port number range is 0 - 65535: http://tools.ietf.org/html/rfc6335#page-11
typedef std::map<unsigned short /*portNumber*/, int /*fd*/> PortSocketMap_t;

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

static const struct portMap_t {
  srch2http::PortType_t portType;
  const char *operationName;
} portTypeOperationMap[] = {
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

struct CoreShardInfo {
  srch2http::DPExternalRequestHandler& dpHandler;
  srch2http::CoreInfo_t& core;
  srch2http::CoreShardInfo info;

  CoreShardInfo(srch2http::DPExternalRequestHandler &dp,
      srch2http::CoreInfo_t& core) : dpHandler(dp), core(core), 
      info(core.getCoreId(), core.getName()) {}

  CoreShardInfo(const CoreShardInfo& toCpy) : dpHandler(toCpy.dpHandler), 
      core(toCpy.core), info(toCpy.info) {}
  CoreShardInfo& operator=(const CoreShardInfo& toCpy) {
    new (this) CoreShardInfo(toCpy);
    return *this;
  }
};

static bool checkOperationPermission(evhttp_request *req, 
    CoreShardInfo *coreShardInfo, srch2http::PortType_t portType) {
  if (portType >= srch2http::EndOfPortType) {
    Logger::error("Illegal port type: %d", static_cast<int> (portType));
    cb_notfound(req, NULL);
    return false;
  }

  const srch2http::CoreInfo_t *const coreInfo = &coreShardInfo->core;
  unsigned short configuredPort = coreInfo->getPort(portType);
  if(!configuredPort) configuredPort = globalDefaultPort;
  unsigned short arrivalPort = getLibeventHttpRequestPort(req);

  if(!arrivalPort) {
    Logger::warn("Unable to ascertain arrival port from request headers.");
    return false;
  }

  if(!configuredPort) {
    //error
    //TODO: make sure cm return defaultPort on portType which are not set
    // this operation not set to a specific port so only accepted 
    // on the default port
  }

  // compare arrival port to configuration file port
  if(configuredPort != arrivalPort) {
    string coreName = coreInfo->getName();
    Logger::warn("/%s request for %s core arriving on port %d"
       " denied (port %d will permit)",
       portTypeOperationMap[portType].operationName, coreName.c_str(), 
       arrivalPort, configuredPort);

    cb_notfound(req, NULL);
    return false;
  }
  return true;
}

/**
 * 'search' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_search(evhttp_request *req, void *arg) {
  CoreShardInfo *const core = (CoreShardInfo*) arg;
 
  evhttp_add_header(req->output_headers, "Content-Type",
      "application/json; charset=UTF-8");

  if(!checkOperationPermission(req, core, srch2http::SearchPort))
    return;

  try {
    core->dpHandler.externalSearchCommand(req, &core->info);
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
static void cb_suggest(evhttp_request *req, void *arg) {
  CoreShardInfo *const core = (CoreShardInfo*) arg;
  
  evhttp_add_header(req->output_headers, "Content-Type",
      "application/json; charset=UTF-8");

  if(!checkOperationPermission(req, core, srch2http::SuggestPort))
    return;

  try {
    //core->dpHandler.externalSuggestCommand(req, &core->info);
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
static void cb_info(evhttp_request *req, void *arg) {
  CoreShardInfo *const core = (CoreShardInfo*) arg;
  evhttp_add_header(req->output_headers, "Content-Type",
      "application/json; charset=UTF-8");

  if(!checkOperationPermission(req, core, srch2http::InfoPort))
    return;

 try {
   core->dpHandler.externalGetInfoCommand(req, &core->info);
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
static void cb_write(evhttp_request *req, void *arg) {
  CoreShardInfo *const core = (CoreShardInfo*) arg;
  evhttp_add_header(req->output_headers, "Content-Type",
      "application/json; charset=UTF-8");

  if(!checkOperationPermission(req, core, srch2http::DocsPort)) 
    return;

  try {
  // core->dpHandler.externalWriteCommand(req, &core->info, versioninfo);
  } catch (exception& e) {
   // exception caught
   Logger::error(e.what());
   srch2http::HTTPRequestHandler::handleException(req);
  }
}

static void cb_update(evhttp_request *req, void *arg) {
  CoreShardInfo *const core = (CoreShardInfo*) arg;
  evhttp_add_header(req->output_headers, "Content-Type",
      "application/json; charset=UTF-8");

  if(!checkOperationPermission(req, core, srch2http::UpdatePort)) 
    return;

  try {
   core->dpHandler.externalUpdateCommand(req, &core->info);
  } catch (exception& e) {
    // exception caught
    Logger::error(e.what());
    srch2http::HTTPRequestHandler::handleException(req);
  }

}

static void cb_save(evhttp_request *req, void *arg) {
  CoreShardInfo *const core = (CoreShardInfo*) arg;
  evhttp_add_header(req->output_headers, "Content-Type",
      "application/json; charset=UTF-8");

 if(!checkOperationPermission(req, core, srch2http::SavePort)) 
   return;

 try {
   //core->dpHandler.externalSaveCommand(req, &core->info, versioninfo);
 } catch (exception& e) {
   // exception caught
   Logger::error(e.what());
   srch2http::HTTPRequestHandler::handleException(req);
 }
}

static void cb_export(evhttp_request *req, void *arg) {
  CoreShardInfo *const core = (CoreShardInfo*) arg;
  evhttp_add_header(req->output_headers, "Content-Type",
      "application/json; charset=UTF-8");

  if(!checkOperationPermission(req, core, srch2http::ExportPort)) 
    return;

 // core->dpHandler.externalExportCommand(req, &core->info);
}

/**
 *  'reset logger file' callback function
 *  @param req evhttp request object
 *  @param arg optional argument
 */
static void cb_resetLogger(evhttp_request *req, void *arg) {
  CoreShardInfo *const core = (CoreShardInfo*) arg;
  evhttp_add_header(req->output_headers, "Content-Type",
      "application/json; charset=UTF-8");

  if(!checkOperationPermission(req, core, srch2http::ResetLoggerPort)) 
    return;

  try {
    core->dpHandler.externalResetLogCommand(req, &core->info);
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
static void cb_busy_indexing(evhttp_request *req, void *arg) {
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
        pthread_cancel(threads[i]);
    }
    for(srch2http::RouteMap::iterator conn = 
        subway->getRouteMap()->begin();
        conn != subway->getRouteMap()->end(); ++conn) {
      close(conn->second);
    }
    pthread_cancel(subway->getListeningThread());
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

static int getHttpServerMetadata(ConfigManager *config, 
    PortSocketMap_t *globalPortSocketMap) {
  // 1). event initialization 
  globalDefaultPort = atoi(config->getHTTPServerListeningPort().c_str());
  globalHostName = config->getHTTPServerListeningHostname().c_str(); 

  // bind the default port
  if (globalDefaultPort > 0) {
      int socketFd = bindSocket(globalHostName, globalDefaultPort);
      if (socketFd < 0) {
          perror("socket bind error");
          return 255;
      }
      (*globalPortSocketMap)[globalDefaultPort] = socketFd;
  }

  // loop over cores and extract all ports to use
  std::set<short> ports;
  for(ConfigManager::CoreInfoMap_t::iterator core = 
      config->coreInfoIterateBegin();
      core != config->coreInfoIterateEnd(); ++core) {

    // bind once each port defined for use by this core
    unsigned short port;
    for (srch2http::PortType_t portType = (srch2http::PortType_t) 0; 
        portType < srch2http::EndOfPortType; 
        portType = srch2http::incrementPortType(portType)) {
      if((port = core->second->getPort(portType) < 0))
        ports.insert(port);
    }
  } 

  int socketFd;
  for(std::set<short>::const_iterator port = ports.begin(); 
      port != ports.end() ; ++port) {
    if((socketFd = bindSocket(globalHostName, *port)) < 0) {
      perror("socket bind error");
      return 255;
    }
    (*globalPortSocketMap)[*port] = socketFd;
  }
  return 0;
}

static int createHTTPServersAndAccompanyingThreads(int MAX_THREADS,
    vector<struct event_base *> *evBases, vector<struct evhttp *> *evServers) {
  threads = new pthread_t[MAX_THREADS];
  for (int i = 0; i < MAX_THREADS; i++) {
   struct event_base *evbase = event_base_new();
    if(!evbase) {
      perror("event_base_new");
      return 255;
    }
    evBases->push_back(evbase);

   struct evhttp *http_server = evhttp_new(evbase);
      if(!http_server) {
        perror("evhttp_new");
        return 255;
      }
   evServers->push_back(http_server);
  }
  return 0;
}


// helper array - loop instead of repetitous code
static const struct PortList_t {
  const char *path;
  srch2http::PortType_t portType;
  void (*callback)(struct evhttp_request *, void *);
} portList[] = {
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
typedef unsigned CoreId;

int setCallBacksonHTTPServer(ConfigManager *const config,
    evhttp *const http_server, event_base *const evbase, 
    std::vector<CoreShardInfo>& cores) {

  // setup default core callbacks for queries without a core name
  for (int j = 0; portList[j].path != NULL; j++) {
    unsigned short port = 
      config->getDefaultCoreInfo()->getPort(portList[j].portType);
    if (port == 1) port = globalDefaultPort;

    srch2http::CoreInfo_t* defaultCore = config->getDefaultCoreInfo();

    evhttp_set_cb(http_server, portList[j].path, 
        portList[j].callback, &cores[defaultCore->getCoreId()]);
        
    Logger::debug("Routing port %d route %s to default core %s",
        port, portList[j].path, defaultCore->getName().c_str());
  }

  evhttp_set_gencb(http_server, cb_notfound, NULL);

  if(config->getDefaultCoreSetFlag() == true) {
     // for every core, for every OTHER port that core uses, do accept
     for(srch2http::ConfigManager::CoreInfoMap_t::iterator core = 
         config->coreInfoIterateBegin();
         core != config->coreInfoIterateEnd(); ++core) {
       string coreName = core->first;
       for(unsigned int j = 0; portList[j].path != NULL; j++) {
         string path = string("/") + coreName + string(portList[j].path);
              
         unsigned short port = core->second->getPort(portList[j].portType);
         if(port < 1) {
           port = globalDefaultPort;
         }
              
         evhttp_set_cb(http_server, path.c_str(),
             portList[j].callback, &core->second);
              
         Logger::debug("Adding port %d route %s to core %s", 
                  port, path.c_str(), coreName.c_str());
       }
     }
  }

 return 0;
}

int startListeningToRequest(evhttp *const http_server, 
    PortSocketMap_t& globalPortSocketMap) {
  /* 4). accept bound socket */
  for(PortSocketMap_t::iterator iterator = globalPortSocketMap.begin();
      iterator != globalPortSocketMap.end(); iterator++) {
    if(evhttp_accept_socket(http_server, iterator->second) != 0) {
      perror("evhttp_accept_socket");
      return 255;
    }
  //  Logger::debug("Socket accept by thread %d on port %d", i, iterator->first);
  }
  return 0;
}

void generateShards(srch2http::ConfigManager&);

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
    
    // all libevent base objects (one per thread)
    vector<struct event_base *> evBases; 
    vector<struct evhttp *> evServers;
    // map of all ports across all cores to shared socket file descriptors
    PortSocketMap_t globalPortSocketMap;  

    int start = getHttpServerMetadata(serverConf, &globalPortSocketMap);

    if (start != 0) {
        Logger::close();
        return start; // startup failed
    }

    MAX_THREADS = serverConf->getNumberOfThreads();

    createHTTPServersAndAccompanyingThreads(MAX_THREADS, &evBases, &evServers);

    std::vector<srch2http::Node>& map = *serverConf->getCluster()->getNodes();
    
    // create Transport Module
    subway = new srch2http::TransportManager(evBases, map);
    srch2http::RoutingManager *routes = 
      new srch2http::RoutingManager(*serverConf, *subway);
    srch2http::DPExternalRequestHandler *dpHandler = 
      new srch2http::DPExternalRequestHandler(serverConf, routes);
    vector<struct CoreShardInfo> cores;

    for(srch2http::ConfigManager::CoreInfoMap_t::iterator core = 
        serverConf->coreInfoIterateBegin();
                core != serverConf->coreInfoIterateEnd(); ++core)  {
      cores.push_back(CoreShardInfo(*dpHandler, *core->second));
    }

    generateShards(*serverConf);

    for(int j=0; j < evServers.size(); ++j) {
      setCallBacksonHTTPServer(serverConf, evServers[j], evBases[j], cores);
      startListeningToRequest(evServers[j], globalPortSocketMap);
      
      if (pthread_create(threads + j, NULL, dispatch, evBases[j]) != 0)
          return 255;
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

    pthread_join(subway->getListeningThread(), NULL);
    Logger::console("Thread = <%u> stopped", subway->getListeningThread());

    delete[] threads;

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
