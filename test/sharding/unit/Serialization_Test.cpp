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

#include "test/core/unit/UnitTestHelper.h"

using srch2http::ConfigManager;
using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

void initializeQueryResult(unsigned internalRecordId, double physicalDistance,
		TypedValue score, string externalRecordId, vector<unsigned> attributeBitmaps,
		vector<unsigned> editDistances, vector<TermType> termTypes , vector<string> matchingKeywords,  QueryResult * queryResult){
	queryResult->internalRecordId = internalRecordId;
	queryResult->physicalDistance = physicalDistance;
	queryResult->_score = score;
	queryResult->externalRecordId = externalRecordId;
	queryResult->attributeIdsList.push_back(attributeBitmaps);
	queryResult->editDistances = editDistances;
	queryResult->termTypes = termTypes;
	queryResult->matchingKeywords = matchingKeywords;
}

void checkQueryResultContent(QueryResult * obj1, QueryResult * obj2){
	ASSERT(obj1 != NULL || obj1 == obj2);
	ASSERT(obj2 != NULL || obj1 == obj2);
	if(obj1 == NULL){
		return;
	}
	ASSERT(obj1->internalRecordId == obj2->internalRecordId);
	ASSERT(obj1->physicalDistance == obj2->physicalDistance);
	ASSERT(obj1->_score == obj2->_score);
	ASSERT(obj1->externalRecordId == obj2->externalRecordId);
	ASSERT(obj1->attributeIdsList.size() == obj2->attributeIdsList.size());
	for(unsigned i=0; i<obj1->attributeIdsList.size(); ++i ){
		ASSERT(obj1->attributeIdsList.at(i).size() == obj2->attributeIdsList.at(i).size());
	}
	ASSERT(obj1->editDistances.size() == obj2->editDistances.size());
	for(unsigned i=0; i<obj1->editDistances.size(); ++i ){
		ASSERT(obj1->editDistances.at(i) == obj2->editDistances.at(i) );
	}
	ASSERT(obj1->termTypes.size() == obj2->editDistances.size());
	for(unsigned i=0; i<obj1->termTypes.size(); ++i ){
		ASSERT(obj1->termTypes.at(i) == obj2->termTypes.at(i) );
	}
	ASSERT(obj1->matchingKeywords.size() == obj2->matchingKeywords.size());
	for(unsigned i=0; i<obj1->matchingKeywords.size(); ++i ){
		ASSERT(obj1->matchingKeywords.at(i) == obj2->matchingKeywords.at(i) );
	}
}

void checkFacetResults(QueryResults * results1, QueryResults * results2){
	ASSERT(results1 != NULL || results1 == results2);
	if(results1 == NULL){
		return;
	}
	ASSERT(results1->impl->facetResults.size() == results2->impl->facetResults.size());
	for(unsigned categoryIndex =0 ; categoryIndex < results1->impl->facetResults.size() ; ++categoryIndex){
		stringstream ss ;
		ss << categoryIndex;
		ASSERT(results1->impl->facetResults.at("category number "+ss.str()).first ==
				results2->impl->facetResults.at("category number "+ss.str()).first);
		ASSERT(results1->impl->facetResults.at("category number "+ss.str()).second.size() ==
				results2->impl->facetResults.at("category number "+ss.str()).second.size());
		for(unsigned facetIndex = 0;
				facetIndex < results1->impl->facetResults.at("category number " + ss.str()).second.size(); ++facetIndex ){
			ASSERT(results1->impl->facetResults.at("category number "+ss.str()).second.at(facetIndex).first ==
					results2->impl->facetResults.at("category number "+ss.str()).second.at(facetIndex).first);
			ASSERT(results1->impl->facetResults.at("category number "+ss.str()).second.at(facetIndex).second ==
					results2->impl->facetResults.at("category number "+ss.str()).second.at(facetIndex).second);
		}
	}
}

