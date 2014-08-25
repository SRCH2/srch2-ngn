#include "core/util/Assert.h"

#include <iostream>
#include <sstream>
#include <time.h>
#include <string>
#include <stdlib.h>

#include "sharding/sharding/metadata_manager/ResourceLocks.h"

using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;


typedef ClusterShardId R;
typedef NodeOperationId O;

class LockRepoInitializer{
public:

	LockRepoInitializer(LockHoldersRepository * repo = NULL){
		this->lockType = 'S';
		this->resource = R(0,0,0);
		this->repo = repo;
	}

	LockRepoInitializer& operator<<(char lockType){
		if(lockType != 'S' &&
				lockType != 'U' &&
				lockType != 'X' ){
			ASSERT(false);
			return *this;
		}
		this->lockType = lockType;
		return *this;
	}
	LockRepoInitializer& operator<<(R resource){
		this->resource = resource;
		return *this;
	}
	LockRepoInitializer& operator<<(O lockHolder){
		switch (this->lockType) {
			case 'S':
				if(repo->S_Holders.find(resource) == repo->S_Holders.end()){
					repo->S_Holders[resource] = vector<O>();
				}
				repo->S_Holders[resource].push_back(lockHolder);
				break;
			case 'U':
				if(repo->U_Holders.find(resource) == repo->U_Holders.end()){
					repo->U_Holders[resource] = vector<O>();
				}
				repo->U_Holders[resource].push_back(lockHolder);
				break;
			case 'X':
				if(repo->X_Holders.find(resource) == repo->X_Holders.end()){
					repo->X_Holders[resource] = vector<O>();
				}
				repo->X_Holders[resource].push_back(lockHolder);
				break;
		}

		return *this;
	}
	void setRepo(LockHoldersRepository * repo){
		ASSERT(repo != NULL);
		this->repo = repo;
	}
private:
	LockHoldersRepository * repo;
	char lockType;
	R resource;
};
