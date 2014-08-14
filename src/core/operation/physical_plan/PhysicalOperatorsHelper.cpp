#include "PhysicalOperatorsHelper.h"

#include "instantsearch/Term.h"
#include "operation/ActiveNode.h"
#include "operation/QueryEvaluatorInternal.h"

using namespace std;

namespace srch2 {
namespace instantsearch {


/*
 * The following 4 functions are helper functions which are used in verifyByRandomAccess() implementation of
 * different operators. The first function (right below) is the main one which accesses forward index to see if
 * the record contains a term or not.
 */

bool verifyByRandomAccessHelper(QueryEvaluatorInternal * queryEvaluator, PrefixActiveNodeSet *prefixActiveNodeSet, Term * term, PhysicalPlanRandomAccessVerificationParameters & parameters){
	unsigned termSearchableAttributeIdToFilterTermHits = term->getAttributeToFilterTermHits();
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
			unsigned termAttributeBitmap;
			if (queryEvaluator->getForwardIndex()->haveWordInRange(parameters.forwardListDirectoryReadView,
					parameters.recordToVerify->getRecordId(),
					minId, maxId,
					termSearchableAttributeIdToFilterTermHits,
					matchingKeywordId, termAttributeBitmap, termRecordStaticScore)) {
				//// checking the access list
				parameters.termRecordMatchingPrefixes.push_back(trieNode);
				parameters.attributeBitmaps.push_back(termAttributeBitmap);
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
			unsigned termAttributeBitmap;
			if (queryEvaluator->getForwardIndex()->haveWordInRange(parameters.forwardListDirectoryReadView,
					parameters.recordToVerify->getRecordId(),
					minId, maxId,
					termSearchableAttributeIdToFilterTermHits,
					matchingKeywordId, termAttributeBitmap, termRecordStaticScore)) {
				parameters.termRecordMatchingPrefixes.push_back(trieNode);
				parameters.attributeBitmaps.push_back(termAttributeBitmap);
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

}
}
