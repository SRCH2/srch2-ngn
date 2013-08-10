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
#include "Srch2KafkaConsumer.h"
#include "HTTPResponse.h"
#include "Srch2Server.h"
#include "license/LicenseVerifier.h"
#include "util/Logger.h"
#include "util/Version.h"
#include <event2/http.h>

#include <sys/types.h>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "util/FileOps.h"

namespace po = boost::program_options;
namespace srch2is = srch2::instantsearch;
namespace srch2http = srch2::httpwrapper;

using srch2http::Srch2Server;
using srch2http::HTTPResponse;
using srch2http::ConfigManager;
using namespace srch2::util;

using std::string;

#define MAX_USER_LEN  20
#define MAX_MESSAGE_LEN  100
#define MAX_MESSAGES 5
#define MAX_SESSIONS 2
#define SESSION_TTL 120

srch2http::Srch2Server server;

/* Convert an amount of bytes into a human readable string in the form
 * of 100B, 2G, 100M, 4K, and so forth.
 * Thanks Redis */
void bytesToHuman(char *s, unsigned long long n) {
    double d;

    if (n < 1024) {
        /* Bytes */
        sprintf(s,"%lluB",n);
        return;
    } else if (n < (1024*1024)) {
        d = (double)n/(1024);
        sprintf(s,"%.2fK",d);
    } else if (n < (1024LL*1024*1024)) {
        d = (double)n/(1024*1024);
        sprintf(s,"%.2fM",d);
    } else if (n < (1024LL*1024*1024*1024)) {
        d = (double)n/(1024LL*1024*1024);
        sprintf(s,"%.2fG",d);
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

std::string getCurrentVersion()
{
    return Version::getCurrentVersion();
}

/*
// A handler for the ajax get message endpoint.
static void ajax_search_command(struct mg_connection *conn,
								const struct mg_request_info *request_info,
								Srch2Server *server)
{
	HTTPResponse::searchCommand(conn, request_info, server);
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
	srch2http::HTTPResponse::searchCommand(req, server);
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
	srch2http::HTTPResponse::lookupCommand(req, server);
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

    HTTPResponse::infoCommand(req, server, versioninfo);
}

/**
 * 'write/v1/' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
void cb_bmwrite_v0(evhttp_request *req, void *arg) {

	Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
	evhttp_add_header(req->output_headers, "Content-Type",
                    "application/json; charset=UTF-8");
	HTTPResponse::writeCommand_v0(req, server);
}

void cb_bmupdate(evhttp_request *req, void *arg) {

	Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
	evhttp_add_header(req->output_headers, "Content-Type",
                    "application/json; charset=UTF-8");
	HTTPResponse::updateCommand(req, server);
}

void cb_bmsave(evhttp_request *req, void *arg) {

	Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
	evhttp_add_header(req->output_headers, "Content-Type",
                    "application/json; charset=UTF-8");
	HTTPResponse::saveCommand(req, server);
}

/**
 * 'write/v2/' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
void cb_bmwrite_v1(evhttp_request *req, void *arg) {

	Srch2Server *server = reinterpret_cast<Srch2Server *>(arg);
	evhttp_add_header(req->output_headers, "Content-Type",
                    "application/json; charset=UTF-8");
	HTTPResponse::writeCommand_v1(req, server);
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
	HTTPResponse::activateCommand(req, server);
}


/**
 * NotFound event handler.
 * @param req evhttp request object
 * @param arg optional argument
 */
void cb_notfound(evhttp_request *req, void *arg)
{
  evhttp_add_header(req->output_headers, "Content-Type",
                    "application/json; charset=UTF-8");
  evhttp_send_reply(req, HTTP_NOTFOUND, "Not found", NULL);
}

/**
 * Busy 409 event handler.
 * @param req evhttp request object
 * @param arg optional argument
 */
void cb_busy_indexing(evhttp_request *req, void *arg)
{
  evhttp_add_header(req->output_headers, "Content-Type",
                    "application/json; charset=UTF-8");
  evhttp_send_reply(req, 409, "Indexer busy", NULL);
}

void printVersion()
{
	std::cout << "SRCH2 server version:" << getCurrentVersion() << std::endl;
}

int bindSocket(const char * hostname , int port) {
    int r;
    int nfd;
    nfd = socket(AF_INET, SOCK_STREAM, 0);
    if (nfd < 0) return -1;

    int one = 1;
    r = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));

    struct sockaddr_in addr;
    struct hostent * host = gethostbyname(hostname);
    if (host == NULL) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    bcopy((char *)host->h_addr,
          (char *)&addr.sin_addr.s_addr,
          host->h_length);
    addr.sin_port = htons(port);

    r = bind(nfd, (struct sockaddr*)&addr, sizeof(addr));
    if (r < 0) return -1;
    r = listen(nfd, 10240);
    if (r < 0) return -1;

    int flags;
    if ((flags = fcntl(nfd, F_GETFL, 0)) < 0
        || fcntl(nfd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;

    return nfd;
}

void* dispatch(void *arg)
{
    event_base_dispatch((struct event_base*)arg);
    return NULL;
}
 
void parseProgramArguments(int argc, char** argv, po::options_description& description, po::variables_map& vm_command_line_args) {
    description.add_options()
    ("help", "Prints help message")
    ("version", "Prints version number of the engine")
    ("config-file", po::value<string>(), "Path to the config file")
    ;
    try{
        po::store(po::parse_command_line(argc, argv, description), vm_command_line_args);
        po::notify(vm_command_line_args);
    }
    catch(exception &ex) {
        cout << "error while parsing the arguments : " << endl <<  ex.what() << endl;
        cout << "Usage: $SRCH2_HOME/bin/srch2-engine" << endl;
        cout << description << endl;
        exit(-1);
    }
}

int main(int argc, char** argv)
{		
	if(argc > 1)
	{ 
		if (strcmp(argv[1], "--version") == 0)
		{
			printVersion();
			return 0;
		}	
	}	
    // Parse command line arguments
    po::options_description description("Optional Arguments");
    po::variables_map vm_command_line_args;
    parseProgramArguments(argc, argv, description, vm_command_line_args);

    if (vm_command_line_args.count("help")) {
        cout << "Usage: $SRCH2_HOME/bin/srch2-engine" << endl;
        cout << description << endl; 
        return 0;
    }

    std::string srch2HomePath = "";
    char * srch2HomePathCStr= getenv("SRCH2_HOME");
    if (srch2HomePathCStr != NULL)
        srch2HomePath = srch2HomePathCStr;

    std::string srch2_config_file = "";
    if (vm_command_line_args.count("config-file")){
        srch2_config_file = vm_command_line_args["config-file"].as<string>();
        int status = ::access(srch2_config_file.c_str(), F_OK);
        if (status != 0) {
           std::cout << "config file = '"<< srch2_config_file <<"' not found or could not be read" << std::endl;    
           return -1;
        }
    }
    else{
        if (srch2HomePath != ""){
            srch2_config_file = srch2HomePath + "/conf/srch2_config.ini";
            int status = ::access(srch2_config_file.c_str(), F_OK);
            if (status != 0) {
                std::cout << "config file = '"<< srch2_config_file <<"' not found or could not be read" << std::endl;
                std::cout << "Please check whether SRCH2_HOME is set correctly" << std::endl;
                return -1;
            }
        }
        else{
            std::cout << "Environment variable SRCH2_HOME is not set " << std::endl;
            std::cout << "Please read README file " << std::endl;
            return -1;
        }
    }
    
	
    ConfigManager *serverConf = new ConfigManager(srch2_config_file);

    serverConf->loadConfigFile();
	
	LicenseVerifier::testFile(serverConf->getLicenseKeyFileName());
    string logDir = getFilePath(serverConf->getHTTPServerAccessLogFile());
    // If the path does't exist, try to create it.
    if(!logDir.empty() && !checkDirExistence(logDir.c_str())){
        if(createDir(logDir.c_str()) == -1){
            exit(1);
        }
    }
    FILE *logFile = fopen(serverConf->getHTTPServerAccessLogFile().c_str(), "a");
    if(logFile == NULL){
    	Logger::setOutputFile(stdout);
    	Logger::error("Open Log file %s failed.", serverConf->getHTTPServerAccessLogFile().c_str());
    }
    else
    	Logger::setOutputFile(logFile);
    Logger::setLogLevel(serverConf->getHTTPServerLogLevel());

	//load the index from the data source
	server.init(serverConf);
	//cout << "srch2 server started." << endl;

	//sleep(200);

	short http_port = atoi(serverConf->getHTTPServerListeningPort().c_str());
	const char *http_addr = serverConf->getHTTPServerListeningHostname().c_str();//"127.0.0.1";
	struct evhttp *http_server = NULL;
	struct event_base *evbase = NULL;
    	
	//std::cout << "Started Srch2 server:" << http_addr << ":" << http_port << std::endl;
	
	// Step 1: Waiting server 
	// http://code.google.com/p/imhttpd/source/browse/trunk/MHttpd.c
	/* 1). event initialization */
    evbase = event_init();
    if(NULL == evbase)
    {
    	perror("event_base_new");
        return 1;
    }
    
    /* 2). event http initialization */
    http_server = evhttp_new(evbase);
    //evhttp_set_max_body_size(http_server, (size_t)server.indexDataContainerConf->getWriteReadBufferInBytes() );

    if(NULL == http_server)
    {
    	perror("evhttp_new");
        return 2;
    }

    /* 3). set general callback of http request */
    evhttp_set_gencb(http_server, cb_busy_indexing, NULL);

    if (server.indexDataContainerConf->getWriteApiType() == srch2http::HTTPWRITEAPI)
    {
      //std::cout << "HTTPWRITEAPI:ON" << std::endl;
      //evhttp_set_cb(http_server, "/docs", cb_bmwrite_v1, &server);
      //evhttp_set_cb(http_server, "/docs_v0", cb_bmwrite_v0, &server);

      evhttp_set_cb(http_server, "/docs", cb_bmwrite_v0, &server); // CHENLI: we use v0

      evhttp_set_cb(http_server, "/update", cb_bmupdate, &server);

      evhttp_set_cb(http_server, "/save", cb_bmsave, &server);

      evhttp_set_cb(http_server, "/activate", cb_bmactivate, &server);
    }

    /* 4). bind socket */
    if(0 != evhttp_bind_socket(http_server, http_addr, http_port))
    {
    	perror("evhttp_bind_socket");   
        return 3;
    }
    
    /* 5). start http server */
    struct timeval ten_sec;
	ten_sec.tv_sec = 1;
	ten_sec.tv_usec = 0;

	/* Now we run the event_base for a series of 5-second intervals, checking every 5 seconds if index was commited.
	 * For a much better way to implement a 5-second timer, see the section below about persistent timer events. 
	 * http://www.wangafu.net/~nickm/libevent-book/Ref3_eventloop.html
	 * */
    while( not server.indexer->isCommited())
    {
		/* This schedules an exit ten seconds from now. */
		event_base_loopexit(evbase, &ten_sec);
		event_base_dispatch(evbase);
  	}
  		        
    /* 6). free resource before exit */
    evhttp_free(http_server);
    event_base_free(evbase);
	
    int MAX_THREADS = serverConf->getNumberOfThreads();
    
     Logger::console("Starting Srch2 server with %d serving threads at %s:%d", MAX_THREADS, http_addr, http_port);
	
    //string meminfo;
    //getMemoryInfo(meminfo);	
	
	//std::cout << meminfo << std::endl;
	// Step 2: Serving server

    int fd = bindSocket(http_addr, http_port);
    pthread_t *threads = new pthread_t[MAX_THREADS];

    for (int i = 0; i < MAX_THREADS; i++)
    {
        evbase = event_init();
        if(NULL == evbase)
        {
            perror("event_base_new");
            return 1;
        }
        
        http_server = evhttp_new(evbase);
        //evhttp_set_max_body_size(http_server, (size_t)server.indexDataContainerConf->getWriteReadBufferInBytes() );

        if(NULL == http_server)
        {
            perror("evhttp_new");
            return 2;
        }

        //http_server = evhttp_start(http_addr, http_port);
        evhttp_set_cb(http_server, "/search", cb_bmsearch, &server);
        //evhttp_set_cb(http_server, "/lookup", cb_bmlookup, &server);
        evhttp_set_cb(http_server, "/info", cb_bminfo, &server);

        if (server.indexDataContainerConf->getWriteApiType() == srch2http::HTTPWRITEAPI)
        {
          // std::cout << "HTTPWRITEAPI:ON" << std::endl;
          //evhttp_set_cb(http_server, "/docs", cb_bmwrite_v1, &server);
          //evhttp_set_cb(http_server, "/docs_v0", cb_bmwrite_v0, &server);
          evhttp_set_cb(http_server, "/docs", cb_bmwrite_v0, &server); // CHENLI: we use v0

          evhttp_set_cb(http_server, "/update", cb_bmupdate, &server);

          evhttp_set_cb(http_server, "/save", cb_bmsave, &server);

          evhttp_set_cb(http_server, "/activate", cb_bmactivate, &server);
        }

        evhttp_set_gencb(http_server, cb_notfound, NULL);
            
        /* 4). bind socket */
        //if(0 != evhttp_bind_socket(http_server, http_addr, http_port))
        if(0 != evhttp_accept_socket(http_server, fd))
        {
            perror("evhttp_bind_socket");   
            return 3;
        }
        
        //fprintf(stderr, "Server started on port %d\n", http_port);
        //event_base_dispatch(evbase);
        if (pthread_create(&threads[i], NULL, dispatch, evbase) != 0)
            return -1;
    }

    for (int i = 0; i < MAX_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    delete[] threads;
    fclose(logFile);

	return EXIT_SUCCESS;
}
