//$Id: Srch2Server.cpp 3456 2013-06-14 02:11:13Z jiaying $

#include <syslog.h>
#include "Srch2Server.h"
#include "util/RecordSerializerUtil.h"
#include <sys/stat.h>
#include "operation/AttributeAccessControl.h"

#ifndef ANDROID
#   include <sys/statvfs.h>
#else
#   include <sys/vfs.h>
#   define statvfs statfs
#endif


namespace srch2
{
namespace httpwrapper
{

//Helper function to calculate size of the file
namespace {
    ifstream::pos_type getFileSize(const char* filename) {
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        ifstream::pos_type size = in.tellg();
        in.close();
        return size;
    }
}

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
    if(!checkDirExistence((directoryName + "/" + IndexConfig::invertedIndexFileName).c_str()))
    	return false;
    if(!checkDirExistence((directoryName + "/" + IndexConfig::quadTreeFileName).c_str()))
	    return false;
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


	                //check file size in KB
	                unsigned fileSize = getFileSize(getDataFilePath().c_str());
	                //Logger::console("The size of the data file is %lu KB", fileSize/(1024));

	                struct statvfs *buff;
	                if (!(buff = (struct statvfs *) malloc(sizeof(struct statvfs)))) {
	                    Logger::error("Failed to allocate memory to buffer.");
	                } else {
	                    //We check the space available for the disk where srch2Home is set
	                    if (statvfs(indexDataConfig->getSrch2Home().c_str(), buff) < 0) {
	                        Logger::warn("Failed to calculate free disk space, statvfs() failed.");
	                    } else {
	                        //Logger::console("The number of free blocks on disk is %lu", buff->f_bfree);
	                        //Logger::console("The size of each block is %lu bytes", buff->f_bsize);
	                        //Logger::console("The total size of free disk space is %lu KB", (buff->f_bfree * buff->f_bsize) / (1024));

	                        //calculate free disk space. (No. of free blocks * block size) KB
	                        unsigned long freeDiskSpace = (buff->f_bfree * buff->f_bsize) / (1024);

	                        //Display warning if free disk space is less than twice the size of file
	                        if (freeDiskSpace < (2 * fileSize)) {
	                            Logger::warn("The system may not have enough disk space to serialize the indexes for the given json file.");
	                        }
	                    }
	                    free(buff);
	                }

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
	            // Load ACL list from disk
	            indexer->getAttributeAcl().bulkLoadAttributeAclJSON(indexDataConfig->getAttibutesAclFile());

	            if(indexDataConfig->getHasRecordAcl()){
	            	DaemonDataSource::addRecordAclFile(indexer, indexDataConfig);
	            }

	            /*
	             *  if the roleCore is null it means that this core doesn't have any access control
	             *  so we can save it now.
	             *  otherwise first we should read the data for acl and we will save this core after that.
	             */
	            if (indexedCounter > 0) {
	                indexer->save();
	                Logger::console("Indexes saved.");
	            }
				break;
		}
	    default:
	    {
		    indexer->commit();
            // Load ACL list from disk
		    indexer->getAttributeAcl().bulkLoadAttributeAclJSON(indexDataConfig->getAttibutesAclFile());
            if(indexDataConfig->getHasRecordAcl()){
            	DaemonDataSource::addRecordAclFile(indexer, indexDataConfig);
            }
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
	if (component == "SCHEMA.IDX") {
		indexer->setSchema(indexDataConfig->getSchema());
	} else if (input.peek() != std::ifstream::traits_type::eof()){
		indexer->bootStrapComponentFromByteSteam(input, component);
        Logger::sharding(Logger::Detail, "Srch2Server | component %s is bootStrap from byte stream.");
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
	indexFiles.push_back(make_pair(IndexConfig::quadTreeFileName, 0));
	indexFiles.push_back(make_pair(IndexConfig::invertedIndexFileName, 0));
	// TODO : Remove this if else statement if you confirm that we dont need it.
//	if (indexer->getSchema()->getIndexType() == LocationIndex) {
//	} else {
//	}
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
	this->__debugShardingInfo = this->getIndexer()->__getDebugShardingInfo();
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
