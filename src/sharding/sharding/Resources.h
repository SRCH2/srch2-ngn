#ifndef __SHARDING_SHARDING_RESOURCES_H__
#define __SHARDING_SHARDING_RESOURCES_H__

#include "core/util/Assert.h"
#include "core/util/SerializationHelper.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class ShardingChange{
public:
	ShardingChange(){
		this->changeId = 0;
	}
	ShardingChange(const ShardingChange & change){
		this->changeId = change.changeId;
	}
	virtual ~ShardingChange(){

	}
	unsigned getChangeId(){
		return this->changeId;
	}
	void setChangeId(unsigned cid){
		this->changeId = cid;
	}
	void * serialize(void * buffer) const{
		return srch2::util::serializeFixedTypes(changeId, buffer);
	}
	unsigned getNumberOfBytes() const{
		return sizeof(unsigned); // changeID
	}
	void * deserialize(void * buffer) const{
		return srch2::util::deserializeFixedTypes(buffer, changeId);
	}
	virtual ShardingChange * clone() = 0;

private:
	unsigned changeId;
};


}
}

#endif //__SHARDING_SHARDING_RESOURCES_H__
