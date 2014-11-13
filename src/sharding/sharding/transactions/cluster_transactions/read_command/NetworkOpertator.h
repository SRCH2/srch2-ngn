#ifndef NETWORKOPERTATOR_H_
#define NETWORKOPERTATOR_H_

#include "ClusterPhysicalOperators.h"
#include "../../../notifications/Notification.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

class NetworkOperator : public ClusterPhysicalOperator{
public:
	NetworkOperator(){
	}
	bool open(ClusterPhysicalPlanExecutionParameter * params);
	QueryResult * getNext(ClusterPhysicalPlanExecutionParameter * params);
	bool close(ClusterPhysicalPlanExecutionParameter * params);
	void load(srch2is::QueryResults* queryResults);
private:
	std::vector<QueryResult *>  queryResults;
	std::vector<QueryResult *>::iterator it;
};


}
}


#endif /* NETWORKOPERTATOR_H_ */
