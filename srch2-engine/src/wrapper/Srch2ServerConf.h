//$Id: Srch2ServerConf.h 3456 2013-06-14 02:11:13Z jiaying $

#ifndef __SRCH2SERVERCONG_H__
#define __SRCH2SERVERCONG_H__

#include <instantsearch/Schema.h>
#include <string>
#include <vector>
#include <sstream>
#include <stdint.h>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
namespace po = boost::program_options;

namespace srch2
{
namespace httpwrapper
{

typedef enum
{
	KAFKAWRITEAPI = 0,
	HTTPWRITEAPI = 1
} WriteApiType;

typedef enum
{
	INDEXCREATE = 0,
	INDEXLOAD = 1
} IndexCreateOrLoad;

typedef enum
{
	FILEBOOTSTRAP_FALSE = 0,
	FILEBOOTSTRAP_TRUE = 1
}DataSourceType;

class Srch2ServerConf
{
private:

	// Argument file options
	string licenseKeyFile;
	string trieBootstrapDictFile;
	uint32_t documentLimit;
	uint64_t memoryLimit;
	string httpServerListeningHostname;
	string httpServerListeningPort;
	string kafkaBrokerHostName;
	uint16_t kafkaBrokerPort;
	string kafkaConsumerTopicName; //Customer name
	uint32_t kafkaConsumerPartitionId;
	uint32_t pingKafkaBrokerEveryNSeconds;
	uint32_t writeReadBufferInBytes;
	unsigned cacheSizeInBytes;
	unsigned mergeEveryNSeconds;
	unsigned mergeEveryMWrites;

	int indexType;
	string attributeLatitude;
	string attributeLongitude;
	float defaultSpatialQueryBoundingBox;

	string primaryKey;
	int searchResponseFormat;
    vector<string> attributesToReturn;
	int numberOfThreads;
	string attributeStringForMySQLQuery;

	//vector<string> searchableAttributes;
    // < keyword, < offset, boost > >
    map<string, pair<unsigned, unsigned> > searchableAttributesTriple;
	vector<string> sortableAttributes;
	string attributeRecordBoost;

	string scoringExpressionString;
	vector<srch2::instantsearch::FilterType> sortableAttributesType; // Float or unsigned
	vector<string> sortableAttributesDefaultValue;

	//vector<unsigned> attributesBoosts;

	std::string allowedRecordTokenizerCharacters;
	int searchType;
	int isPrimSearchable;
	bool exactFuzzy;
	bool queryTermType;
	unsigned queryTermBoost;
	float queryTermSimilarityBoost;
	float queryTermLengthBoost;
	float prefixMatchPenalty;
	bool supportAttributeBasedSearch;

	DataSourceType dataSourceType;
	IndexCreateOrLoad indexCreateOrLoad;
	WriteApiType writeApiType;

	int resultsToRetrieve;
	int attributeToSort;
	int ordering;
	int searchResponseJsonFormat;
	bool recordBoostAttributeSet;

	string indexPath;
	string filePath;
	string httpServerAccessLogFile;
	string httpServerErrorLogFile;
	//string httpServerDocumentRoot;

public:
	Srch2ServerConf(int argc, char** argv, bool &configSuccess, std::stringstream &parseError);
	virtual ~Srch2ServerConf();

	void kafkaOptionsParse(const po::variables_map &vm, bool &configSuccess, std::stringstream &parseError);
	void _setDefaultSearchableAttributeBoosts(const vector<string> &searchableAttributesVector);
	void parse(const boost::program_options::variables_map &vm, bool &configSuccess, std::stringstream &parseError);

	const std::string& getCustomerName() const;
	uint32_t getDocumentLimit() const;
	uint64_t getMemoryLimit() const;

	const std::string& getIndexPath() const;
	const std::string& getFilePath() const;
	const std::string& getPrimaryKey() const;

	const map<string, pair<unsigned, unsigned> > * getSearchableAttributes() const;
    const vector<string> * getAttributesToReturnName() const;

	const vector<string> * getSortableAttributesName() const;
	const vector<srch2::instantsearch::FilterType> * getSortableAttributesType() const;
	const vector<string> * getSortableAttributesDefaultValue() const;

	//const vector<unsigned>* getAttributesBoosts() const;
	const std::string& getAttributeRecordBoostName() const;
	//string getDefaultAttributeRecordBoost() const;
	const std::string& getScoringExpressionString() const;

	const std::string& getRecordAllowedSpecialCharacters() const;
	int getSearchType() const;
	int getIsPrimSearchable() const;
	bool getIsFuzzyTermsQuery() const;
	bool getQueryTermType() const;
	unsigned getQueryTermBoost() const;
	float getQueryTermSimilarityBoost() const;
	float getQueryTermLengthBoost() const;
	float getPrefixMatchPenalty() const;
	bool getSupportAttributeBasedSearch() const;
	int getDefaultResultsToRetrieve() const;
	int getAttributeToSort() const;
	int getOrdering() const;

	uint32_t getCacheSizeInBytes() const;
	uint32_t getMergeEveryNSeconds() const;
	uint32_t getMergeEveryMWrites() const;

	int getNumberOfThreads() const;

	DataSourceType getDataSourceType() const;
	IndexCreateOrLoad getIndexCreateOrLoad() const;
	WriteApiType getWriteApiType() const;

	int getSearchResponseFormat() const;
	const std::string& getAttributeStringForMySQLQuery() const;
	int getSearchResponseJSONFormat() const;

	const std::string& getLicenseKeyFileName() const;
	const std::string& getTrieBootstrapDictFileName() const;

	const std::string& getHTTPServerAccessLogFile() const;
	const std::string& getHTTPServerErrorLogFile() const;
	const std::string& getHTTPServerDocumentRoot() const;
	const std::string& getHTTPServerListeningHostname() const;
	const std::string& getHTTPServerListeningPort() const;

	const std::string& getKafkaBrokerHostName() const;
	uint16_t getKafkaBrokerPort() const;
	const std::string& getKafkaConsumerTopicName() const;
	uint32_t getKafkaConsumerPartitionId() const;
	uint32_t getWriteReadBufferInBytes() const;
	uint32_t getPingKafkaBrokerEveryNSeconds() const;

	bool isRecordBoostAttributeSet() const;

	int getIndexType() const;
	const std::string& getAttributeLatitude() const;
	const std::string& getAttributeLongitude() const;
	float getDefaultSpatialQueryBoundingBox() const;
};

}
}



#endif /* __SRCH2SERVERCONG_H__ */
