// $Id: IndexData.cpp 3480 2013-06-19 08:00:34Z iman $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright 2010 SRCH2 Inc. All rights reserved
 */

#include "IndexData.h"
#include "record/SchemaInternal.h"
#include "index/IndexUtil.h"
#include "index/Trie.h"
#include "index/InvertedIndex.h"
#include "index/ForwardIndex.h"
#include "util/Assert.h"
#include "util/Logger.h"
#include <instantsearch/Record.h>
#include <instantsearch/Analyzer.h>
#include "util/FileOps.h"
#include "serialization/Serializer.h"
#include "util/RecordSerializerUtil.h"
#include "util/RecordSerializer.h"
#include <stdio.h>  /* defines FILENAME_MAX */
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <exception>
#include "AttributeAccessControl.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

using std::string;
using std::vector;
using std::map;
using std::pair;
using srch2::util::Logger;
//using std::unordered_set;
using namespace srch2::util;

namespace srch2 {
namespace instantsearch {

IndexData::IndexData(const string &directoryName, Analyzer *analyzer,
		Schema *schema, const StemmerNormalizerFlagType &stemmerFlag) {

	this->directoryName = directoryName;

	if (!checkDirExistence(directoryName.c_str())) {
		if (createDir(directoryName.c_str()) == -1) {
			throw std::runtime_error("Index Directory can not be created");
		}
	}

	this->schemaInternal = new SchemaInternal(
			*(dynamic_cast<SchemaInternal *>(schema)));

	this->rankerExpression = new RankerExpression(
			this->schemaInternal->getScoringExpression());

	this->trie = new Trie_Internal();

	this->forwardIndex = new ForwardIndex(this->schemaInternal);

	this->invertedIndex = new InvertedIndex(this->forwardIndex);

	this->quadTree = new QuadTree();

	this->permissionMap = new PermissionMap();

	this->readCounter = new ReadCounter();
	this->writeCounter = new WriteCounter();
	this->flagBulkLoadDone = false;

	this->mergeRequired = true;

	this->attributeAcl = new AttributeAccessControl(this->schemaInternal);
}

IndexData::IndexData(const string& directoryName) {
	this->directoryName = directoryName;

	if (!checkDirExistence(directoryName.c_str())) {
		Logger::error("Given index path %s does not exist",
				directoryName.c_str());
		throw std::runtime_error("Index load exception ");
	}
	Serializer serializer;
	try {
		this->schemaInternal = new SchemaInternal();
		serializer.load(*(this->schemaInternal),
				this->directoryName + "/" + IndexConfig::schemaFileName);
		this->rankerExpression = new RankerExpression(
				this->schemaInternal->getScoringExpression());

		this->trie = new Trie_Internal();
		this->forwardIndex = new ForwardIndex(this->schemaInternal);
		serializer.load(*(this->trie),
				directoryName + "/" + IndexConfig::trieFileName);

		this->invertedIndex = new InvertedIndex(this->forwardIndex);

		this->quadTree = new QuadTree();

		// set if it's a attributeBasedSearch
		PositionIndexType positionIndexType =
				this->schemaInternal->getPositionIndexType();
		if (isEnabledAttributeBasedSearch(positionIndexType))
			this->forwardIndex->isAttributeBasedSearch = true;

		serializer.load(*(this->forwardIndex),
				directoryName + "/" + IndexConfig::forwardIndexFileName);
		this->forwardIndex->setSchema(this->schemaInternal);

		serializer.load(*(this->invertedIndex),
				directoryName + "/" + IndexConfig::invertedIndexFileName);
		this->invertedIndex->setForwardIndex(this->forwardIndex);

		serializer.load(*(this->quadTree),
				directoryName + "/" + IndexConfig::quadTreeFileName);

		this->permissionMap = new PermissionMap();
		serializer.load(*(this->permissionMap),
				directoryName + "/" + IndexConfig::permissionMapFileName);

		this->attributeAcl = new AttributeAccessControl(this->schemaInternal);
		string attrAclFileName = directoryName + "/" + IndexConfig::AccessControlFile;
		if (::access(attrAclFileName.c_str(), F_OK) != -1) {
			serializer.load(*(this->attributeAcl), attrAclFileName);
		}

		this->loadCounts(
				directoryName + "/" + IndexConfig::indexCountsFileName);
		this->flagBulkLoadDone = true;
	} catch (exception& ex) {
		Logger::error("Error while loading the index files ...");
		throw ex;
	}

	this->mergeRequired = true;
}

// check whether the keyword id list is sorted. This is called from ASSERT statement below to
// verify the correctness of the assumption that keywordIdList is alphabetaically sorted
bool isSortedAlphabetically(
		const KeywordIdKeywordStringInvertedListIdTriple& keywordIdList) {

	if (keywordIdList.size() < 2)
		return true; // 0 or 1 element array is considered sorted

	KeywordIdKeywordStringInvertedListIdTriple::const_iterator iter =
			keywordIdList.begin();
	KeywordIdKeywordStringInvertedListIdTriple::const_iterator previter = iter;
	++iter;
	while (iter != keywordIdList.end()) {
		if (iter->second.first.compare(previter->second.first) <= 0)
			return false;
		++iter;
	}
	return true;
}

// Get the read views of different indexes so that we can use the same, consistent
// read view for each of them during the lifecycle of a search process.
void IndexData::getReadView(IndexReadStateSharedPtr_Token &readToken)
{
    this->trie->getTrieRootNode_ReadView(readToken.trieRootNodeSharedPtr);
    this->quadTree->getQuadTreeRootNode_ReadView(readToken.quadTreeRootNodeSharedPtr);
    this->forwardIndex->getForwardListDirectory_ReadView(readToken.forwardIndexReadViewSharedPtr);
    this->invertedIndex->getInvertedIndexDirectory_ReadView(readToken.invertedIndexReadViewSharedPtr);
    this->readCounter->increment();
}

INDEXWRITE_RETVAL IndexData::_aclModifyRecordAccessList(const std::string& resourcePrimaryKeyID,
		vector<string> &roleIds, RecordAclCommandType commandType) {

	shared_ptr<vectorview<ForwardListPtr> >  forwardListDirectoryReadView;
	this->forwardIndex->getForwardListDirectory_ReadView(forwardListDirectoryReadView);
	RecordAcl* accessList = this->forwardIndex->getRecordAccessList(forwardListDirectoryReadView, resourcePrimaryKeyID);

	switch (commandType){
	case Acl_Record_Add:
		if(accessList != NULL){
			#if 0
			this->permissionMap->deleteResourceFromRoles(resourcePrimaryKeyID, accessList->getRoles());
			this->permissionMap->appendResourceToRoles(resourcePrimaryKeyID, roleIds);
			#endif
			accessList->clearRoles();
			this->forwardIndex->appendRoleToResource(forwardListDirectoryReadView, resourcePrimaryKeyID, roleIds);
			return OP_SUCCESS;
		}
		break;
	case Acl_Record_Append:
		// 1- append these role ids to the access list of the record
		// 2- add the id of this record to vector of resource ids for this role id in the permission map
		if(this->forwardIndex->appendRoleToResource(forwardListDirectoryReadView, resourcePrimaryKeyID, roleIds)){
			#if 0
			this->permissionMap->appendResourceToRoles(resourcePrimaryKeyID, roleIds);
			#endif
			return OP_SUCCESS;
		}
		break;
	case Acl_Record_Delete:
		// 1- Delete these role ids from the access list of the record
		// 2- delete the id of this record from the vector of resource ids for this role id in the permission map
		if(this->forwardIndex->deleteRoleFromResource(forwardListDirectoryReadView, resourcePrimaryKeyID, roleIds)){
			#if 0
			this->permissionMap->deleteResourceFromRoles(resourcePrimaryKeyID, roleIds);
			#endif
			return OP_SUCCESS;
		}
		break;
	default:
		ASSERT(false);
		break;
	};

	return OP_FAIL;
}

INDEXWRITE_RETVAL IndexData::_aclRoleRecordDelete(const std::string& rolePrimaryKeyID){

	// 1- get the resource ids for this role record
	vector<string>* resourceIds = this->permissionMap->getResourceIdsForRole(rolePrimaryKeyID);

	if( resourceIds != NULL){

		shared_ptr<vectorview<ForwardListPtr> >  forwardListDirectoryReadView;
		this->forwardIndex->getForwardListDirectory_ReadView(forwardListDirectoryReadView);

		for(unsigned i = 0 ; i < resourceIds->size() ; ++i ){
			// 2- delete this role record id from the access list of these resource records
			this->forwardIndex->deleteRoleFromResource(forwardListDirectoryReadView, resourceIds->at(i), rolePrimaryKeyID);
		}
		// 3- delete this role id from the permission map
		this->permissionMap->deleteRole(rolePrimaryKeyID);
	}
	return OP_SUCCESS;
}

/// Add a record
INDEXWRITE_RETVAL IndexData::_addRecord(const Record *record,
		Analyzer *analyzer) {
	return _addRecordWithoutLock(record, analyzer);
}

INDEXWRITE_RETVAL IndexData::_addRecordWithoutLock(const Record *record,
		Analyzer *analyzer) {
	/// Get the internalRecordId
	unsigned internalRecordIdTemp;
	//Check for duplicate record
	bool recordPrimaryKeyFound =
			this->forwardIndex->getInternalRecordIdFromExternalRecordId(
					record->getPrimaryKey(), internalRecordIdTemp);

	// InternalRecordId == ForwardListId
	if (recordPrimaryKeyFound) {
		return OP_FAIL;
	}

	this->writeCounter->incWritesCounter();
	this->writeCounter->incDocsCounter();

	this->mergeRequired = true;
	/// analyze the record (tokenize it, remove stop words)
	map<string, TokenAttributeHits> tokenAttributeHitsMap;
	analyzer->tokenizeRecord(record, tokenAttributeHitsMap);

	KeywordIdKeywordStringInvertedListIdTriple keywordIdList;

	for (map<string, TokenAttributeHits>::iterator mapIterator =
			tokenAttributeHitsMap.begin();
			mapIterator != tokenAttributeHitsMap.end(); ++mapIterator) {
		/// add words to trie

		unsigned invertedIndexOffset = 0;
		unsigned keywordId = 0;

		// two flags that only M1 update needs
		bool isNewTrieNode = false;
		bool isNewInternalTerminalNode = false;
		// the reason we need @var hasExactlyOneChild is that for this kind of prefix on c-filter, we shouldn't change it in-place. Instead, we should add the new prefix when the old one is found on c-filter
		// e.g. Initially on Trie we have "cancer"-4 'c'->'a'->'n'->'c'->'e'->'r'. Each Trie node have the same prefix interval [4, 4]
		//        If we insert a new record with keyword "can"-1, then Trie nodes 'c', 'a' and 'n' will become [1,4], while 'c', 'e' and 'r' will stay the same.
		//        So if we find [4, 4] on c-filter, we should NOT change it to [2, 4]. Instead, we should keep [4, 4] and add [2, 4]
		bool hadExactlyOneChild = false;
		unsigned breakLeftOrRight = 0;
		vector<Prefix> *oldParentOrSelfAndAncs = NULL;

		if (!this->flagBulkLoadDone) // not committed yet
			//transform string to vector<CharType>
			keywordId = this->trie->addKeyword(
					getCharTypeVector(mapIterator->first), invertedIndexOffset);
		else {
			//transform string to vector<CharType>
			keywordId = this->trie->addKeyword_ThreadSafe(
					getCharTypeVector(mapIterator->first), invertedIndexOffset,
					isNewTrieNode, isNewInternalTerminalNode);
		}

		// For the case where the keyword is new, and we do not have the "space" to assign a new id
		// for it, we assign a positive integer to this keyword.  So the returned value
		// should also be valid.
		keywordIdList.push_back(
				make_pair(keywordId,
						make_pair(mapIterator->first, invertedIndexOffset)));

		this->invertedIndex->incrementHitCount(invertedIndexOffset);
	}

	// Commented out the sort statement below. We do not need to sort the keyword List again
	// because keywords are stored in a map which keeps them alphabetically sorted.
	// If this _addRecord() is called BEFORE commit(), these keyword IDs within this list are
	// sorted using the alphabetical order of the keywords, even though they are not sorted
	// based on the integer order. During the commit() phase, we will map these IDs to their
	// final IDs from the trie, and the order of the IDs becomes the integer order automatically.
	// If this _addRecord() is called AFTER commit(), the keyword IDs are sorted using the
	// alphabetical order, which is already consistent with the integer order.
	//
	// std::sort( keywordIdList.begin(), keywordIdList.end());

	// Adding this assert to ensure that keywordIdList is alphabetically sorted. see isSorted()
	// function above.
	ASSERT(isSortedAlphabetically(keywordIdList));

	unsigned internalRecordId;
	this->forwardIndex->appendExternalRecordIdToIdMap(record->getPrimaryKey(),
			internalRecordId);
	this->forwardIndex->addRecord(record, internalRecordId, keywordIdList,
			tokenAttributeHitsMap);

	if (this->flagBulkLoadDone) {
		const unsigned totalNumberofDocuments =
				this->forwardIndex->getTotalNumberOfForwardLists_WriteView();
		ForwardList *forwardList = this->forwardIndex->getForwardList_ForCommit(
				internalRecordId);
		this->invertedIndex->addRecord(forwardList, this->trie,
				this->rankerExpression, internalRecordId, this->schemaInternal,
				record, totalNumberofDocuments, keywordIdList);
	}

	#if 0
	if(record->hasRoleIds()){
		this->permissionMap->appendResourceToRoles(record->getPrimaryKey(), *(record->getRoleIds()));
	}
	#endif

	// Geo Index: need to add this record to the quadtree.
	if (this->schemaInternal->getIndexType()
			== srch2::instantsearch::LocationIndex) {
		if (!this->flagBulkLoadDone) {
			this->quadTree->insert(record, internalRecordId);
		} else {
			this->quadTree->insert_ThreadSafe(record, internalRecordId);
		}
	}

	return OP_SUCCESS;
}

// delete a record with a specific id //TODO Give the correct return message for delete pass/fail
INDEXWRITE_RETVAL IndexData::_deleteRecord(
		const std::string &externalRecordId) {

	unsigned int internalRecordId;
	bool hasRecord =
			this->forwardIndex->getInternalRecordIdFromExternalRecordId(
					externalRecordId, internalRecordId);
	if (hasRecord) {
		ForwardList* forwardList =
				this->forwardIndex->getForwardList_ForCommit(
						internalRecordId);
		this->permissionMap->deleteResourceFromRoles(externalRecordId, forwardList->getAccessList()->getRoles());

		if (this->schemaInternal->getIndexType()
				== srch2::instantsearch::LocationIndex) {

				StoredRecordBuffer buffer = forwardList->getInMemoryData();

				Schema * storedSchema = Schema::create();
				srch2::util::RecordSerializerUtil::populateStoredSchema(
						storedSchema, this->getSchema());
				srch2::util::RecordSerializer compactRecDeserializer =
						srch2::util::RecordSerializer(*storedSchema);

				// get the name of the attributes
				const string* nameOfLatitudeAttribute =
						this->getSchema()->getNameOfLatituteAttribute();
				const string* nameOfLongitudeAttribute =
						this->getSchema()->getNameOfLongitudeAttribute();

				unsigned idLat = storedSchema->getRefiningAttributeId(
						*nameOfLatitudeAttribute);
				unsigned latOffset = compactRecDeserializer.getRefiningOffset(
						idLat);

				unsigned idLong = storedSchema->getRefiningAttributeId(
						*nameOfLongitudeAttribute);
				unsigned longOffset = compactRecDeserializer.getRefiningOffset(
						idLong);
				Point point;
				point.x = *((float *) (buffer.start.get() + latOffset));
				point.y = *((float *) (buffer.start.get() + longOffset));
				this->quadTree->remove_ThreadSafe(point, internalRecordId);
		}
	}

	INDEXWRITE_RETVAL success =
			this->forwardIndex->deleteRecord(externalRecordId) ?
					OP_SUCCESS : OP_FAIL;


	if (success == OP_SUCCESS) {
		ForwardList * fwdList = this->forwardIndex->getForwardList_ForCommit(internalRecordId);
		if (fwdList) {
            // Before calling function getKeywordCorrespondingPathToTrieNode_WriteView, if
            // there are keywords whose ids need to be assigned, we need to call
            // reassignKeywordIds() to make sure the ids of the trie lead nodes
            // are ordered properly. This order can make sure the function
            // getKeywordCorrespondingPathToTrieNode_WriteView() works properly.
                  /*if (this->trie->needToReassignKeywordIds()) {
                // reassign id is not thread safe so we need to grab an exclusive lock
                // NOTE : all index structure commits are happened before reassign id phase. Only QuadTree is left
                //        because we need new ids in quadTree commit phase.
                boost::unique_lock<boost::shared_mutex> lock(globalRwMutexForReadersWriters);
                this->reassignKeywordIds();
                lock.unlock();
                }*/

            // iterate through the keyword ids
            unsigned keywordsCount = fwdList->getNumberOfKeywords();
			const unsigned * listofKeywordIds = fwdList->getKeywordIds();
			// Loop over the keyword-ids for the current forward list and get
			// the inverted-list-ids from the trie.
			TrieNodePath trieNodePath;
			trieNodePath.path = new vector<TrieNode *>();
			vector<unsigned> invertedListIdsToMerge;
			for (unsigned i = 0; i < keywordsCount; ++i) {
				unsigned keywordId = *(listofKeywordIds + i);
				// get the TrieNode path of the current keyword in write view based on its id.
				this->trie->getKeywordCorrespondingPathToTrieNode_WriteView(keywordId, &trieNodePath);
				if (trieNodePath.path->size() == 0) {
					// should not happen.
					ASSERT(false);
					continue;
				}
				TrieNode * leafNode = trieNodePath.path->back();
				if(leafNode && leafNode->isTerminalNode()) {
					invertedListIdsToMerge.push_back(leafNode->invertedListOffset);
				} else {
					// should not happen.
					ASSERT(false);
				}
				trieNodePath.path->clear();
			}
			delete trieNodePath.path;
			this->invertedIndex->appendInvertedListIdsForMerge(invertedListIdsToMerge);
		}

		this->mergeRequired = true; // need to tell the merge thread to merge
		this->writeCounter->decDocsCounter();
		this->writeCounter->incWritesCounter();
	}

	return success;
}

// delete a record with a specific id //TODO Give the correct return message for delete pass/fail
// get the deleted internal recordID
INDEXWRITE_RETVAL IndexData::_deleteRecordGetInternalId(
		const std::string &externalRecordId, unsigned &internalRecordId) {
	bool hasRecord =
			this->forwardIndex->getInternalRecordIdFromExternalRecordId(
					externalRecordId, internalRecordId);
	if (hasRecord) {
		ForwardList* forwardList =
				this->forwardIndex->getForwardList_ForCommit(
						internalRecordId);

		this->permissionMap->deleteResourceFromRoles(externalRecordId, forwardList->getAccessList()->getRoles());

		if (this->schemaInternal->getIndexType()
				== srch2::instantsearch::LocationIndex) {
				StoredRecordBuffer buffer = forwardList->getInMemoryData();

				Schema * storedSchema = Schema::create();
				srch2::util::RecordSerializerUtil::populateStoredSchema(
						storedSchema, this->getSchema());
				srch2::util::RecordSerializer compactRecDeserializer =
						srch2::util::RecordSerializer(*storedSchema);

				// get the name of the attributes
				const string* nameOfLatitudeAttribute =
						this->getSchema()->getNameOfLatituteAttribute();
				const string* nameOfLongitudeAttribute =
						this->getSchema()->getNameOfLongitudeAttribute();

				unsigned idLat = storedSchema->getRefiningAttributeId(
						*nameOfLatitudeAttribute);
				unsigned latOffset = compactRecDeserializer.getRefiningOffset(
						idLat);

				unsigned idLong = storedSchema->getRefiningAttributeId(
						*nameOfLongitudeAttribute);
				unsigned longOffset = compactRecDeserializer.getRefiningOffset(
						idLong);
				Point point;
				point.x = *((float *) (buffer.start.get() + latOffset));
				point.y = *((float *) (buffer.start.get() + longOffset));
				this->quadTree->remove_ThreadSafe(point, internalRecordId);
		}

	}

	INDEXWRITE_RETVAL success =
			this->forwardIndex->deleteRecordGetInternalId(externalRecordId,
					internalRecordId) ? OP_SUCCESS : OP_FAIL;

	if (success == OP_SUCCESS) {
		this->mergeRequired = true; // need to tell the merge thread to merge
		this->writeCounter->decDocsCounter();
		this->writeCounter->incWritesCounter();
	}

	return success;
}

// recover the deleted record
INDEXWRITE_RETVAL IndexData::_recoverRecord(const std::string &externalRecordId,
		unsigned internalRecordId) {
	INDEXWRITE_RETVAL success =
			this->forwardIndex->recoverRecord(externalRecordId,
					internalRecordId) ? OP_SUCCESS : OP_FAIL;

	if(success == OP_SUCCESS){
		this->forwardIndex->getInternalRecordIdFromExternalRecordId(
				externalRecordId, internalRecordId);
		ForwardList* forwardList =
				this->forwardIndex->getForwardList_ForCommit(internalRecordId);

		this->permissionMap->appendResourceToRoles(externalRecordId, forwardList->getAccessList()->getRoles());

		if (this->schemaInternal->getIndexType()
						== srch2::instantsearch::LocationIndex) {

			StoredRecordBuffer buffer = forwardList->getInMemoryData();

			Schema * storedSchema = Schema::create();
			srch2::util::RecordSerializerUtil::populateStoredSchema(storedSchema,
					this->getSchema());
			srch2::util::RecordSerializer compactRecDeserializer =
					srch2::util::RecordSerializer(*storedSchema);

			// get the name of the attributes
			const string* nameOfLatitudeAttribute =
					this->getSchema()->getNameOfLatituteAttribute();
			const string* nameOfLongitudeAttribute =
					this->getSchema()->getNameOfLongitudeAttribute();

			unsigned idLat = storedSchema->getRefiningAttributeId(
					*nameOfLatitudeAttribute);
			unsigned latOffset = compactRecDeserializer.getRefiningOffset(idLat);

			unsigned idLong = storedSchema->getRefiningAttributeId(
					*nameOfLongitudeAttribute);
			unsigned longOffset = compactRecDeserializer.getRefiningOffset(idLong);
			Point point;
			point.x = *((float *) (buffer.start.get() + latOffset));
			point.y = *((float *) (buffer.start.get() + longOffset));
			this->quadTree->insert_ThreadSafe(point, internalRecordId);
		}
	}



	if (success == OP_SUCCESS) {
		this->mergeRequired = true; // need to tell the merge thread to merge

		// recover the changes to the counters made by _deleteRecordGetInternalId
		this->writeCounter->incDocsCounter();
		this->writeCounter->decWritesCounter();
	}

	return success;
}

// check if the record exists
INDEXLOOKUP_RETVAL IndexData::_lookupRecord(
		const std::string &externalRecordId) const {
	return this->forwardIndex->lookupRecord(externalRecordId);
}

/* build the index. After commit(), no more records can be added.
 *
 * There has to be at least one record in Index for the commit to succeed.
 * Returns 1, if the committing succeeded.
 * Returns 0, if the committing failed, which is in the following two cases.
 *    a) No records in index.
 *    b) Index had been already commited.
 */
INDEXWRITE_RETVAL IndexData::finishBulkLoad() {

	if (this->flagBulkLoadDone == false) {
		/*
		 * For the text only Index:
		 * 1. Initialize the size of Inverted Index vector as size of Forward Index vector.
		 * 2. For each ForwardIndex Element, get PositionIndex Element.
		 * 3. Add ForwardIndex and Position Index information as entry into InvertedIndex.
		 * 4. Commit Inverted Index, by traversing Trie by Depth First.
		 * 5. For each Terminal Node, add InvertedIndex offset information in it.
		 *
		 */
		const unsigned totalNumberofDocuments =
				this->forwardIndex->getTotalNumberOfForwardLists_WriteView();

		// Note: we should commit even if totalNumberofDocuments = 0

		this->forwardIndex->commit();
		this->trie->commit();
		this->quadTree->commit();
		//this->trie->print_Trie();
		const vector<unsigned> *oldIdToNewIdMapVector =
				this->trie->getOldIdToNewIdMapVector();

		map<unsigned, unsigned> oldIdToNewIdMapper;
		for (unsigned i = 0; i < oldIdToNewIdMapVector->size(); i++)
			oldIdToNewIdMapper[i] = oldIdToNewIdMapVector->at(i);

		this->invertedIndex->initialiseInvertedIndexCommit();

		for (unsigned forwardIndexIter = 0;
				forwardIndexIter < totalNumberofDocuments; ++forwardIndexIter) {
			ForwardList *forwardList =
					this->forwardIndex->getForwardList_ForCommit(
							forwardIndexIter);
			vector<NewKeywordIdKeywordOffsetTriple> newKeywordIdKeywordOffsetTriple;
			//this->forwardIndex->commit(forwardList, oldIdToNewIdMapVector, newKeywordIdKeywordOffsetTriple);
			this->forwardIndex->commit(forwardList, oldIdToNewIdMapper,
					newKeywordIdKeywordOffsetTriple);

			this->invertedIndex->commit(forwardList, this->rankerExpression,
					forwardIndexIter, totalNumberofDocuments,
					this->schemaInternal, newKeywordIdKeywordOffsetTriple);
		}
		this->forwardIndex->finalCommit();
//        this->forwardIndex->print_size();

		this->invertedIndex->setForwardIndex(this->forwardIndex);
		this->invertedIndex->finalCommit();

		// delete the keyword mapper (from the old ids to the new ids) inside the trie
		this->trie->deleteOldIdToNewIdMapVector();

		/*
		 * Since we don't have inverted index for M1, we send NULL.
		 * NULL will make the component to compute simple frequency
		 * (vs. integration of frequency and recordStaticScores) for nodeSubTrieValue of trie nodes.
		 */

		this->trie->finalCommit_finalizeHistogramInformation(
				this->invertedIndex, this->forwardIndex,
				this->forwardIndex->getTotalNumberOfForwardLists_ReadView());

		this->flagBulkLoadDone = true;
		return OP_SUCCESS;
	} else {
		return OP_FAIL;
	}
}

INDEXWRITE_RETVAL IndexData::_merge(bool updateHistogram) {
	Logger::debug("Merge begins--------------------------------");

	if (!this->mergeRequired)
		return OP_FAIL;

	// struct timespec tstart;
	// clock_gettime(CLOCK_REALTIME, &tstart);

	// struct timespec tend;
	// clock_gettime(CLOCK_REALTIME, &tend);
	// unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
	// cout << time << "-trie merge" << endl;

	this->forwardIndex->merge();
	if (this->forwardIndex->hasDeletedRecords()) {
		// free the space for deleted records.
		// need the global lock to block other readers
		boost::unique_lock<boost::shared_mutex> lock(
				globalRwMutexForReadersWriters);
		this->forwardIndex->freeSpaceOfDeletedRecords();
	}

	if (this->invertedIndex->mergeWorkersCount <= 1) {
		this->invertedIndex->merge( this->rankerExpression,
				this->writeCounter->getNumberOfDocuments(), this->schemaInternal);
	} else {
		this->invertedIndex->parallelMerge();
	}

	// Since trie is the entry point of every search, trie merge should be done after all other merges.
	// If forwardIndex or invertedIndex is merged before trie, then users can see an inconsistent state of
	// the index.
	// if it is the case of M1 (geo), invertedIndex is passed as a NULL, so that histogram information is calculated only
	// using frequencies. Otherwise, the invertedIndex will be used to integrate its information with frequencies.
	const InvertedIndex * invertedIndex = NULL;

	invertedIndex = this->invertedIndex;

	// check if we need to reassign some keyword ids
	if (this->trie->needToReassignKeywordIds()) {

		// struct timespec tstart;
		// clock_gettime(CLOCK_REALTIME, &tstart);

		// reassign id is not thread safe so we need to grab an exclusive lock
		// NOTE : all index structure commits are happened before reassign id phase. Only QuadTree is left
		//        because we need new ids in quadTree commit phase.

		boost::unique_lock<boost::shared_mutex> lock(
				globalRwMutexForReadersWriters);

		this->reassignKeywordIds();

		lock.unlock();

		// struct timespec tend;
		// clock_gettime(CLOCK_REALTIME, &tend);
		// unsigned time = (tend.tv_sec - tstart.tv_s ec) * 1000 +
		// (double) (tend.tv_nsec - tstart.tv_nsec) / (double)1000000L;
		// cout << "Commit phase: time spent to reassign keyword IDs in the forward index (ms): " << time << endl;
	}

	this->trie->merge(invertedIndex, this->forwardIndex,
			this->forwardIndex->getTotalNumberOfForwardLists_ReadView(),
			updateHistogram);

	if (this->schemaInternal->getIndexType()
			== srch2::instantsearch::LocationIndex) {
		this->quadTree->merge();
	}

	this->mergeRequired = false;

	Logger::debug("Merge ends--------------------------------");

	return OP_SUCCESS;
}

/*
 *
 */
void IndexData::reassignKeywordIds() {
	map<TrieNode *, unsigned> trieNodeIdMapper; //
	this->trie->reassignKeywordIds(trieNodeIdMapper);

	// Generating an ID mapper by iterating through the set of trie nodes whose
	// ids need to be reassigned
	// a map from temperory id to new ids, this map is used for changing forwardIndex and quadTree
	map<unsigned, unsigned> keywordIdMapper;
	for (map<TrieNode *, unsigned>::iterator iter = trieNodeIdMapper.begin();
			iter != trieNodeIdMapper.end(); ++iter) {
		TrieNode *node = iter->first;
		unsigned newKeywordId = iter->second;

		keywordIdMapper[node->getId()] = newKeywordId;

		node->setId(newKeywordId); // set the new keyword Id
	}

	// TODO: change it to an unordered set
	//std::unordered_set<unsigned> processedRecordIds; // keep track of records that have been converted
	map<unsigned, unsigned> processedRecordIds; // keep track of records that have been converted

	// Now we have the ID mapper.  We want to go through the trie nodes one by one.
	// For each of them, access its inverted list.  For each record,
	// use the id mapper to change the integers on the forward list.
	changeKeywordIdsOnForwardLists(trieNodeIdMapper, keywordIdMapper,
			processedRecordIds);

}

/*
 * Jamshid : uses the id mapped to replace old ids to new ids in forward list.
 * since we use inverted index to go through all records of a keyword it is possible to visit a record more than once
 * so we use processedRecordIds to remember what records have been reassigned.
 */
void IndexData::changeKeywordIdsOnForwardLists(
		const map<TrieNode *, unsigned> &trieNodeIdMapper,
		const map<unsigned, unsigned> &keywordIdMapper,
		map<unsigned, unsigned> &processedRecordIds) {
	vectorview<unsigned>* &keywordIDsWriteView =
			this->invertedIndex->getKeywordIds()->getWriteView();

	shared_ptr<vectorview<ForwardListPtr> > forwardListDirectoryReadView;
	this->forwardIndex->getForwardListDirectory_ReadView(
			forwardListDirectoryReadView);

	for (map<TrieNode *, unsigned>::const_iterator iter =
			trieNodeIdMapper.begin(); iter != trieNodeIdMapper.end(); ++iter) {
		TrieNode *node = iter->first;

		// the following code is based on TermVirtualList.cpp
		unsigned invertedListId = node->getInvertedListOffset();
		// change the keywordId for given invertedListId
		map<unsigned, unsigned>::const_iterator keywordIdMapperIter =
				keywordIdMapper.find(invertedListId);
		keywordIDsWriteView->at(invertedListId) = keywordIdMapperIter->second;
		// Jamshid : since it happens after the commit of other index structures it uses read view
		shared_ptr<vectorview<unsigned> > readview;
		shared_ptr<vectorview<InvertedListContainerPtr> > invertedListDirectoryReadView;
		this->invertedIndex->getInvertedIndexDirectory_ReadView(
				invertedListDirectoryReadView);
		this->invertedIndex->getInvertedListReadView(
				invertedListDirectoryReadView, invertedListId, readview);
		unsigned invertedListSize = readview->size();
		// go through each record id on the inverted list
		InvertedListElement invertedListElement;
		for (unsigned i = 0; i < invertedListSize; i++) {
			/*if (invertedListElement == NULL)
			 continue;*/
			unsigned recordId = readview->getElement(i);

			// re-map it only it is not done before
			if (processedRecordIds.find(recordId) == processedRecordIds.end()) {

				this->forwardIndex->reassignKeywordIds(
						forwardListDirectoryReadView, recordId,
						keywordIdMapper);
				processedRecordIds[recordId] = 0; // add it to the set
			}
		}
	}

}

void IndexData::_exportData(const string &exportedDataFileName) const {
	ForwardIndex::exportData(*this->forwardIndex, exportedDataFileName);
}

void IndexData::_save(const string &directoryName) const {
	Serializer serializer;
	if (this->trie->isMergeRequired())
		this->trie->merge(NULL, NULL, 0, false);
	// serialize the data structures to disk
	try {
		serializer.save(*this->trie,
				directoryName + "/" + IndexConfig::trieFileName);
	} catch (exception &ex) {
		Logger::error("Error writing trie index file: %s/%s",
				directoryName.c_str(), IndexConfig::trieFileName);
		// can keep running - don't rethrow exception
	}

	if (this->forwardIndex->isMergeRequired()) {
		this->forwardIndex->merge();
		if (this->forwardIndex->hasDeletedRecords()) {
			// free the space for deleted records.
			// need the global lock to block other readers
			boost::unique_lock<boost::shared_mutex> lock(
					globalRwMutexForReadersWriters);
			this->forwardIndex->freeSpaceOfDeletedRecords();
		}
	}

	try {
		serializer.save(*this->forwardIndex,
				directoryName + "/" + IndexConfig::forwardIndexFileName);
	} catch (exception &ex) {
		Logger::error("Error writing forward index file: %s/%s",
				directoryName.c_str(), IndexConfig::forwardIndexFileName);
	}

	try {
		serializer.save(*this->schemaInternal,
				directoryName + "/" + IndexConfig::schemaFileName);
	} catch (exception &ex) {
		Logger::error("Error writing schema index file: %s/%s",
				directoryName.c_str(), IndexConfig::schemaFileName);
	}

	if (this->invertedIndex->mergeRequired())
		this->invertedIndex->merge(this->rankerExpression,
				this->writeCounter->getNumberOfDocuments(), this->schemaInternal);
	try {
		serializer.save(*this->invertedIndex,
				directoryName + "/" + IndexConfig::invertedIndexFileName);
	} catch (exception &ex) {
		Logger::error("Error writing inverted index file: %s/%s",
				directoryName.c_str(), IndexConfig::invertedIndexFileName);
	}

	try {
		serializer.save(*this->quadTree,
				directoryName + "/" + IndexConfig::quadTreeFileName);
	} catch (exception &ex) {
		Logger::error("Error writing quadtree file: %s/%s",
				directoryName.c_str(), IndexConfig::quadTreeFileName);
	}

	try {
		this->saveCounts(
				directoryName + "/" + IndexConfig::indexCountsFileName);
	} catch (exception &ex) {
		Logger::error("Error writing index counts file: %s/%s",
				directoryName.c_str(), IndexConfig::indexCountsFileName);
	}

	try {
		serializer.save(*this->permissionMap,
				directoryName + "/" + IndexConfig::permissionMapFileName);
	} catch (exception &ex) {
		Logger::error("Error writing permissionMap file: %s/%s",
				directoryName.c_str(), IndexConfig::permissionMapFileName);
	}

	try{
		serializer.save(*(this->attributeAcl), directoryName + "/" + IndexConfig::AccessControlFile);
	} catch (exception &ex) {
		Logger::error("Error saving access control file: %s/%s", directoryName.c_str(),
				IndexConfig::AccessControlFile);
	}
}

void IndexData::printNumberOfBytes() const {
	Logger::debug("Number Of Bytes:");
	Logger::debug("Trie:\t\t %d bytes\t %.5f MB",
			this->trie->getNumberOfBytes(),
			(float) this->trie->getNumberOfBytes() / 1048576);
	Logger::debug("ForwardIndex:\t %d bytes\t %.5f MB",
			this->forwardIndex->getNumberOfBytes(),
			(float) this->forwardIndex->getNumberOfBytes() / 1048576);
	Logger::debug("InvertedIndex:\t %d bytes\t %.5f MB",
			this->invertedIndex->getNumberOfBytes(),
			(float) this->invertedIndex->getNumberOfBytes() / 1048576);
}

const Schema* IndexData::getSchema() const {
	return dynamic_cast<const Schema *>(this->schemaInternal);
}

Schema* IndexData::getSchema() {
	return dynamic_cast<Schema *>(this->schemaInternal);
}

void IndexData::loadCounts(const std::string &indeDataPathFileName) {
	std::ifstream ifs(indeDataPathFileName.c_str(), std::ios::binary);
	boost::archive::binary_iarchive ia(ifs);
	uint64_t readCount_tmp;
	uint32_t writeCount_tmp, numDocs_tmp;
	ia >> readCount_tmp;
	ia >> writeCount_tmp;
	ia >> numDocs_tmp;
	this->readCounter = new ReadCounter(readCount_tmp);
	this->writeCounter = new WriteCounter(writeCount_tmp, numDocs_tmp);
	ifs.close();
}

void IndexData::saveCounts(const std::string &indeDataPathFileName) const {
	std::ofstream ofs(indeDataPathFileName.c_str(), std::ios::binary);
	if (!ofs.good())
		throw std::runtime_error("Error opening " + indeDataPathFileName);
	boost::archive::binary_oarchive oa(ofs);
	uint64_t readCount_tmp = this->readCounter->getCount();
	uint32_t writeCount_tmp = this->writeCounter->getCount();
	uint32_t numDocs_tmp = this->writeCounter->getNumberOfDocuments();
	oa << readCount_tmp;
	oa << writeCount_tmp;
	oa << numDocs_tmp;
	ofs.close();
}

IndexData::~IndexData() {
	delete this->trie;
	delete this->forwardIndex;

	delete this->invertedIndex;
	delete this->quadTree;
	delete this->schemaInternal;
	delete this->readCounter;
	delete this->writeCounter;
	delete this->rankerExpression;
	delete this->permissionMap;
}

// Adds resource id to some of the role ids.
// for each role id if it exists in the permission map it will add this resource id to its vector
// otherwise it adds new record to the map with this role id and then adds this resource id to it.
void PermissionMap::appendResourceToRoles(const string &resourceId, vector<string> &roleIds){
	for(unsigned i = 0 ; i < roleIds.size() ; i++){
		map<string, vector<string> >::iterator it = permissionMap.find(roleIds[i]);
		if( it == permissionMap.end()){
			vector<string> resources;
			resources.push_back(resourceId);
			permissionMap.insert(std::pair<string,vector<string> >(roleIds[i],resources));
		}else{
			vector<string>::iterator rIt = std::find(it->second.begin(),it->second.end(),resourceId);
			if(rIt == it->second.end()){
				it->second.push_back(resourceId);
			}
		}
	}
}

// Deletes resource id from the role ids.
void PermissionMap::deleteResourceFromRoles(const string &resourceId, vector<string> &roleIds){
	for(unsigned i = 0 ; i < roleIds.size() ; i++){
		map<string, vector<string> >::iterator it = permissionMap.find(roleIds[i]);
		if( it != this->permissionMap.end()){
			vector<string>::iterator resourceIt = std::find(it->second.begin(),it->second.end(),resourceId);
			if( resourceIt != it->second.end()){
				*resourceIt = *(it->second.end() - 1);
				it->second.pop_back();
			}
		}
	}
}


}
}
