/*
 * ServerInterfaceInternal.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

//#include "Srch2Server.h"
#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include "ServerInterfaceInternal.h"

namespace adapter{
ServerInterfaceInternal::ServerInterfaceInternal(void *server, std::map<std::string, std::string> *connectorConfig) {
	this->server=(Srch2Server*)server;
	this->connectorConfig = connectorConfig;
}

int ServerInterfaceInternal::insertRecord(std::string jsonString) {
	std::cout << "ServerInterfaceInternal::insertRecord()" << std::endl;
}

int ServerInterfaceInternal::deleteRecord(std::string primaryKey) {
	std::cout << "ServerInterfaceInternal::deleteRecord()" << std::endl;
}

int ServerInterfaceInternal::updateRecord(std::string jsonString) {
	std::cout << "ServerInterfaceInternal::updateRecord()" << std::endl;
}
}
