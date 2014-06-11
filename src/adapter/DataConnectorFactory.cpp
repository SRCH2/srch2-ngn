/*
 * DataConnectorFactory.cpp
 *
 *  Created on: Jun 10, 2014
 *      Author: liusrch2
 */

#include <dlfcn.h>
#include "DataConnectorFactory.h"
#include <iostream>
#include <string>


DataConnector* DataConnectorFactory::getDataConnector(
		std::string dbType) {
	std::string libName = DB_CONNECTORS_PATH + DYNAMIC_LIBRARY_PREFIX + dbType
			+ DYNAMIC_LIBRARY_SUFFIX;
	void *pdlHandle = dlopen(libName.c_str(), RTLD_NOW);
	if (!pdlHandle) {
		std::cout << "Fail to load " << libName << std::endl;
		return 0;
	}
	char *err = dlerror();
	if (err) {
		std::cout << "Fail to load " << libName << " due to " << err
				<< std::endl;
		return 0;
	}
	DataConnector* getNewInstance = (DataConnector*) dlsym(pdlHandle,
			"getNewInstance");
	if (!getNewInstance) {
		std::cout << "There is no \"getNewInstance\" function in " << libName
				<< std::endl;
		return 0;
	}
	return getNewInstance;
}

