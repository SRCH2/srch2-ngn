
#ifndef __SERVER_SRCH2SERVEREXTERNALGATEWAY_H__
#define __SERVER_SRCH2SERVEREXTERNALGATEWAY_H__

#include "util/Logger.h"
#include "util/Version.h"
#include <event2/http.h>
#include <event2/thread.h>
#include "event2/event.h"
#include "boost/shared_ptr.hpp"

#include <instantsearch/Constants.h>
#include "WrapperConstants.h"
#include "sharding/configuration/ShardingConstants.h"
#include "sharding/sharding/metadata_manager/Cluster.h"

namespace srch2is = srch2::instantsearch;
namespace srch2http = srch2::httpwrapper;

using namespace srch2::util;
using namespace std;

namespace srch2
{
namespace httpwrapper
{

class Srch2ServerRuntime;
class DPExternalRequestHandler;

class Srch2ServerGateway{
public:

	struct CallbackArgs{
		Srch2ServerRuntime * runtime;
		DPExternalRequestHandler * dpExternal;
		unsigned coreId;
		PortType_t portType;
	};

	struct PortInfo{
		const char *path;
		PortType_t portType;
		void (*callback)(struct evhttp_request *, void *);
	};

	static const PortInfo coreSpecificPorts[];
	static const PortInfo globalPorts[];


	// callback functions
	static void cb_coreSpecificOperations(struct evhttp_request *, void *);
	static void cb_globalOperations(struct evhttp_request *, void *);
	static void cb_notfound(evhttp_request *req, void *arg);
	static void cb_wrongauthorizationkey(evhttp_request *req, void *arg);
private:

	/*
	 * example: OAuth=Hey
	 * if the authorization key matches with the key written in the file, only then request will be served
	 * If the config file doesn't have a key, then this check always passes.
	 */
	static bool checkAuthorizationKey(Srch2ServerRuntime * runtime , evhttp_request *req);

	static bool checkGlobalOperationPermission(Srch2ServerRuntime * runtime , evhttp_request *req);

	static bool checkOperationPermission(Srch2ServerRuntime * runtime ,
			boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req,
			unsigned coreId, PortType_t portType) ;


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
	static unsigned short int getLibeventHttpRequestPort(struct evhttp_request *req);


};


}
}


#endif // __SERVER_SRCH2SERVEREXTERNALGATEWAY_H__
