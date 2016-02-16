/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
		this->clusterReadview = this->getReadview();
		vector<const CoreInfo_t *> cores;
		this->clusterReadview->getAllCores(cores);
		for(unsigned i = 0 ; i < cores.size(); ++i){
			CoreReadCommandInfo * coreReadCommand = new CoreReadCommandInfo();
			coreReadCommand->coreInfo = cores.at(i);
			coreReadCommands[cores.at(i)->getCoreId()] = coreReadCommand;
		}
		this->req = req;
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

	    if (coreReadCommandInfo->coreInfo->isUserFeedbackEnabled()) {
	    	// set only if user feedback is enabled else leave it empty.
	    	coreReadCommandInfo->paramContainer.queryStringWithTermsAndOpsOnly = qp.fetchCleanQueryString();
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
		// core name :

		JsonResponseHandler * coreResponse = NULL;
		if(coreReadCommands.size() == 1){
			coreResponse = this->response;
		}else{
			coreResponse = new JsonResponseHandler();
		}
		coreResponse->setRoot(jsonResponse.get());
		for(vector<JsonMessageCode>::const_iterator msgItr = messageCodes.begin(); msgItr != messageCodes.end(); ++msgItr){
			coreResponse->addMessage(JsonResponseHandler::getJsonSingleMessageStr(*msgItr));
		}
		for(vector<string>::const_iterator msgItr = customMessageStrings.begin(); msgItr != customMessageStrings.end(); ++msgItr){
			coreResponse->addMessage(*msgItr);
		}
		coreResponse->addMessage(coreReadCommandInfo->paramContainer.getMessageString());
		coreResponse->setResponseAttribute(c_status, Json::Value(booleanResult));
		// and finally, write the results to the response
		if(coreReadCommands.size() > 1){
			this->response->setResponseAttribute(coreReadCommandInfo->coreInfo->getName().c_str(), coreResponse->getRoot());
			delete coreResponse;
		}
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
