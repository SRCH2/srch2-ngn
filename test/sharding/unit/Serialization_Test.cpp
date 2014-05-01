#include <iostream>
#include <string>
#include <stdlib.h>
#include "util/Assert.h"
#include <assert.h>

#include "src/sharding/configuration/ConfigManager.h"

#include "src/sharding/processor/serializables/SerializableCommandStatus.h"
#include "src/sharding/processor/serializables/SerializableCommitCommandInput.h"
#include "src/sharding/processor/serializables/SerializableDeleteCommandInput.h"
#include "src/sharding/processor/serializables/SerializableGetInfoCommandInput.h"
#include "src/sharding/processor/serializables/SerializableGetInfoResults.h"
#include "src/sharding/processor/serializables/SerializableInsertUpdateCommandInput.h"
#include "src/sharding/processor/serializables/SerializableResetLogCommandInput.h"
#include "src/sharding/processor/serializables/SerializableSearchCommandInput.h"
#include "src/sharding/processor/serializables/SerializableSearchResults.h"
#include "src/sharding/processor/serializables/SerializableSerializeCommandInput.h"


using srch2http::ConfigManager;
using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

void testSerializableCommandStatus(){
//	SerializableCommandStatus commandStatus1(SerializableCommandStatus::INSERT, true, "INSERT True");
//	void * buffer = commandStatus1.serialize(allocator<char>());
//	const SerializableCommandStatus & deserializedCommandStatus1 = SerializableCommandStatus::deserialize(buffer);
//	ASSERT(commandInput1.getCommandCode() == deserializedCommandStatus1.getCommandCode());
//	ASSERT(commandInput1.getStatus() == deserializedCommandStatus1.getStatus());
//	ASSERT(commandInput1.getMessage().compare(deserializedCommandStatus1.getMessage()) == 0);
//
//	SerializableCommandStatus commandStatus2(SerializableCommandStatus::DELETE, false, "Delete false");
//	void * buffer = commandStatus2.serialize(allocator<char>());
//	const SerializableCommandStatus & deserializedCommandStatus2 = SerializableCommandStatus::deserialize(buffer);
//	ASSERT(commandStatus2.getCommandCode() == deserializedCommandStatus2.getCommandCode());
//	ASSERT(commandStatus2.getStatus() == deserializedCommandStatus2.getStatus());
//	ASSERT(commandStatus2.getMessage().compare(deserializedCommandStatus2.getMessage()) == 0);
}

void testSerializableCommitCommandInput(){

	// This class is empty for now
}

void testSerializableDeleteCommandInput(){
//	SerializableDeleteCommandInput commandInput1("primary key 1",1);
//	void * buffer = commandInput1.serialize(allocator<char>());
//	const SerializableDeleteCommandInput & deserializedCommandInput1 = SerializableDeleteCommandInput::deserialize(buffer);
//	ASSERT(commandInput1.getPrimaryKey() == deserializedCommandInput1.getPrimaryKey());
//	ASSERT(commandInput1.getShardingKey() == deserializedCommandInput1.getShardingKey());
//
//	SerializableDeleteCommandInput commandInput1("primary key 3",-1);
//	void * buffer = commandInput1.serialize(allocator<char>());
//	const SerializableDeleteCommandInput & deserializedCommandInput1 = SerializableDeleteCommandInput::deserialize(buffer);
//	ASSERT(commandInput1.getPrimaryKey() == deserializedCommandInput1.getPrimaryKey());
//	ASSERT(commandInput1.getShardingKey() == deserializedCommandInput1.getShardingKey());
}

void testSerializableGetInfoCommandInput(){
	// This class is empty for now
}

void testSerializableGetInfoResults(){
//	SerializableGetInfoResults commandInput1(10, 20, 30, "yesterday", 40, "version 3.4.1");
//	void * buffer = commandInput1.serialize(allocator<char>());
//	const SerializableGetInfoResults & deserializedCommandInput1 = SerializableGetInfoResults::deserialize(buffer);
//	ASSERT(commandInput1.getReadCount() == deserializedCommandInput1.getReadCount());
//	ASSERT(commandInput1.getWriteCount() == deserializedCommandInput1.getWriteCount());
//	ASSERT(commandInput1.getDocCount() == deserializedCommandInput1.getDocCount());
//	ASSERT(commandInput1.getLastMergeTimeString() == deserializedCommandInput1.getLastMergeTimeString());
//	ASSERT(commandInput1.getNumberOfDocumentsInIndex() == deserializedCommandInput1.getNumberOfDocumentsInIndex());
//	ASSERT(commandInput1.getVersionInfo() == deserializedCommandInput1.getVersionInfo());
//
//
//	SerializableGetInfoResults commandInput2(11, 2, 300, "tomorrow", 4, "version 3.4.2");
//	void * buffer = commandInput2.serialize(allocator<char>());
//	const SerializableGetInfoResults & deserializedCommandInput2 = SerializableGetInfoResults::deserialize(buffer);
//	ASSERT(commandInput2.getReadCount() == deserializedCommandInput2.getReadCount());
//	ASSERT(commandInput2.getWriteCount() == deserializedCommandInput2.getWriteCount());
//	ASSERT(commandInput2.getDocCount() == deserializedCommandInput2.getDocCount());
//	ASSERT(commandInput2.getLastMergeTimeString() == deserializedCommandInput2.getLastMergeTimeString());
//	ASSERT(commandInput2.getNumberOfDocumentsInIndex() == deserializedCommandInput2.getNumberOfDocumentsInIndex());
//	ASSERT(commandInput2.getVersionInfo() == deserializedCommandInput2.getVersionInfo());
}

void testSerializableInsertUpdateCommandInput(){
//	Record * record1 = new Record(NULL);
//	SerializableInsertUpdateCommandInput commandInput1(record1, SerializableInsertUpdateCommandInput::INSERT);
//	void * buffer = commandInput1.serialize(allocator<char>());
//	const SerializableInsertUpdateCommandInput & deserializedCommandInput1 = SerializableInsertUpdateCommandInput::deserialize(buffer);
}

void testSerializableResetLogCommandInput(){

}

void testSerializableSearchCommandInput(){

}

void testSerializableSearchResults(){

}

void testSerializableSerializeCommandInput(){

}

int main(){
	testSerializableCommandStatus();
	testSerializableCommitCommandInput();
	testSerializableDeleteCommandInput();
	testSerializableGetInfoCommandInput();
	testSerializableGetInfoResults();
	testSerializableInsertUpdateCommandInput();
	testSerializableResetLogCommandInput();
	testSerializableSearchCommandInput();
	testSerializableSearchResults();
	testSerializableSerializeCommandInput();

    cout << "Sharding Serialization unit tests: Passed" << endl;
}
