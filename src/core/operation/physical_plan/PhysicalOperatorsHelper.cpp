#include "PhysicalOperatorsHelper.h"

#include "instantsearch/Term.h"
#include "operation/ActiveNode.h"
#include "operation/QueryEvaluatorInternal.h"

using namespace std;

namespace srch2 {
namespace instantsearch {

bool verifyByRandomAccessHelper(QueryEvaluatorInternal * queryEvaluator, PrefixActiveNodeSet *prefixActiveNodeSet, Term * term, PhysicalPlanRandomAccessVerificationParameters & parameters){
	unsigned termSearchableAttributeIdToFilterTermHits = term->getAttributeToFilterTermHits();
	// assume the iterator returns the ActiveNodes in the increasing order based on edit distance
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
		unsigned termAttributeBitmap;
		if (queryEvaluator->getForwardIndex()->haveWordInRange(parameters.recordToVerify->getRecordId(), minId, maxId,
				termSearchableAttributeIdToFilterTermHits,
				matchingKeywordId, termAttributeBitmap, termRecordStaticScore)) {
		    bool validForwardList;
		    queryEvaluator->getForwardIndex()->getForwardList(parameters.recordToVerify->getRecordId(), validForwardList);
		    if (validForwardList) {
				parameters.termRecordMatchingPrefixes.push_back(trieNode);
				parameters.attributeBitmaps.push_back(termAttributeBitmap);
				parameters.prefixEditDistances.push_back(distance);
				bool isPrefixMatch = ( (!trieNode->isTerminalNode()) || (minId != matchingKeywordId) );
				parameters.runTimeTermRecordScore = DefaultTopKRanker::computeTermRecordRuntimeScore(termRecordStaticScore, distance,
							term->getKeyword()->size(),
							isPrefixMatch,
							parameters.prefixMatchPenalty , term->getSimilarityBoost() ) ;
				parameters.staticTermRecordScore = termRecordStaticScore ;
				// parameters.positionIndexOffsets ????
				return true;
		    }
		}
	}
	return false;
}


float computeAggregatedRuntimeScoreForAnd(std::vector<float> runTimeTermRecordScores){

	float resultScore = 0;

	for(vector<float>::iterator score = runTimeTermRecordScores.begin(); score != runTimeTermRecordScores.end(); ++score){
		resultScore += *(score);
	}
	return resultScore;
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
	parameters.runTimeTermRecordScore = computeAggregatedRuntimeScoreForAnd(runtimeScore);

	return true;
}

float computeAggregatedRuntimeScoreForOr(std::vector<float> runTimeTermRecordScores){

	// max
	float resultScore = -1;

	for(vector<float>::iterator score = runTimeTermRecordScores.begin(); score != runTimeTermRecordScores.end(); ++score){
		if((*score) > resultScore){
			resultScore = (*score);
		}
	}
	return resultScore;
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
		parameters.runTimeTermRecordScore = computeAggregatedRuntimeScoreForOr(runtimeScore);
	}
	return verified;
}

}
}
