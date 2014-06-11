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


ServerInterfaceInternal::ServerInterfaceInternal(Srch2Server *server, std::map<string, string> *connectorConfig) {
	this->server=server;
	this->connectorConfig = connectorConfig;
}

int ServerInterfaceInternal::insertRecord(string jsonString) {
	std::cout << "ServerInterfaceInternal::insertRecord()" << std::endl;
}

int ServerInterfaceInternal::deleteRecord(string primaryKey) {
	std::cout << "ServerInterfaceInternal::deleteRecord()" << std::endl;
}

int ServerInterfaceInternal::updateRecord(string jsonString) {
	std::cout << "ServerInterfaceInternal::updateRecord()" << std::endl;
}