QueryResults * prepareQueryResults(){

    Query *query = new Query(srch2is::SearchTypeTopKQuery);
    string keywords[3] = { "pink", "floyd", "shine"};

    TermType termType = TERM_TYPE_COMPLETE;
    Term *term0 = ExactTerm::create(keywords[0], termType, 1, 1);
    query->add(term0);
    Term *term1 = ExactTerm::create(keywords[1], termType, 1, 1);
    query->add(term1);
    Term *term2 = FuzzyTerm::create(keywords[2],termType, 0.5, 0.5, 2);
    query->add(term2);

    QueryResultFactory * factory = new QueryResultFactory();

    QueryResults *queryResults = new QueryResults(factory,NULL, query);

    // prepare query results
    vector<QueryResult *> sourceQueryResults;
    for(int i=0; i < 10; i++){
		stringstream ss;
		ss << i;
    	QueryResult * queryResult = factory->impl->createQueryResult();
    	vector<unsigned> attributeBitmaps;
    	vector<unsigned> editDistances;
        vector<TermType> termTypes;
		std::vector<std::string> matchingKeywords;
    	for(int j=0; j < i; j++){
    		attributeBitmaps.push_back(j*100);
    		editDistances.push_back(i*100);
    		termTypes.push_back(i%2 == 0 ? TERM_TYPE_COMPLETE : TERM_TYPE_PREFIX);
    		matchingKeywords.push_back("matching keyword " + ss.str());
    	}
    	TypedValue score;
    	score.setTypedValue((float)10.4567, ATTRIBUTE_TYPE_FLOAT);
    	initializeQueryResult(i , 10.5*i, score , "external record id" + ss.str() ,
    			attributeBitmaps, editDistances,termTypes,matchingKeywords, queryResult);
    	queryResults->impl->sortedFinalResults.push_back(queryResult);
    }

    // prepare facet results
    std::map<std::string , std::pair< FacetType , std::vector<std::pair<std::string, float> > > >  * facetContainer =
    		&(queryResults->impl->facetResults);
    for(unsigned categoryIndex = 0 ; categoryIndex < 10; categoryIndex ++){
    	stringstream categoryIndexString ;
    	categoryIndexString << categoryIndex;
    	std::vector<std::pair<std::string, float> > facetVector;
    	for(unsigned facetIndex = 0; facetIndex < categoryIndex; facetIndex ++){
    		stringstream ss;
    		ss << facetIndex;
    		facetVector.push_back(std::make_pair("facet number " + ss.str(), facetIndex*10));
    	}
    	(*facetContainer)["category number "+categoryIndexString.str()] = std::make_pair(FacetTypeCategorical , facetVector);
    }

    // prepare other members
    queryResults->impl->estimatedNumberOfResults = 23;
    queryResults->impl->resultsApproximated = true;

    return queryResults;
}


void testSerializableCommandStatus(){
	CommandStatus commandInput1(CommandStatus::DP_INSERT);
	CommandStatus::ShardResults* shardResult = new CommandStatus::ShardResults("identifier");
	shardResult->statusValue =  true;
	shardResult->messages.append(Json::Value("INSERT True"));
	commandInput1.addShardResult(shardResult);

	MessageAllocator * aloc = new MessageAllocator();
	void * buffer = commandInput1.serialize(aloc);
	const CommandStatus & deserializedCommandInput1 = *(CommandStatus::deserialize(buffer));
	ASSERT(commandInput1.getCommandCode() == deserializedCommandInput1.getCommandCode());
	ASSERT(commandInput1.getShardResults().at(0)->statusValue == deserializedCommandInput1.getShardResults().at(0)->statusValue);
	ASSERT(commandInput1.getShardResults().at(0)->messages == deserializedCommandInput1.getShardResults().at(0)->messages);

	CommandStatus commandInput2(CommandStatus::DP_DELETE);
	CommandStatus::ShardResults* shardResult2 = new CommandStatus::ShardResults("identifier2");
	shardResult2->statusValue =  true;
	shardResult->messages.append(Json::Value("INSERT True"));
	commandInput2.addShardResult(shardResult2);
	buffer = commandInput2.serialize(aloc);
	const CommandStatus & deserializedCommandInput2 = *(CommandStatus::deserialize(buffer));
	ASSERT(commandInput2.getCommandCode() == deserializedCommandInput2.getCommandCode());
	ASSERT(commandInput2.getShardResults().at(0)->statusValue == deserializedCommandInput2.getShardResults().at(0)->statusValue);
	ASSERT(commandInput2.getShardResults().at(0)->messages == deserializedCommandInput2.getShardResults().at(0)->messages);
}

