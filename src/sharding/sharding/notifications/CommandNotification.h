#ifndef __SHARDING_SHARDING_SHARDING_COMMAND_NOTIF_H_
#define __SHARDING_SHARDING_SHARDING_COMMAND_NOTIF_H_


#include "sharding/configuration/ShardingConstants.h"
#include "server/HTTPJsonResponse.h"
#include "Notification.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"

using namespace std;
namespace srch2 {
namespace httpwrapper {



class CommandNotification : public ShardingNotification{
public:

	CommandNotification(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			const NodeTargetShardInfo & target,
			ShardCommandCode commandCode = ShardCommandCode_Merge, const string & filePath = "");
	CommandNotification();
	~CommandNotification();


	bool resolveNotification(SP(ShardingNotification) _notif);

    ShardingMessageType messageType() const;
	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer) ;

	ShardCommandCode getCommandCode() const;

	string getJsonFilePath() const;

	string getNewLogFilePath() const;

	NodeTargetShardInfo getTarget();

	boost::shared_ptr<const ClusterResourceMetadata_Readview> getReadview() const;

private:
	NodeTargetShardInfo target;
	ShardCommandCode commandCode;

	// in case it's export
	string jsonFilePath;

	// in case it's resetLogger
	string newLogFilePath;
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
};


}}


#endif // __SHARDING_SHARDING_SHARDING_COMMAND_NOTIF_H_
