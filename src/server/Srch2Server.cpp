//$Id: Srch2Server.cpp 3456 2013-06-14 02:11:13Z jiaying $

#include <syslog.h>
#include "Srch2Server.h"
#include "util/RecordSerializerUtil.h"
#include <sys/stat.h>

namespace srch2
{
namespace httpwrapper
{

bool Srch2Server::checkIndexExistence(const string & directoryPath)
{
    const string &directoryName = directoryPath;
//    if(!checkDirExistence((directoryName + "/" + IndexConfig::analyzerFileName).c_str()))
//        return false;
    if(!checkDirExistence((directoryName + "/" + IndexConfig::trieFileName).c_str()))
        return false;
    if(!checkDirExistence((directoryName + "/" + IndexConfig::forwardIndexFileName).c_str()))
        return false;
    if(!checkDirExistence((directoryName + "/" + IndexConfig::schemaFileName).c_str()))
        return false;
    if (getCoreInfo()->getIndexType() == srch2::instantsearch::DefaultIndex){
        // Check existence of the inverted index file for basic keyword search ("A1")
        if(!checkDirExistence((directoryName + "/" + IndexConfig::invertedIndexFileName).c_str()))
	    return false;
    }else{
        // Check existence of the quadtree index file for geo keyword search ("M1")
        if(!checkDirExistence((directoryName + "/" + IndexConfig::quadTreeFileName).c_str()))
	    return false;
    }
    return true;
}

IndexMetaData *Srch2Server::createIndexMetaData(const string & directoryPath)
{
    //Create a cache
    srch2is::GlobalCache *cache = srch2is::GlobalCache::create(getCoreInfo()->getCacheSizeInBytes(), 200000);

    // Create an IndexMetaData
    srch2is::IndexMetaData *indexMetaData =
        new srch2is::IndexMetaData( cache,
        		getCoreInfo()->getMergeEveryNSeconds(),
        		getCoreInfo()->getMergeEveryMWrites(),
        		getCoreInfo()->getUpdateHistogramEveryPMerges(),
        		getCoreInfo()->getUpdateHistogramEveryQWrites(),
        		directoryPath);

    return indexMetaData;
}
void Srch2Server::createHighlightAttributesVector(const srch2is::Schema * schema) {

	CoreInfo_t *indexConfig = const_cast<CoreInfo_t *> (this->getCoreInfo());
	vector<std::pair<unsigned, string> > highlightAttributes;

	const map<string, SearchableAttributeInfoContainer > * searchableAttrsFromConfig
	 	 = indexConfig->getSearchableAttributes();

	map<string, SearchableAttributeInfoContainer >::const_iterator cIter;

	std::map<std::string, unsigned>::const_iterator iter =
			schema->getSearchableAttribute().begin();
	for ( ; iter != schema->getSearchableAttribute().end(); iter++)
	{
		// Currently only searchable attributes are highlightable. Cross validate the schema
		// attribute with configuration attributes. (There could be a mismatch when index is loaded
		// from file).
		cIter =  searchableAttrsFromConfig->find(iter->first);
		if (cIter != searchableAttrsFromConfig->end() && cIter->second.highlight)
		{
			highlightAttributes.push_back(make_pair(iter->second, iter->first));
		}
	}
	indexConfig->setHighlightAttributeIdsVector(highlightAttributes);
}
void Srch2Server::createAndBootStrapIndexer(const string & directoryPath)
{
    // create IndexMetaData
    IndexMetaData *indexMetaData = createIndexMetaData(directoryPath);
    IndexCreateOrLoad indexCreateOrLoad;
    if(checkIndexExistence(directoryPath))
        indexCreateOrLoad = srch2http::INDEXLOAD;
    else
        indexCreateOrLoad = srch2http::INDEXCREATE;
    Schema * storedAttrSchema = Schema::create();
    switch (indexCreateOrLoad)
    {
    case srch2http::INDEXCREATE:
	{
	    AnalyzerHelper::initializeAnalyzerResource(this->getCoreInfo());
	    const srch2is::Schema *schema = this->getCoreInfo()->getSchema();
	    indexer = Indexer::create(indexMetaData, schema);
		RecordSerializerUtil::populateStoredSchema(storedAttrSchema, getIndexer()->getSchema());
	    switch(getCoreInfo()->getDataSourceType())
	    {
	    case srch2http::DATA_SOURCE_JSON_FILE:
	        {
	        	unsigned indexedCounter = 0;
	        	if(getDataFilePath().compare("") != 0){
					// Create from JSON and save to index-dir
					Logger::console("Creating indexes from JSON file...");
					indexedCounter = DaemonDataSource::createNewIndexFromFile(getIndexer(),
							storedAttrSchema, this->getCoreInfo(),getDataFilePath());
	        	}
				/*
				 *  commit the indexes once bulk load is done and then save it to the disk only
				 *  if number of indexed record is > 0.
				 */
				getIndexer()->commit();
				if (indexedCounter > 0) {
					getIndexer()->save();
				Logger::console("Indexes saved.");
				}
				break;
		}
	    default:
	        {
		    indexer->commit();
		    Logger::console("Creating new empty index");
		}
	    };
	    AnalyzerHelper::saveAnalyzerResource(this->getCoreInfo());
	    break;
	}
    case srch2http::INDEXLOAD:
    {
	    // Load from index-dir directly, skip creating an index initially.
        indexer = Indexer::create(indexMetaData);
        indexer->bootStrapFromDisk();

        if (!checkSchemaConsistency(this->getCoreInfo()->getSchema(), indexer->getSchema())) {
            Logger::warn("The schema in the config file is different from the"
                    " serialized schema on the disk. The engine will ignore "
                    "the schema from the config file. Please make sure they "
                    "are consistent. One possible solution is to remove all "
                    "the index files and run the engine again.");
        }

	    // Load Analyzer data from disk
	    AnalyzerHelper::loadAnalyzerResource(this->getCoreInfo());
	    getIndexer()->getSchema()->setSupportSwapInEditDistance(getCoreInfo()->getSupportSwapInEditDistance());
	    bool isAttributeBasedSearch = false;
	    if (isEnabledAttributeBasedSearch(getIndexer()->getSchema()->getPositionIndexType())) {
	        isAttributeBasedSearch =true;
	    }
	    if(isAttributeBasedSearch != getCoreInfo()->getSupportAttributeBasedSearch())
	    {
	    	Logger::warn("support-attribute-based-search has changed in the config file"
	    		        		" remove all index files and run it again!");
	    }
	    RecordSerializerUtil::populateStoredSchema(storedAttrSchema, getIndexer()->getSchema());
	    break;
	}
    }
    createHighlightAttributesVector(storedAttrSchema);
    delete storedAttrSchema;
    // start merger thread
    getIndexer()->createAndStartMergeThreadLoop();
}

/*
 * This function will check the consistency of the schema that is loaded from the
 * disk and the schema that is loaded from the config file.
 */
bool Srch2Server::checkSchemaConsistency(const srch2is::Schema *confSchema,
        const srch2is::Schema *loadedSchema) {
    if (confSchema->getNumberOfRefiningAttributes()
            != loadedSchema->getNumberOfRefiningAttributes()) {
        return false;
    }

    if (confSchema->getNumberOfSearchableAttributes()
            != loadedSchema->getNumberOfSearchableAttributes()) {
        return false;
    }

    for (std::map<std::string, unsigned>::const_iterator confIt =
            confSchema->getRefiningAttributes()->begin(), loadedIt =
            loadedSchema->getRefiningAttributes()->begin();
            confIt != confSchema->getRefiningAttributes()->end()
                    && loadedIt != loadedSchema->getRefiningAttributes()->end();
            confIt++, loadedIt++) {
        //Compare the refining attribute's name and type to see if they are same.
        if (confIt->first.compare(loadedIt->first) != 0) {
            return false;
        }

        if (confSchema->getTypeOfRefiningAttribute(confIt->second)
                != loadedSchema->getTypeOfRefiningAttribute(loadedIt->second)) {
            return false;
        }
    }
    for (std::map<std::string, unsigned>::const_iterator confIt =
            confSchema->getSearchableAttribute().begin(), loadedIt =
            loadedSchema->getSearchableAttribute().begin();
            confIt != confSchema->getSearchableAttribute().end()
                    && loadedIt != loadedSchema->getSearchableAttribute().end();
            confIt++, loadedIt++) {
        //Compare the searchable attribute's name and type to see if they are same.
        if (confIt->first.compare(loadedIt->first) != 0) {
            return false;
        }

        if (confSchema->getTypeOfSearchableAttribute(confIt->second)
                != loadedSchema->getTypeOfSearchableAttribute(
                        loadedIt->second)) {
            return false;
        }
    }
    return true;
}

/*
 *   Load Shard indexes from byte Stream
 */
void Srch2Server::bootStrapShardComponentFromByteStream(std::istream& input, const string & component) {

	if (indexer == NULL) {
		// this is first call on this shard.
		IndexMetaData *indexMetaData = createIndexMetaData(this->directoryPath);
		indexer = Indexer::create(indexMetaData);
	}
	if (input.peek() != std::ifstream::traits_type::eof()) {
		indexer->bootStrapComponentFromByteSteam(input, component);
	} else if (component == "SCHEMA.IDX"){
		indexer->setSchema(indexDataConfig->getSchema());
	}
}

int Srch2Server::getSerializedShardSize(vector<std::pair<string, long> > &indexFiles) {
	string directoryName = this->indexer->getStoredIndexDirectory();

	// first check whether the directory exists.
	struct stat dirInfo;
	int returnStatus = ::stat(directoryName.c_str(), &dirInfo);
	if (returnStatus == -1) {
		return -1;
	}

	if (directoryName[directoryName.size() - 1] != '/') {
		directoryName.append("/");
	}

	indexFiles.push_back(make_pair(IndexConfig::schemaFileName, 0));
	indexFiles.push_back(make_pair(IndexConfig::trieFileName, 0));
	indexFiles.push_back(make_pair(IndexConfig::forwardIndexFileName, 0));
	if (indexer->getSchema()->getIndexType() == LocationIndex) {
		indexFiles.push_back(make_pair(IndexConfig::quadTreeFileName, 0));
	} else {
		indexFiles.push_back(make_pair(IndexConfig::invertedIndexFileName, 0));
	}
	indexFiles.push_back(make_pair(IndexConfig::indexCountsFileName, 0));
	//indexFiles.push_back(make_pair(IndexConfig::analyzerFileName, 0));

	for (unsigned i = 0; i < indexFiles.size(); ++i) {
		long size = getSerializedIndexSizeInBytes(directoryName + indexFiles[i].first);
		if (size == -1) {
			// there was some error and we cannot determine the correct shard size.
			indexFiles.clear();
			return -1;
		}
		indexFiles[i].second = size;
	}
	return 0;
}

long Srch2Server::getSerializedIndexSizeInBytes(const string &indexFileFullPath) {
	struct stat fileInfo;
	int returnStatus = ::stat(indexFileFullPath.c_str(), &fileInfo);
	if (returnStatus == -1) {
		if (errno == ENOENT) {
			// index files may not be written to disk because there is no record in the index.
			return 0;
		} else {
			// any other return status should be treated as an error
			perror("");
			return -1;
		}
	} else {
		return fileInfo.st_size;
	}
}

/*
 *   Any inconsistency between loaded indexes and current configuration should be handled in this
 *   function.
 */
void Srch2Server::postBootStrap() {

	if (indexer == NULL) {
		return;  // call bootStrap functions first.
	}

	Schema * storedAttrSchema = Schema::create();
	indexer->getSchema()->setSupportSwapInEditDistance(getCoreInfo()->getSupportSwapInEditDistance());
	bool isAttributeBasedSearch = false;
	if (isEnabledAttributeBasedSearch(getIndexer()->getSchema()->getPositionIndexType())) {
		isAttributeBasedSearch =true;
	}
	if(isAttributeBasedSearch != getCoreInfo()->getSupportAttributeBasedSearch())
	{
		Logger::warn("support-attribute-based-search has changed in the config file"
				" remove all index files and run it again!");
	}
	RecordSerializerUtil::populateStoredSchema(storedAttrSchema, getIndexer()->getSchema());
    createHighlightAttributesVector(storedAttrSchema);
	delete storedAttrSchema;
	getIndexer()->createAndStartMergeThreadLoop();
}

Indexer * Srch2Server::getIndexer(){
	return indexer;
}
const CoreInfo_t * Srch2Server::getCoreInfo(){
	return indexDataConfig;
}

}
}