void testSerializableCommitCommandInput(){

	// This class is empty for now
}

void testSerializableDeleteCommandInput(){
	DeleteCommand commandInput1("primary key 1",1);
	MessageAllocator * aloc = new MessageAllocator();
	void * buffer = commandInput1.serialize(aloc);
	const DeleteCommand & deserializedCommandInput1 = *(DeleteCommand::deserialize(buffer));
	ASSERT(commandInput1.getPrimaryKey() == deserializedCommandInput1.getPrimaryKey());
	ASSERT(commandInput1.getShardingKey() == deserializedCommandInput1.getShardingKey());

	DeleteCommand commandInput2("primary key 3",-1);
	buffer = commandInput2.serialize(aloc);
	const DeleteCommand & deserializedCommandInput2 = *(DeleteCommand::deserialize(buffer));
	ASSERT(commandInput2.getPrimaryKey() == deserializedCommandInput2.getPrimaryKey());
	ASSERT(commandInput2.getShardingKey() == deserializedCommandInput2.getShardingKey());
}

void testSerializableGetInfoCommandInput(){
	// This class is empty for now
}

void testSerializableGetInfoResults(){
	GetInfoCommandResults commandInput1;
	GetInfoCommandResults::ShardResults * shardResults = new GetInfoCommandResults::ShardResults(new ClusterShardId(1,2,4),10, 20, "yesterday", 40, "version 3.4.1" , false, true);
	commandInput1.addShardResults(shardResults);
	MessageAllocator * aloc = new MessageAllocator();
	void * buffer = commandInput1.serialize(aloc);
	const GetInfoCommandResults & deserializedCommandInput1 = *(GetInfoCommandResults::deserialize(buffer));
	ASSERT(commandInput1.getShardResults().at(0)->healthInfo.readCount == deserializedCommandInput1.getShardResults().at(0)->healthInfo.readCount);
	ASSERT(commandInput1.getShardResults().at(0)->healthInfo.writeCount == deserializedCommandInput1.getShardResults().at(0)->healthInfo.writeCount);
	ASSERT(commandInput1.getShardResults().at(0)->healthInfo.docCount == deserializedCommandInput1.getShardResults().at(0)->healthInfo.docCount);
	ASSERT(commandInput1.getShardResults().at(0)->healthInfo.lastMergeTimeString == deserializedCommandInput1.getShardResults().at(0)->healthInfo.lastMergeTimeString);
	ASSERT(commandInput1.getShardResults().at(0)->healthInfo.isMergeRequired == deserializedCommandInput1.getShardResults().at(0)->healthInfo.isMergeRequired);
	ASSERT(commandInput1.getShardResults().at(0)->healthInfo.isBulkLoadDone == deserializedCommandInput1.getShardResults().at(0)->healthInfo.isBulkLoadDone);
	ASSERT(commandInput1.getShardResults().at(0)->versionInfo == deserializedCommandInput1.getShardResults().at(0)->versionInfo);


	GetInfoCommandResults commandInput2;
	GetInfoCommandResults::ShardResults * shardResults2 = new GetInfoCommandResults::ShardResults(new ClusterShardId(1,2,4),11, 2, "tomorrow", 4, "version 3.4.2" , false, false);
	commandInput2.addShardResults(shardResults2);
	buffer = commandInput2.serialize(aloc);
	const GetInfoCommandResults & deserializedCommandInput2 = *(GetInfoCommandResults::deserialize(buffer));
	ASSERT(commandInput2.getShardResults().at(0)->healthInfo.readCount == deserializedCommandInput2.getShardResults().at(0)->healthInfo.readCount);
	ASSERT(commandInput2.getShardResults().at(0)->healthInfo.writeCount == deserializedCommandInput2.getShardResults().at(0)->healthInfo.writeCount);
	ASSERT(commandInput2.getShardResults().at(0)->healthInfo.docCount == deserializedCommandInput2.getShardResults().at(0)->healthInfo.docCount);
	ASSERT(commandInput2.getShardResults().at(0)->healthInfo.lastMergeTimeString == deserializedCommandInput2.getShardResults().at(0)->healthInfo.lastMergeTimeString);
	ASSERT(commandInput2.getShardResults().at(0)->healthInfo.isMergeRequired == deserializedCommandInput2.getShardResults().at(0)->healthInfo.isMergeRequired);
	ASSERT(commandInput2.getShardResults().at(0)->healthInfo.isBulkLoadDone == deserializedCommandInput2.getShardResults().at(0)->healthInfo.isBulkLoadDone);
	ASSERT(commandInput2.getShardResults().at(0)->versionInfo == deserializedCommandInput2.getShardResults().at(0)->versionInfo);
}

