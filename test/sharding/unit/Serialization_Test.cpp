#include <iostream>
#include <string>
#include <stdlib.h>
#include "util/Assert.h"
#include <assert.h>

#include "src/sharding/configuration/ConfigManager.h"

#include "sharding/processor/serializables/SerializableCommandStatus.h"
#include "sharding/processor/serializables/SerializableCommitCommandInput.h"
#include "sharding/processor/serializables/SerializableDeleteCommandInput.h"
#include "sharding/processor/serializables/SerializableGetInfoCommandInput.h"
#include "sharding/processor/serializables/SerializableGetInfoResults.h"
#include "sharding/processor/serializables/SerializableInsertUpdateCommandInput.h"
#include "sharding/processor/serializables/SerializableResetLogCommandInput.h"
#include "sharding/processor/serializables/SerializableSearchCommandInput.h"
#include "sharding/processor/serializables/SerializableSearchResults.h"
#include "sharding/processor/serializables/SerializableSerializeCommandInput.h"


using srch2http::ConfigManager;
using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

void testSerializableCommandStatus(){
	SerializableCommandStatus commandStatus1(SerializableCommandStatus::INSERT, true, "INSERT True");
	void * buffer = commandStatus1.serialize(allocator<char>());
	const SerializableCommandStatus & deserializedCommandStatus1 = SerializableCommandStatus::deserialize(buffer);
	ASSERT(commandStatus1.getCommandCode() == deserializedCommandStatus1.getCommandCode());
	ASSERT(commandStatus1.getStatus() == deserializedCommandStatus1.getStatus());
	ASSERT(commandStatus1.getMessage().compare(deserializedCommandStatus1.getMessage()) == 0);
}

void testSerializableCommitCommandInput(){

}

void testSerializableDeleteCommandInput(){

}

void testSerializableGetInfoCommandInput(){

}

void testSerializableGetInfoResults(){

}

void testSerializableInsertUpdateCommandInput(){

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
