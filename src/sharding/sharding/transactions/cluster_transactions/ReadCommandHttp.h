#ifndef __SHARDING_SHARDING_READ_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_READ_COMMAND_HTTP_H__

#include "../../metadata_manager/Shard.h"
#include "../../state_machine/ConsumerProducer.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"
#include "ReadCommand.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * contains the state of write operation per partition
 */
class ReadCommandHttp : public ReadviewTransaction, public ConsumerInterface{
public:
	static void search(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId){
		SP(ReadCommandHttp) readCommand =SP(ReadCommandHttp)(new ReadCommandHttp(clusterReadview, req, coreId));
		Transaction::startTransaction(readCommand);
	}

	static void searchAll(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req){
		SP(ReadCommandHttp) readCommand =SP(ReadCommandHttp)(new ReadCommandHttp(clusterReadview, req));
		Transaction::startTransaction(readCommand);
	}
	~ReadCommandHttp(){
		finalize();
		for(map<unsigned , CoreReadCommandInfo *>::iterator coreReadCmdItr = coreReadCommands.begin();
				coreReadCmdItr != coreReadCommands.end(); ++coreReadCmdItr){
			delete coreReadCmdItr->second;
		}
		coreReadCommands.clear();
	}
private:
	struct CoreReadCommandInfo{
		CoreReadCommandInfo(){
			this->readCommand = NULL;
		}
		const CoreInfo_t * coreInfo;
		ReadCommand * readCommand;
		ParsedParameterContainer paramContainer;
	};
	ReadCommandHttp(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId):ReadviewTransaction(clusterReadview){
		CoreReadCommandInfo * coreReadCommand = new CoreReadCommandInfo();
		coreReadCommand->coreInfo = clusterReadview->getCore(coreId);
		coreReadCommands[coreId] = coreReadCommand;
		this->clusterReadview = this->getReadview();
		this->req = req;
	}
	ReadCommandHttp(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req):ReadviewTransaction(clusterReadview){
		// search all case
		vector<const CoreInfo_t *> cores;
		this->clusterReadview->getAllCores(cores);
		for(unsigned i = 0 ; i < cores.size(); ++i){
			CoreReadCommandInfo * coreReadCommand = new CoreReadCommandInfo();
			coreReadCommand->coreInfo = cores.at(i);
			coreReadCommands[cores.at(i)->getCoreId()] = coreReadCommand;
		}
		this->clusterReadview = this->getReadview();
		this->req = req;
		this->response = this->getSession()->response;
	}

	void init(){
		ReadviewTransaction::init();
		this->response = this->getSession()->response;
	}
	void run(){
		for(map<unsigned , CoreReadCommandInfo *>::iterator coreReadCmdItr = coreReadCommands.begin();
				coreReadCmdItr != coreReadCommands.end(); ++coreReadCmdItr){
			parse(coreReadCmdItr->second);
		}
	}


	void parse(CoreReadCommandInfo * coreReadCommandInfo){
		if(coreReadCommandInfo == NULL){
			return;
		}
		if(coreReadCommandInfo->coreInfo == NULL){
			response->addError(JsonResponseHandler::getJsonSingleMessageStr(HTTP_JSON_Core_Does_Not_Exist));
			response->finalizeOK();
			return;
		}
		// parse and populate the paramContainer
	    evhttp_parse_query(req->uri, &headers);


	    if(coreReadCommandInfo->coreInfo->getHasRecordAcl()){
	    	coreReadCommandInfo->paramContainer.hasRoleCore = true;
	    }
	    // simple example for query is : q={boost=2}name:foo~0.5 AND bar^3*&fq=name:"John"
	    //1. first create query parser to parse the url
	    QueryParser qp(headers, &(coreReadCommandInfo->paramContainer));
	    bool isSyntaxValid = qp.parse(coreReadCommandInfo->coreInfo->getSchema());
	    if (!isSyntaxValid) {
	        // if the query is not valid print the error message to the response
	        this->response->finalizeError(coreReadCommandInfo->paramContainer.getMessageString());
	        return;
	    }

		// give parameters to ReadCommand to perform the search
	    coreReadCommandInfo->readCommand = new ReadCommand(coreReadCommandInfo->coreInfo, coreReadCommandInfo->paramContainer, this);
	    coreReadCommandInfo->readCommand->produce();
	}


	void consume(unsigned coreId, bool booleanResult, SP(Json::Value) jsonResponse ,
				const vector<JsonMessageCode> & messageCodes, const vector<string> & customMessageStrings){
		if(coreReadCommands.find(coreId) == coreReadCommands.end()){
			return;
		}
		CoreReadCommandInfo * coreReadCommandInfo = coreReadCommands.find(coreId)->second;
		for(vector<JsonMessageCode>::const_iterator msgItr = messageCodes.begin(); msgItr != messageCodes.end(); ++msgItr){
			this->response->addMessage(JsonResponseHandler::getJsonSingleMessageStr(*msgItr));
		}
		for(vector<string>::const_iterator msgItr = customMessageStrings.begin(); msgItr != customMessageStrings.end(); ++msgItr){
			this->response->addMessage(*msgItr);
		}
		this->response->addMessage(coreReadCommandInfo->paramContainer.getMessageString());
		this->response->setResponseAttribute(c_status, Json::Value(booleanResult));
		// and finally, write the results to the response
		this->response->setRoot(jsonResponse.get());
	    Logger::info(
	            "ip: %s, port: %d GET query: %s, searcher_time: %d ms, payload_access_time: %d ms",
	            req->remote_host, req->remote_port, req->uri + 1,
	            coreReadCommandInfo->readCommand->getTotalSearchTime(), coreReadCommandInfo->readCommand->getPayloadAccessTime());
		// The end.
		// After this, finalizeWork will eventually be called in the time of destruction
	}


	void finalizeWork(Transaction::Params * arg){
		response->printHTTP(req, &headers);
	    evhttp_clear_headers(&headers);
	}

	ShardingTransactionType getTransactionType(){
		return ShardingTransactionType_ReadCommand;
	}

	SP(Transaction) getTransaction(){
		return sharedPointer;
	}
	string getName() const {
		return "read-command-http";
	}

private:
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	map<unsigned , CoreReadCommandInfo *> coreReadCommands;
	evhttp_request *req;
	evkeyvalq headers;
	JsonResponseHandler * response;
};

}
}

#endif // __SHARDING_SHARDING_READ_COMMAND_HTTP_H__
