#include "CommitNotification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


CommitNotification::CommitNotification(MetadataChange * metadataChange){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
}

MetadataChange * CommitNotification::getMetadataChange() const{
	return this->metadataChange;
}


//Returns the type of message which uses this kind of object as transport
ShardingMessageType CommitNotification::messageType() const{
	return ShardingCommitMessageType;
}

}
}
