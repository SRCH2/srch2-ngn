#include "Srch2ServerExternalGateway.h"
#include "Srch2ServerRuntime.h"
#include "HTTPJsonResponse.h"
#include "sharding/processor/DistributedProcessorExternal.h"
#include "sharding/configuration/ConfigManager.h"
#include "sharding/sharding/transactions/cluster_transactions/ShardCommandHttp.h"
#include "sharding/sharding/transactions/cluster_transactions/ClusterShutdownOperation.h"
#include <exception>


namespace srch2
{
namespace httpwrapper
{

Srch2ServerGateway::PortInfo * Srch2ServerGateway::coreSpecificPorts = NULL;
Srch2ServerGateway::PortInfo * Srch2ServerGateway::globalPorts = NULL;

void Srch2ServerGateway::init(ConfigManager * serverConf){
	// Ready to take off.
	// We are going open to the outside world.
	// Connecting to the Sharding core API_a (coreSpecificPorts[portType].callback)

	coreSpecificPorts = new PortInfo[(unsigned)GlobalPortsStart + 1];
	for (srch2http::PortType_t portType = (srch2http::PortType_t) 0;
			portType < srch2http::GlobalPortsStart; portType = srch2http::incrementPortType(portType)) {
		coreSpecificPorts[portType].callback = cb_coreSpecificOperations;
		coreSpecificPorts[portType].path = serverConf->portNameMap[portType].portPath;
		coreSpecificPorts[portType].portType = portType;
	}
	PortInfo endPortInfo = { NULL , srch2http::EndOfPortType, NULL };
	coreSpecificPorts[(unsigned)GlobalPortsStart] = endPortInfo;

	globalPorts = new PortInfo[(unsigned) EndOfPortType - (unsigned)GlobalPortsStart];
	for (srch2http::PortType_t portType = (srch2http::PortType_t)((unsigned)GlobalPortsStart + 1);
			portType < srch2http::EndOfPortType; portType = srch2http::incrementPortType(portType)) {
		unsigned portIndex = (unsigned)portType - ((unsigned)srch2::httpwrapper::GlobalPortsStart + 1);
		globalPorts[portIndex].callback = cb_globalOperations;
		globalPorts[portIndex].path = serverConf->portNameMap[portType].portPath;
		globalPorts[portIndex].portType = portType;
	}
	globalPorts[((unsigned) EndOfPortType - (unsigned)GlobalPortsStart) - 1 ] = endPortInfo;
}

void Srch2ServerGateway::cb_coreSpecificOperations(struct evhttp_request * req, void * arg){
	if (arg == NULL){
		return;
	}
	Srch2ServerGateway::CallbackArgs * cbArgs = (Srch2ServerGateway::CallbackArgs * )arg;
	unsigned coreId = cbArgs->coreId;
	srch2http::PortType_t portType = cbArgs->portType;
	srch2::httpwrapper::DPExternalRequestHandler * dpExternal = cbArgs->dpExternal;
	Srch2ServerRuntime * runtime = cbArgs->runtime;
    // set has_one_pulse to true to let the heart-beat thread notice there is one activity.
    runtime->setHeartBeatPulse();

	boost::shared_ptr<const srch2::httpwrapper::ClusterResourceMetadata_Readview> clusterReadview;
	srch2::httpwrapper::ShardManager::getReadview(clusterReadview);

    if (checkOperationPermission(runtime, clusterReadview, req, coreId, cbArgs->portType) == false) {
		cb_notfound(req, NULL);
    	return;
	}


    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try{
    	switch (portType){
    	case srch2http::SearchPort:
    		dpExternal->externalSearchCommand(clusterReadview, req, coreId);
    		break;
    	case srch2http::SuggestPort:
//    		dpExternal->externalSuggestCommand(req, coreId); //TODO
    		break;
    	case srch2http::InfoPort:
    		dpExternal->externalGetInfoCommand(clusterReadview, req, coreId);
    		break;
    	case srch2http::DocsPort:
    	    if(req->type == EVHTTP_REQ_PUT){
                dpExternal->externalInsertCommand(clusterReadview, req, coreId);
    	    }else if(req->type == EVHTTP_REQ_DELETE){
    	    	Logger::console("Delete request came ...");
    	    	dpExternal->externalDeleteCommand(clusterReadview, req, coreId);
    	    }
    		break;
    	case srch2http::UpdatePort:
    		dpExternal->externalUpdateCommand(clusterReadview, req, coreId);
    		break;
    	case srch2http::SavePort:
    		ShardCommandHttpHandler::runCommand(clusterReadview, req, coreId, ShardCommandCode_SaveData_SaveMetadata);
    		break;
    	case srch2http::ExportPort:
    		ShardCommandHttpHandler::runCommand(clusterReadview, req, coreId, ShardCommandCode_Export);
    		break;
    	case srch2http::ResetLoggerPort:
    		ShardCommandHttpHandler::runCommand(clusterReadview, req, coreId, ShardCommandCode_ResetLogger);
    		break;
    	case srch2http::CommitPort:
    		ShardCommandHttpHandler::runCommand(clusterReadview, req, coreId, ShardCommandCode_Commit);
    		break;
    	case srch2http::MergePort: // also includes mergeSetOn and mergeSetOff
    		ShardCommandHttpHandler::runCommand(clusterReadview, req, coreId, ShardCommandCode_Merge);
    		break;
    	case srch2http::AttributeAclAdd:
    	case srch2http::AttributeAclDelete:
    	case srch2http::AttributeAclAppend:
    		//dpExternal->externalAclAttributeModifyCommand(clusterReadview, req, coreId);
    		break;
    	case srch2http::RecordAclAdd:
    		//dpExternal->externalAclRecordAddRolesCommand(clusterReadview, req, coreId);
    		break;
    	case srch2http::RecordAclAppend:
    		//dpExternal->externalAclRecordAppendRolesCommand(clusterReadview, req, coreId);
    		break;
    	case srch2http::RecordAclDelete:
    		//dpExternal->externalAclRecordDeleteRolesCommand(clusterReadview, req, coreId);
    		break;
    	default:
    		cb_notfound(req, NULL);
    		break;
    	}
    }catch(exception & e){
    	// exception caught
    	Logger::error(e.what());
    	srch2http::Srch2ServerRuntime::handleException(req);
    }


}
void Srch2ServerGateway::cb_globalOperations(struct evhttp_request * req, void * arg){
	if (arg == NULL){
		return;
	}
	Srch2ServerGateway::CallbackArgs * cbArgs = (Srch2ServerGateway::CallbackArgs * )arg;
	unsigned coreId = cbArgs->coreId;
	ASSERT(coreId == (unsigned)-1);
	srch2http::PortType_t portType = cbArgs->portType;
	srch2::httpwrapper::DPExternalRequestHandler * dpExternal = cbArgs->dpExternal;
	Srch2ServerRuntime * runtime = cbArgs->runtime;
	// set has_one_pulse to true to let the heart-beat thread notice there is one activity.
    runtime->setHeartBeatPulse();

	if (checkGlobalOperationPermission(runtime, req) == false) {
		cb_notfound(req, NULL);
    	return;
	}
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
	boost::shared_ptr<const srch2::httpwrapper::ClusterResourceMetadata_Readview> clusterReadview;
	srch2::httpwrapper::ShardManager::getReadview(clusterReadview);
    try{
    	switch (portType){
    	case srch2http::SearchAllPort:
    		dpExternal->externalSearchAllCommand(clusterReadview, req);
    		break;
    	case srch2http::InfoPort:
    		dpExternal->externalGetInfoCommand(clusterReadview, req, (unsigned) -1);
    		break;
    	case srch2http::InfoPort_Nodes_NodeID:
    	case srch2http::InfoPort_Cluster_Stats:
    	case srch2http::DebugStatsPort:
    		dpExternal->externalGetInfoCommand(clusterReadview, req, (unsigned) -1, portType);
    		break;
    	case srch2http::ShutdownPort:
    		srch2http::ShutdownCommand::runShutdown(req);
    		break;
    	case srch2http::NodeShutdownPort:
    		srch2http::ShardManager::getShardManager()->_shutdown();
    		break;
    	default:
    		cb_notfound(req, NULL);
    		break;
    	}
    } catch (exception& e){
    	Logger::error(e.what());
    	srch2http::Srch2ServerRuntime::handleException(req);
    }

}

void Srch2ServerGateway::cb_notfound(evhttp_request *req, void *arg)
{
	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");
	try {
		evhttp_send_reply(req, HTTP_NOTFOUND, "Not found", NULL);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::Srch2ServerRuntime::handleException(req);
	}
}

void Srch2ServerGateway::cb_wrongauthorizationkey(evhttp_request *req, void *arg)
{
    evhttp_add_header(req->output_headers, "Content-Type",
            "application/json; charset=UTF-8");
    try {
        evhttp_send_reply(req, HTTP_BADREQUEST, "Wrong authorization key", NULL);
    } catch (exception& e) {
        // exception caught
        Logger::error(e.what());
        Srch2ServerRuntime::handleException(req);
    }
}



bool Srch2ServerGateway::checkAuthorizationKey(Srch2ServerRuntime * runtime , evhttp_request *req){

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

bool Srch2ServerGateway::checkGlobalOperationPermission(Srch2ServerRuntime * runtime , evhttp_request *req) {
    if (checkAuthorizationKey(runtime, req) == false) {
        cb_wrongauthorizationkey(req, NULL);
        return false;
    }

    unsigned short arrivalPort = getLibeventHttpRequestPort(req);

    if (arrivalPort <= 0 ) {
        Logger::warn("Unable to ascertain arrival port from request headers.");
        return false;
    }
    // compare arrival port to globalDefaultPort
    if (runtime->getGlobalDefaultPort() != arrivalPort) {
        Logger::warn("/%s request for global operation arriving on port %d denied (port %d will permit)",
        		"",  arrivalPort, runtime->getGlobalDefaultPort() ); //TODO
        cb_notfound(req, NULL);
        return false;
    }

    return true;

}


bool Srch2ServerGateway::checkOperationPermission(Srch2ServerRuntime * runtime ,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		evhttp_request *req,
		unsigned coreId, srch2http::PortType_t portType) {

	ASSERT(coreId != (unsigned) -1);
	if(coreId == (unsigned) -1){
		return false;
	}

	if (checkAuthorizationKey(runtime, req) == false) {
		cb_wrongauthorizationkey(req, NULL);
		return false;
	}

	if (portType >= srch2http::EndOfPortType) {
		Logger::error("Illegal port type: %d", static_cast<int> (portType));
		cb_notfound(req, NULL);
		return false;
	}

	const srch2http::CoreInfo_t *const coreInfo = clusterReadview->getCore(coreId);

	unsigned short configuredPort = coreInfo->getPort(portType);
	if(configuredPort == 0){
		configuredPort = runtime->getGlobalDefaultPort();
	}
	unsigned short arrivalPort = getLibeventHttpRequestPort(req);

	if(arrivalPort == 0) {
		Logger::warn("Unable to ascertain arrival port from request headers.");
		return false;
	}

	// compare arrival port to configuration file port
	if(configuredPort != arrivalPort) {
		string coreName = coreInfo->getName();
		Logger::warn("/%s request for %s core arriving on port %d"
				" denied (port %d will permit)",
				ConfigManager::portNameMap[portType], coreName.c_str(),
				arrivalPort, configuredPort);

		return false;
	}
	return true;
}

unsigned short int Srch2ServerGateway::getLibeventHttpRequestPort(struct evhttp_request *req)
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

}
}