void testSerializableInsertUpdateCommandInput(){
    /// Create a Schema
	srch2is::SchemaInternal *schema = dynamic_cast<srch2is::SchemaInternal*>(srch2is::Schema::create(srch2is::DefaultIndex));
	schema->setPrimaryKey("primaryKey"); // integer, not searchable
	schema->setSearchableAttribute("description", 2); // searchable text

	Record *record1 = new Record(schema);
    record1->setPrimaryKey("primary key 1");
    record1->setSearchableAttributeValue(0, "the data for primary key 1");

	WriteCommandNotification commandInput1(record1, WriteCommandNotification::DP_INSERT);
	MessageAllocator * aloc = new MessageAllocator();
	void * buffer = commandInput1.serialize(aloc);
	const WriteCommandNotification & deserializedCommandInput1 = *(WriteCommandNotification::deserialize(buffer, schema));
	ASSERT(commandInput1.getInsertOrUpdate() == deserializedCommandInput1.getInsertOrUpdate());
	ASSERT(commandInput1.getRecord()->getPrimaryKey() == deserializedCommandInput1.getRecord()->getPrimaryKey());
	string originalValue, deserializedValue;
	commandInput1.getRecord()->getSearchableAttributeValue(0,originalValue);
	deserializedCommandInput1.getRecord()->getSearchableAttributeValue(0,deserializedValue);
	ASSERT(originalValue == deserializedValue);


	srch2is::SchemaInternal *schema2 = dynamic_cast<srch2is::SchemaInternal*>(srch2is::Schema::create(srch2is::DefaultIndex));
	schema2->setPrimaryKey("primaryKey"); // integer, not searchable
	schema2->setSearchableAttribute("description", 2); // searchable text
	schema2->setSearchableAttribute("body", 3); // searchable text

	Record *record2 = new Record(schema);
    record2->setPrimaryKey("primary key 1");
    record2->setSearchableAttributeValue(0, "the data for primary key 1");
    record2->setSearchableAttributeValue(1, "the body for primary key 1");


	WriteCommandNotification commandInput2(record2, WriteCommandNotification::DP_UPDATE);
	MessageAllocator * aloc2 = new MessageAllocator();
	void * buffer2 = commandInput2.serialize(aloc2);
	const WriteCommandNotification & deserializedCommandInput2 = *(WriteCommandNotification::deserialize(buffer2, schema2));
	ASSERT(commandInput2.getInsertOrUpdate() == deserializedCommandInput2.getInsertOrUpdate());
	ASSERT(commandInput2.getRecord()->getPrimaryKey() == deserializedCommandInput2.getRecord()->getPrimaryKey());
	commandInput2.getRecord()->getSearchableAttributeValue(0,originalValue);
	deserializedCommandInput2.getRecord()->getSearchableAttributeValue(0,deserializedValue);
	ASSERT(originalValue == deserializedValue);
	commandInput2.getRecord()->getSearchableAttributeValue(1,originalValue);
	deserializedCommandInput2.getRecord()->getSearchableAttributeValue(1,deserializedValue);
	ASSERT(originalValue == deserializedValue);

}

void testSerializableResetLogCommandInput(){
	// This class is empty for now
}

