#include "ActiveNode.h"

namespace srch2
{
namespace instantsearch
{

void PrefixActiveNodeSet::getComputedSimilarPrefixes(const Trie *trie, std::vector<std::string> &similarPrefixes){
	for (std::map<const TrieNode*, PivotalActiveNode >::iterator mapIterator = PANMap.begin();
			mapIterator != PANMap.end(); mapIterator ++) {
		const TrieNode *trieNode = mapIterator->first;
		std::string prefix;
		trie->getPrefixString_NotThreadSafe(trieNode, prefix);
		similarPrefixes.push_back(prefix);
	}
}

PrefixActiveNodeSet *PrefixActiveNodeSet::computeActiveNodeSetIncrementally(const CharType additionalChar) {
	// form the new string. // TODO (OPT): avoid string copy
	std::vector<CharType> newString = this->prefix;
	newString.push_back(additionalChar);

	PrefixActiveNodeSet *newActiveNodeSet = new PrefixActiveNodeSet(newString, this->getEditDistanceThreshold(), this->trieRootNodeSharedPtr);

	// PAN:
	for (std::map<const TrieNode*, PivotalActiveNode >::const_iterator mapIterator = PANMap.begin();
			mapIterator != PANMap.end(); mapIterator ++) {
		// Compute the new active nodes for this trie node
		_addPANSetForOneNode(mapIterator->first, mapIterator->second, additionalChar, newActiveNodeSet);
	}

	return newActiveNodeSet;
}

void PrefixActiveNodeSet::printActiveNodes(const Trie* trie) const// Deprecated due to removal of TrieNode->getParent() pointers.
{
	typedef const TrieNode* trieNodeStar;
	std::map<trieNodeStar,PivotalActiveNode>::const_iterator mapIterater;
	std::cout << "QueryPrefix:" << this->prefix << "|PANMap:" << std::endl;
	for ( mapIterater  = this->PANMap.begin(); mapIterater != this->PANMap.end(); mapIterater++ )
	{
		trieNodeStar trieNode = mapIterater->first;
		string prefix;
		trie->getPrefixString(this->trieRootNodeSharedPtr->root, trieNode, prefix);
		std::cout << prefix << ":" << mapIterater->second.transformationdistance << std::endl;
	}
}

void PrefixActiveNodeSet::_addPANSetForOneNode(const TrieNode *trieNode, PivotalActiveNode pan,
            const CharType additionalChar, PrefixActiveNodeSet *newActiveNodeSet) {
        // deletion
	if(additionalChar < CHARTYPE_FUZZY_UPPERBOUND)
	{
		PivotalActiveNode dpan;
		dpan.transformationdistance = pan.transformationdistance + 1;
		dpan.differ = pan.differ + 1;
		dpan.editdistanceofPrefix = pan.editdistanceofPrefix;
		newActiveNodeSet->_addPAN(trieNode, dpan);
	}

	// go through the children of this treNode
	int depthLimit = this->getEditDistanceThreshold() - pan.editdistanceofPrefix;//transformationdistance; LGL: A bug
	int curDepth = 0;
	addPANUpToDepth(trieNode, pan, curDepth, depthLimit, additionalChar, newActiveNodeSet);

}

void PrefixActiveNodeSet::_addPAN(const TrieNode *trieNode, PivotalActiveNode pan) {
	if (pan.transformationdistance > this->editDistanceThreshold) // do nothing if the new distance is above the threshold
		return;
	//PAN:
	std::map<const TrieNode*, PivotalActiveNode >::iterator mapIterator = PANMap.find(trieNode);
	if (mapIterator != PANMap.end()) { // found one
		if (mapIterator->second.transformationdistance > pan.transformationdistance) // reassign the distance if it's smaller
			mapIterator->second = pan;
		else if (mapIterator->second.transformationdistance == pan.transformationdistance)
		{
			if((mapIterator->second.differ < pan.differ)||(mapIterator->second.editdistanceofPrefix > pan.editdistanceofPrefix))
				mapIterator->second = pan;
		}
		return; // otherwise, do nothing
	}

	// insert the new pair
	PANMap.insert(std::pair<const TrieNode*, PivotalActiveNode >(trieNode, pan));

	// set the flag
	this->trieNodeSetVectorComputed = false;
}

void PrefixActiveNodeSet::addPANUpToDepth(const TrieNode *trieNode, PivotalActiveNode pan, const unsigned curDepth, const unsigned depthLimit, const CharType additionalChar, PrefixActiveNodeSet *newActiveNodeSet) {
	// add children
	int max = curDepth;
	if( max < pan.differ )
		max = pan.differ;
	PivotalActiveNode panlocal;

	// if the character is larger than or equal to the upper bound or the depthLimit == 0, we directly
	// locate the child and add it to the pans
	// When depthLimit == 0 we do an exact search, so we don't need to go in the loop
	if(additionalChar >= CHARTYPE_FUZZY_UPPERBOUND || depthLimit == 0)
	{
		int childPosition = trieNode->findChildNodePosition(additionalChar);
		if(childPosition >= 0)
		{
			const TrieNode *child= trieNode->getChild(childPosition);
			panlocal.transformationdistance = pan.editdistanceofPrefix + max;
			panlocal.differ = 0;
			panlocal.editdistanceofPrefix = pan.editdistanceofPrefix + max;
			newActiveNodeSet->_addPAN(child, panlocal);
		}
		return;
	}
	for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
		const TrieNode *child = trieNode->getChild(childIterator);
		//if the current child's character is larger than the upper bound,
		// we no longer need to visit those later children.
		if(child->getCharacter() >= CHARTYPE_FUZZY_UPPERBOUND) break;
		if (child->getCharacter() == additionalChar) { // match
			panlocal.transformationdistance = pan.editdistanceofPrefix + max;
			panlocal.differ = 0;
			panlocal.editdistanceofPrefix = pan.editdistanceofPrefix + max;
			newActiveNodeSet->_addPAN(child, panlocal);
		}
		if (curDepth < depthLimit) {// recursive call for each child
			addPANUpToDepth(child, pan, curDepth+1, depthLimit, additionalChar, newActiveNodeSet);
		}
	}
}

}
}
