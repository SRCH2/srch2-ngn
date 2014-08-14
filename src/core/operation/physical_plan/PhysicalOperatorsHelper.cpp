#include "PhysicalOperatorsHelper.h"

#include "instantsearch/Term.h"
#include "operation/ActiveNode.h"
#include "operation/QueryEvaluatorInternal.h"
#include "src/core/util/RecordSerializerUtil.h"
#include "src/core/util/RecordSerializer.h"

using namespace std;

namespace srch2 {
namespace instantsearch {


/*
 * The following 4 functions are helper functions which are used in verifyByRandomAccess() implementation of
 * different operators. The first function (right below) is the main one which accesses forward index to see if
 * the record contains a term or not.
 */

bool verifyByRandomAccessHelper(QueryEvaluatorInternal * queryEvaluator, PrefixActiveNodeSet *prefixActiveNodeSet, Term * term, PhysicalPlanRandomAccessVerificationParameters & parameters){
	const vector<unsigned>& termSearchableAttributeIdToFilterTermHits = term->getAttributesToFilter();
	// assume the iterator returns the ActiveNodes in the increasing order based on edit distance

	if (term->getTermType() == srch2::instantsearch::TERM_TYPE_COMPLETE) {
		for(LeafNodeSetIteratorForComplete iter(prefixActiveNodeSet , term->getThreshold());
				!iter.isDone(); iter.next()){
			const TrieNode *trieNode;
			unsigned distance;
			iter.getItem(trieNode, distance);
			unsigned minId = trieNode->getMinId();
			unsigned maxId = trieNode->getMaxId();
			if (trieNode->isTerminalNode())
				maxId = minId;
			else
				ASSERT(false);  // ignore non-terminal nodes

			unsigned matchingKeywordId;
			float termRecordStaticScore;
			vector<unsigned> matchedAttributeIdsList;
			if (queryEvaluator->getForwardIndex()->haveWordInRange(parameters.forwardListDirectoryReadView,
					parameters.recordToVerify->getRecordId(),
					minId, maxId,
					termSearchableAttributeIdToFilterTermHits, term->getFilterAttrOperation(),
					matchingKeywordId, matchedAttributeIdsList, termRecordStaticScore)) {
				parameters.termRecordMatchingPrefixes.push_back(trieNode);
				parameters.attributeIdsList.push_back(matchedAttributeIdsList);
				parameters.prefixEditDistances.push_back(distance);
				bool isPrefixMatch = ( (!trieNode->isTerminalNode()) || (minId != matchingKeywordId) );
				parameters.runTimeTermRecordScore = parameters.ranker->computeTermRecordRuntimeScore(termRecordStaticScore, distance,
						term->getKeyword()->size(),
						isPrefixMatch,
						parameters.prefixMatchPenalty , term->getSimilarityBoost() * term->getBoost()) ;
				parameters.staticTermRecordScore = termRecordStaticScore ;
				parameters.termTypes.push_back(term->getTermType());
				// parameters.positionIndexOffsets ????
				return true;
			}
		}
		return false;

	}else{
		for (ActiveNodeSetIterator iter(prefixActiveNodeSet, term->getThreshold());
				!iter.isDone(); iter.next()) {
			const TrieNode *trieNode;
			unsigned distance;
			iter.getItem(trieNode, distance);

			unsigned minId = trieNode->getMinId();
			unsigned maxId = trieNode->getMaxId();
			if (term->getTermType() == srch2::instantsearch::TERM_TYPE_COMPLETE) {
				if (trieNode->isTerminalNode())
					maxId = minId;
				else
					continue;  // ignore non-terminal nodes
			}

			unsigned matchingKeywordId;
			float termRecordStaticScore;
			vector<unsigned> matchedAttributeIdsList;
			if (queryEvaluator->getForwardIndex()->haveWordInRange(parameters.forwardListDirectoryReadView,
					parameters.recordToVerify->getRecordId(),
					minId, maxId,
					termSearchableAttributeIdToFilterTermHits, term->getFilterAttrOperation(),
					matchingKeywordId, matchedAttributeIdsList, termRecordStaticScore)) {
				parameters.termRecordMatchingPrefixes.push_back(trieNode);
				parameters.attributeIdsList.push_back(matchedAttributeIdsList);
				parameters.prefixEditDistances.push_back(distance);
				bool isPrefixMatch = ( (!trieNode->isTerminalNode()) || (minId != matchingKeywordId) );
				parameters.runTimeTermRecordScore = parameters.ranker->computeTermRecordRuntimeScore(termRecordStaticScore, distance,
						term->getKeyword()->size(),
						isPrefixMatch,
						parameters.prefixMatchPenalty , term->getSimilarityBoost() * term->getBoost()) ;
				parameters.staticTermRecordScore = termRecordStaticScore ;
				parameters.termTypes.push_back(term->getTermType());
				// parameters.positionIndexOffsets ????
				return true;
			}
		}
		return false;

	}

}


bool verifyByRandomAccessAndHelper(PhysicalPlanOptimizationNode * node, PhysicalPlanRandomAccessVerificationParameters & parameters){
	// move on children and if at least on of them verifies the record return true
	vector<float> runtimeScore;
	// static score is ignored for now
	for(unsigned childOffset = 0 ; childOffset != node->getChildrenCount() ; ++childOffset){
		bool resultOfThisChild =
				node->getChildAt(childOffset)->getExecutableNode()->verifyByRandomAccess(parameters);
		runtimeScore.push_back(parameters.runTimeTermRecordScore);
		if(resultOfThisChild == false){
			return false;
		}
	}
	parameters.runTimeTermRecordScore = parameters.ranker->computeAggregatedRuntimeScoreForAnd(runtimeScore);

	return true;
}

bool verifyByRandomAccessOrHelper(PhysicalPlanOptimizationNode * node, PhysicalPlanRandomAccessVerificationParameters & parameters){
	// move on children and if at least on of them verifies the record return true
	bool verified = false;
	vector<float> runtimeScore;
	// static score is ignored for now
	for(unsigned childOffset = 0 ; childOffset != node->getChildrenCount() ; ++childOffset){
		bool resultOfThisChild =
				node->getChildAt(childOffset)->getExecutableNode()->verifyByRandomAccess(parameters);
		runtimeScore.push_back(parameters.runTimeTermRecordScore);
		if(resultOfThisChild == true){
			verified = true;
		}
	}
	if(verified == true){ // so we need to aggregate runtime and static score
		parameters.runTimeTermRecordScore = parameters.ranker->computeAggregatedRuntimeScoreForOr(runtimeScore);
	}
	return verified;
}

// this function used in verifyByrandomAccess functions of GeoNearestNeighborOperator, GeoSimpleScanOperator and RandomAccessVerificationGeoOperator
bool verifyByRandomAccessGeoHelper(PhysicalPlanRandomAccessVerificationParameters & parameters, QueryEvaluatorInternal * queryEvaluator, Shape* queryShape, unsigned &latOffset, unsigned &longOffset){
	// 1- get the forwardlist to get the location of the record from it
	bool valid = false;
	const ForwardList* forwardList = queryEvaluator->getForwardIndex()->getForwardList(
			parameters.forwardListDirectoryReadView,
			parameters.recordToVerify->getRecordId(),
			valid);

	if(!valid){ // this record is invalid
		return false;
	}

	// 2- find the latitude and longitude of this record
	StoredRecordBuffer buffer = forwardList->getInMemoryData();
	Point point;
	point.x = *((float *)(buffer.start.get() + latOffset));
	point.y = *((float *)(buffer.start.get() + longOffset));

	// verify the record. The query region should contains this record
	if(queryShape->contain(point)){
		parameters.geoFlag = true;
		parameters.geoScore = parameters.ranker->computeScoreforGeo(point,*(queryShape));
		return true;
	}
	return false;
}

// this function finds the offset of the latitude and longitude attributes in the refining attributes memory
void getLat_Long_Offset(unsigned & latOffset, unsigned & longOffset, Schema * schema){
	Schema * storedSchema = Schema::create();
	srch2::util::RecordSerializerUtil::populateStoredSchema(storedSchema, schema);
	srch2::util::RecordSerializer compactRecDeserializer = srch2::util::RecordSerializer(*storedSchema);

	// get the name of the attributes
	const string* nameOfLatitudeAttribute = schema->getNameOfLatituteAttribute();
	const string* nameOfLongitudeAttribute = schema->getNameOfLongitudeAttribute();

	unsigned idLat = storedSchema->getRefiningAttributeId(*nameOfLatitudeAttribute);
	latOffset = compactRecDeserializer.getRefiningOffset(idLat);

	unsigned idLong = storedSchema->getRefiningAttributeId(*nameOfLongitudeAttribute);
	longOffset = compactRecDeserializer.getRefiningOffset(idLong);
}

}
}