void testSerializableSearchCommandInput(){

    Query *query = new Query(srch2is::SearchTypeTopKQuery);
    string keywords[3] = { "pink", "floyd", "shine"};

    TermType termType = TERM_TYPE_COMPLETE;
    Term *term0 = ExactTerm::create(keywords[0], termType, 1, 1);
    query->add(term0);
    Term *term1 = ExactTerm::create(keywords[1], termType, 1, 1);
    query->add(term1);
    Term *term2 = FuzzyTerm::create(keywords[2],termType, 0.5, 0.5, 2);
    query->add(term2);

	LogicalPlan * logicalPlan = prepareLogicalPlanForUnitTests(query , NULL, 0, 10, false, srch2::instantsearch::SearchTypeTopKQuery);

	SearchCommand commandInput1(logicalPlan);
	MessageAllocator * aloc = new MessageAllocator();
	void * buffer = commandInput1.serialize(aloc);
	const SearchCommand & deserializedCommandInput1 = *(SearchCommand::deserialize(buffer));
	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(0)->getBoost() == term0->getBoost());
	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(1)->getBoost() == term1->getBoost());
	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(2)->getBoost() == term2->getBoost());

	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(0)->getTermType() == term0->getTermType());
	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(1)->getTermType() == term1->getTermType());
	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(2)->getTermType() == term2->getTermType());

	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(0)->getSimilarityBoost() == term0->getSimilarityBoost());
	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(1)->getSimilarityBoost() == term1->getSimilarityBoost());
	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(2)->getSimilarityBoost() == term2->getSimilarityBoost());

	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(0)->getThreshold() == term0->getThreshold());
	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(1)->getThreshold() == term1->getThreshold());
	ASSERT(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(2)->getThreshold() == term2->getThreshold());

	ASSERT(*(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(0)->getKeyword()) == *(term0->getKeyword()));
	ASSERT(*(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(1)->getKeyword()) == *(term1->getKeyword()));
	ASSERT(*(deserializedCommandInput1.getLogicalPlan()->getExactQuery()->getQueryTerms()->at(2)->getKeyword()) == *(term2->getKeyword()));


}

void testSerializableSearchResults(){

	QueryResults * queryResults1 = prepareQueryResults();
	SearchCommandResults commandInput1;
	SearchCommandResults::ShardResults * shardResults = new SearchCommandResults::ShardResults("identifier");
	commandInput1.addShardResults(shardResults);
	shardResults->searcherTime = 120;
	shardResults->queryResults = *queryResults1;
	delete queryResults1;
	MessageAllocator * aloc = new MessageAllocator();
	void * buffer = commandInput1.serialize(aloc);
	const SearchCommandResults & deserializedCommandInput1 = *(SearchCommandResults::deserialize(buffer));

	vector<SearchCommandResults::ShardResults *> allShardResults = deserializedCommandInput1.getShardResults();

	ASSERT(allShardResults.size() == commandInput1.getShardResults().size());
	for(unsigned resultIndex = 0 ; resultIndex < commandInput1.getShardResults().size() ; ++resultIndex){
		checkQueryResultContent(commandInput1.getShardResults().at(resultIndex)->queryResults.impl->sortedFinalResults.at(resultIndex),
				deserializedCommandInput1.getShardResults().at(resultIndex)->queryResults.impl->sortedFinalResults.at(resultIndex));
	}
	checkFacetResults(&(commandInput1.getShardResults().at(0)->queryResults) ,
			&(deserializedCommandInput1.getShardResults().at(0)->queryResults));
	ASSERT(commandInput1.getShardResults().at(0)->queryResults.impl->estimatedNumberOfResults ==
			deserializedCommandInput1.getShardResults().at(0)->queryResults.impl->estimatedNumberOfResults);
	ASSERT(commandInput1.getShardResults().at(0)->queryResults.impl->resultsApproximated ==
			deserializedCommandInput1.getShardResults().at(0)->queryResults.impl->resultsApproximated);

}


void testSerializableSerializeCommandInput(){
	SerializeCommand commandInput1(SerializeCommand::SERIALIZE_INDEX);
	MessageAllocator * aloc = new MessageAllocator();
	void * buffer = commandInput1.serialize(aloc);
	const SerializeCommand & deserializedCommandInput1 = *(SerializeCommand::deserialize(buffer));
	ASSERT(commandInput1.getIndexOrRecord() == deserializedCommandInput1.getIndexOrRecord());

	SerializeCommand commandInput2(SerializeCommand::SERIALIZE_RECORDS , "path/serialization_file_name.txt");
	buffer = commandInput2.serialize(aloc);
	const SerializeCommand & deserializedCommandInput2 = *(SerializeCommand::deserialize(buffer));
	ASSERT(commandInput2.getIndexOrRecord() == deserializedCommandInput2.getIndexOrRecord());
	ASSERT(commandInput2.getDataFileName() == deserializedCommandInput2.getDataFileName());
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
