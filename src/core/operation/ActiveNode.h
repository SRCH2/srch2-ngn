
// $Id: ActiveNode.h 3456 2013-06-14 02:11:13Z jiaying $

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

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#ifndef __ACTIVENODE_H__
#define __ACTIVENODE_H__

#include <stack>
#include "index/Trie.h"
#include "instantsearch/Term.h"
#include "util/BusyBit.h"
#include "util/Assert.h"
#include "util/Logger.h"
#include "src/core/geo/QuadTree.h"

using srch2::util::Logger;

namespace srch2
{
namespace instantsearch
{

/*
PAN:
 */
typedef struct _PivotalActiveNode {
    unsigned transformationdistance;
    unsigned short differ; // |p_{x+1}-p_i|
    unsigned short editdistanceofPrefix;
} PivotalActiveNode;


struct ResultNode {
    const TrieNode *node;
    int editDistance;
    int prefixLength;
    ResultNode(TrieNode* in_node):node(in_node) {}
    ResultNode(const TrieNode* in_node, int in_editDistance, int in_prefixLength):node(in_node),
            editDistance(in_editDistance),prefixLength(in_prefixLength) {}
};

// this comparison is based on preorder.
struct TrieNodePreOrderComparator {
    bool operator() (const TrieNode* t1, const TrieNode* t2) {
        return t1->getMinId() < t2->getMinId() || (t1->getMinId() == t2->getMinId() && t1->getMaxId() > t2->getMaxId());
    }
};

class PrefixActiveNodeSet
{
public:
    typedef std::vector<const TrieNode* > TrieNodeSet;
    typedef boost::shared_ptr<TrieRootNodeAndFreeList > TrieRootNodeSharedPtr;

private:
    std::vector<CharType> prefix;
    unsigned editDistanceThreshold;

    bool flagResultsCached;

    TrieRootNodeSharedPtr trieRootNodeSharedPtr;
    bool supportSwapInEditDistance;

    //PAN: A map from trie node to its pivotal active nodes
    std::map<const TrieNode*, PivotalActiveNode > PANMap;

    // group the trie nodes based on their edit distance to the prefix.
    // used only when it's called by an iterator
    std::vector<TrieNodeSet> trieNodeSetVector;
    bool trieNodeSetVectorComputed; // indicated if the trieNodeSetVector has been computed

public:

    void init(std::vector<CharType> &prefix, const unsigned editDistanceThreshold, const TrieRootNodeSharedPtr &trieRootNodeSharedPtr, bool supportSwapInEditDistance) {
        this->prefix = prefix;
        this->editDistanceThreshold = editDistanceThreshold;

        this->trieNodeSetVector.clear();
        this->trieNodeSetVectorComputed = false;

        this->flagResultsCached = false;

        this->trieRootNodeSharedPtr = trieRootNodeSharedPtr;
        this->supportSwapInEditDistance = supportSwapInEditDistance;
    };


    PrefixActiveNodeSet(std::vector<CharType> &prefix, const unsigned editDistanceThreshold, TrieRootNodeSharedPtr &trieRootNodeSharedPtr, bool supportSwapInEditDistance = true) {
        init(prefix, editDistanceThreshold, trieRootNodeSharedPtr, supportSwapInEditDistance);
    };

    /// A set of active nodes for an empty string and an edit-distance threshold
    PrefixActiveNodeSet(const TrieRootNodeSharedPtr &tsPtr, const unsigned editDistanceThreshold, bool supportSwapInEditDistance = true) {
        std::vector<CharType> emptyString;
        init(emptyString, editDistanceThreshold, tsPtr, supportSwapInEditDistance);

        //PAN:
        // Add the trie nodes up to the given depth
        const TrieNode *root = this->trieRootNodeSharedPtr->root;
        PivotalActiveNode pan;
        pan.transformationdistance = 0;
        pan.differ = 0; // |p_{x+1}-p_i|
        pan.editdistanceofPrefix = 0;
        _addPAN(root, pan);
        //if (editDistanceThreshold > 0)
        //    addTrieNodesUpToDepth(root, editDistanceThreshold, 0);


    };

    /// A set of active nodes for an empty string and an edit-distance threshold
    // Important note: this constructor makes a new copy from the Trie root shared pointer.
    // so it must NOT be used in real query execution because shared pointer must be what's
    // acquired in QueryEvalautorInternal's readerPreEnter() thorough query evaluation.
    PrefixActiveNodeSet(const Trie *trie, const unsigned editDistanceThreshold, bool supportSwapInEditDistance = true) {
        trie->getTrieRootNode_ReadView(this->trieRootNodeSharedPtr);
        std::vector<CharType> emptyString;
        init(emptyString, editDistanceThreshold, this->trieRootNodeSharedPtr, supportSwapInEditDistance);

        //PAN:
        // Add the trie nodes up to the given depth
        const TrieNode *root = this->trieRootNodeSharedPtr->root;
        PivotalActiveNode pan;
        pan.transformationdistance = 0;
        pan.differ = 0; // |p_{x+1}-p_i|
        pan.editdistanceofPrefix = 0;
        _addPAN(root, pan);
        //if (editDistanceThreshold > 0)
        //    addTrieNodesUpToDepth(root, editDistanceThreshold, 0);
    };

    virtual ~PrefixActiveNodeSet() {

    };

    boost::shared_ptr<PrefixActiveNodeSet> computeActiveNodeSetIncrementally(const CharType additionalChar);

    unsigned getEditDistanceThreshold() const {
        return editDistanceThreshold;
    }


    /*
     * Two prefix active nodes sets have the same Trie version if
     * the physical memory addresses of trieRootNodeSharedPtr are equal
     * in them.
     */
    bool hasTheSameVersionTrie(boost::shared_ptr<PrefixActiveNodeSet> right) const {
    	return this->trieRootNodeSharedPtr.get() == right->trieRootNodeSharedPtr.get();
    }
    bool hasTheSameVersionTrie(TrieRootNodeSharedPtr rightTrieRootNodeSharedPtr) const {
    	return this->trieRootNodeSharedPtr.get() == rightTrieRootNodeSharedPtr.get();
    }

    unsigned getNumberOfBytes() const {

    	unsigned totalNumberOfBytes = 0;
    	totalNumberOfBytes += sizeof(PrefixActiveNodeSet);

    	// prefix
    	totalNumberOfBytes += prefix.capacity() * sizeof(CharType);

    	// trieNodeSetVector : vector< vector< TrieNode * > >
    	totalNumberOfBytes += trieNodeSetVector.capacity() * sizeof(TrieNodeSet);
        for (std::vector<TrieNodeSet>::const_iterator vectorIter = trieNodeSetVector.begin();
        		vectorIter != trieNodeSetVector.end(); vectorIter++ ) {
        	totalNumberOfBytes += vectorIter->capacity() * sizeof(TrieNode*);
        	// Trie nodes are attached to Trie and have no additional memory for cache
        	// so we shouldn't actually consider their cost (no need to loop over trie node vector)
        }

        // PANMap
        // we assume the overhead of map is 32 bytes for each entry
        totalNumberOfBytes += PANMap.size() * (sizeof(TrieNode*) + sizeof(PivotalActiveNode) + 32);

       	// TrieRootNodeSharedPtr
       	// we assume that memory overhead of shared_ptr is 24 bytes.
       	totalNumberOfBytes += 24 +
       			trieRootNodeSharedPtr->free_list.capacity() * sizeof(TrieNode*) + sizeof(TrieNode*) ;

        return totalNumberOfBytes;

    }

    unsigned getNumberOfActiveNodes() {
        return (unsigned) PANMap.size();
    }

    std::vector<CharType> *getPrefix() {
        return &prefix;
    }

    std::string getPrefixUtf8String() {
        return getUtf8String(prefix);
    }

    unsigned getPrefixLength() const {
        return prefix.size();
    }

    // Deprecated due to removal of TrieNode->getParent() pointers.
    void getComputedSimilarPrefixes(const Trie *trie, std::vector<std::string> &similarPrefixes);

    //typedef std::vector<TrieNode* > TrieNodeSet;
    std::vector<TrieNodeSet> *getTrieNodeSetVector() {

        // compute it only if necessary
        if (this->trieNodeSetVectorComputed)
            return &trieNodeSetVector;

        _computeTrieNodeSetVector();
        return &trieNodeSetVector;
    }

    bool isResultsCached() const    {
        return this->flagResultsCached;
    }

    void setResultsCached(bool flag) {
        this->flagResultsCached = flag;
    }

    unsigned getEditdistanceofPrefix(const TrieNode *&trieNode) {
        std::map<const TrieNode*, PivotalActiveNode >::iterator iter = PANMap.find(trieNode);
        ASSERT(iter != PANMap.end());
        return iter->second.editdistanceofPrefix;
    }

    void printActiveNodes(const Trie* trie) const;// Deprecated due to removal of TrieNode->getParent() pointers.

    /*
     * This function prepares the 2D vector, trieNodeSetVector, which keeps all trie nodes ordered by their edit-distance.
     */
    void prepareForIteration(){
    	_computeTrieNodeSetVector();
    }
private:

    //PAN:

    /// compute the pivotal active nodes based on one of the active nodes of the previous prefix
    /// add the new pivotal active nodes to newActiveNodeSet
    void _addPANSetForOneNode(const TrieNode *trieNode, PivotalActiveNode pan,
                              const CharType additionalChar, PrefixActiveNodeSet *newActiveNodeSet);

    //PAN:
    /// Add a new pivotal active node with an edit distance.
    /// If the pivotal active node already exists in the set and had a distance no greater than the new one,
    /// then ignore this request.
    void _addPAN(const TrieNode *trieNode, PivotalActiveNode pan);


    //PAN:

    // add the descendants of the current node up to "depth" to the PANMao with
    // the corresponding edit distance.  The edit distance of the current node is "editDistance".
    void addPANUpToDepth(const TrieNode *trieNode, PivotalActiveNode pan, const unsigned curDepth, const unsigned depthLimit, const CharType additionalChar, PrefixActiveNodeSet *newActiveNodeSet);

    void _computeTrieNodeSetVector() {
        if (this->trieNodeSetVectorComputed)
            return;

        // VECTOR: initialize the vector
        this->trieNodeSetVector.resize(editDistanceThreshold + 1);
        for (unsigned i = 0; i <= editDistanceThreshold; i++)
            this->trieNodeSetVector[i].clear();

        // go over the map to populate the vectors.
        /*for (std::map<const TrieNode*, unsigned >::iterator mapIterator = trieNodeDistanceMap.begin();
                mapIterator != trieNodeDistanceMap.end(); mapIterator ++) {
            this->trieNodeSetVector[mapIterator->second].push_back(mapIterator->first);
        }*/

        for (std::map<const TrieNode*, PivotalActiveNode >::iterator mapIterator = PANMap.begin();
                mapIterator != PANMap.end(); mapIterator ++) {
            this->trieNodeSetVector[mapIterator->second.transformationdistance].push_back(mapIterator->first);
        }

        // set the flag
        this->trieNodeSetVectorComputed = true;
    }
};

class ActiveNodeSetIterator
{
public:
    // generate an iterator for the active nodes whose edit distance is within the given @edUpperBound
    ActiveNodeSetIterator(PrefixActiveNodeSet *prefixActiveNodeSet, const unsigned edUpperBound) {
        _initActiveNodeIterator(prefixActiveNodeSet, edUpperBound);
    }

    void next() {
        if (isDone())
            return;

        offsetCursor ++;

        if (offsetCursor < trieNodeSetVector->at(editDistanceCursor).size())
            return;

        // reached the tail of the current vector
        editDistanceCursor ++;
        offsetCursor = 0;

        // move editDistanceCursor to the next non-empty vector
        while (editDistanceCursor <= this->edUpperBound &&
                trieNodeSetVector->at(editDistanceCursor).size() == 0)
            editDistanceCursor ++;
    }

    bool isDone() {
        if (editDistanceCursor <= this->edUpperBound &&
                offsetCursor < trieNodeSetVector->at(editDistanceCursor).size())
            return false;

        return true;
    }

    size_t size(){
        size_t size = 0;
        for(unsigned i = 0; i< this->edUpperBound; i++)
            size += trieNodeSetVector->at(i).size();
        return size;
    }

    void getItem(const TrieNode *&trieNode, unsigned &distance) {
        if (isDone()) {
            trieNode = NULL;
            distance = 0;
        } else {
            trieNode = trieNodeSetVector->at(editDistanceCursor).at(offsetCursor);
            distance = editDistanceCursor;
        }

        //ASSERT(distance != 0);
    }

    // Get current active node. If we have finished the iteration, return NULL
    void getActiveNode(const TrieNode *&trieNode) {
        if (isDone()) {
            trieNode = NULL;
        } else {
            // Return the current active node, which is the offsetCursor-th member in the editDistanceCursor array
            trieNode = trieNodeSetVector->at(editDistanceCursor).at(offsetCursor);
        }
    }

private:
    typedef std::vector<const TrieNode* > TrieNodeSet;

    std::vector<TrieNodeSet> *trieNodeSetVector;
    unsigned edUpperBound;
    unsigned editDistanceCursor;
    unsigned offsetCursor;

    // initialize an iterator to store all the active nodes whose edit distance to
    // the query prefix is within the bound @edUpperBound
    void _initActiveNodeIterator(PrefixActiveNodeSet *prefixActiveNodeSet, const unsigned edUpperBound) {
        // we materialize the vector of trie nodes (indexed by the edit distance) only during the
        // phase of an iterator
        this->trieNodeSetVector = prefixActiveNodeSet->getTrieNodeSetVector();

        ASSERT(edUpperBound < trieNodeSetVector->size());
        this->edUpperBound = edUpperBound;

        // initialize the cursors
        this->editDistanceCursor = 0;
        // Find the first valid active node
        while (editDistanceCursor <= edUpperBound &&
                trieNodeSetVector->at(editDistanceCursor).size() == 0)
            editDistanceCursor ++;

        this->offsetCursor = 0;
    }

};

// a structure to record information of each node

struct LeafNodeSetIteratorItem {
    const TrieNode *prefixNode;
    const TrieNode *leafNode;
    unsigned distance;

    // TODO: OPT. Return the length of the prefix instead of the prefixNode pointer?
    LeafNodeSetIteratorItem(const TrieNode *prefixNode, const TrieNode *leafNode, unsigned distance) {
        this->prefixNode = prefixNode;
        this->leafNode = leafNode;
        this->distance = distance;
    }

};

/*
* for a set of active nodes, given a threshold edUpperBound, find
* all the leaf nodes whose minimal edit distance (among all their prefixes)
* is within the edUpperBound.
* Provide an iterator that can return the leaf nodes sorted by their
* minimal edit distance
*
* Implementation: 1. Get the set of active nodes with an edit distance <= edUpperBound, sorted
*                 based on their edit distance,
*                 2. get their leaf nodes, and keep track of the visited nodes so that
*
*                  each node is visited only once.
*/
class LeafNodeSetIteratorForPrefix
{
private:
    std::vector<LeafNodeSetIteratorItem > resultVector;
    unsigned cursor;

public:
    LeafNodeSetIteratorForPrefix(PrefixActiveNodeSet *prefixActiveNodeSet, const unsigned edUpperBound) {
        _initLeafNodeSetIterator(prefixActiveNodeSet, edUpperBound);
    }

    void next() {
        if (isDone())
            return;
        cursor ++;
    }

    bool isDone() {
        //if (cursor >= leafNodesVector.size())
        if (cursor >= resultVector.size())
            return true;
        return false;
    }

    size_t size() const{
        return resultVector.size();
    }

    void getItem(const TrieNode *&prefixNode, const TrieNode *&leafNode, unsigned &distance) {
        if (isDone()) {
            prefixNode = NULL;
            leafNode = NULL;
            distance = 0;
        } else {
            prefixNode = resultVector.at(cursor).prefixNode;
            leafNode = resultVector.at(cursor).leafNode;
            distance = resultVector.at(cursor).distance;
        }
    }

    // Deprecated due to removal of TrieNode->getParent() pointers.
    void printLeafNodes(const Trie* trie) const {
        typedef LeafNodeSetIteratorItem leafStar;
        typedef const TrieNode* trieStar;
        std::vector<leafStar>::const_iterator vecIter;
        Logger::debug("LeafNodes");
        for ( vecIter = this->resultVector.begin(); vecIter!= this->resultVector.end(); vecIter++ ) {
            leafStar temp = *vecIter;
            trieStar prefixNode = temp.leafNode;
            string prefix;
            trie->getPrefixString_NotThreadSafe(prefixNode, prefix);
            Logger::debug("Prefix: %s|Distance %d", prefix.c_str(), temp.distance);
        }
    }

private:
    void _initLeafNodeSetIterator(PrefixActiveNodeSet *prefixActiveNodeSet, const unsigned edUpperBound) {

        map<const TrieNode*, unsigned, TrieNodePreOrderComparator> activeNodes;
        const TrieNode *currentNode;
        unsigned distance;

        // assume the iterator returns the active nodes in an ascending order of their edit distance
        ActiveNodeSetIterator ani(prefixActiveNodeSet, edUpperBound);
        for (; !ani.isDone(); ani.next()) {
            ani.getItem(currentNode, distance);
            map<const TrieNode*, unsigned>::iterator nodeIter = activeNodes.find(currentNode);
            //if the current node is already in activeNodes, we keep the smaller edit distance of this node and the one already in activeNodes
            if (nodeIter != activeNodes.end())
                nodeIter->second = std::min<unsigned>(nodeIter->second, distance);
            else //else we will add it
                activeNodes[currentNode] = distance; // active nodes that are not visited.
        }
        // Use a stack to avoid recursive function calls.
        std::stack<std::pair<map<const TrieNode*, unsigned>::iterator, const TrieNode *> > activeNodeStack;
        // We are using an ordered map in which the active nodes are sorted in a pre-order. So this iterator returns the active nodes in an ascending order.
        for (map<const TrieNode*, unsigned>::iterator nextActiveNode = activeNodes.begin(), previousActiveNode; nextActiveNode != activeNodes.end();) {
            previousActiveNode = nextActiveNode;
            currentNode = previousActiveNode->first;
            nextActiveNode++;
            activeNodeStack.push(std::make_pair(previousActiveNode, currentNode));
            // non-recursive traversal
            while (!activeNodeStack.empty()) {
                // get and pop the top item
                std::pair<map<const TrieNode*, unsigned>::iterator, const TrieNode *> &stackTop = activeNodeStack.top();
                previousActiveNode = stackTop.first;
                currentNode = stackTop.second;
                activeNodeStack.pop();

                // check if this current trie node is the next active node in the set
                if (nextActiveNode != activeNodes.end() && currentNode == nextActiveNode->first) {
                    // If the next active node's ed is <= the ed of the previous active node, we will set change the previous node to the next node.
                    if (nextActiveNode->second <= previousActiveNode->second)
                        previousActiveNode = nextActiveNode;
                    nextActiveNode++;
                }
                if (currentNode->isTerminalNode())
                    resultVector.push_back(LeafNodeSetIteratorItem(previousActiveNode->first, currentNode, previousActiveNode->second));

                // push all the children from right to left to stack, so that after pop we can access them from left to right.
                for (int childIterator = currentNode->getChildrenCount()-1; childIterator >= 0; childIterator--)
                    activeNodeStack.push(std::make_pair(previousActiveNode, currentNode->getChild(childIterator)));
            }
        }
        // init the cursor
        cursor = 0;
    }

};

/*
 * This struct keeps the TrieNode and its edit-distance of terminal nodes in the LeafNodeSetIteratorForComplete.
 */
struct LeafNodeSetIteratorItemComplete {
    const TrieNode *leafNode;
    unsigned distance;

    LeafNodeSetIteratorItemComplete(const TrieNode *leafNode, unsigned distance) {
        this->leafNode = leafNode;
        this->distance = distance;
    }

};

/*
 * When keyword is complete, ANS consists of a terminal node (keyword itself) and
 * if it is also fuzzy a group of non-terminal nodes. This group of non-terminal nodes
 * must be used to find terminal nodes that are close to them (within the edit-distance threshold.)
 *
 */
class LeafNodeSetIteratorForComplete
{
private:
	map<const TrieNode*, unsigned, TrieNodePreOrderComparator> leafNodesWithinDistance;
	map<const TrieNode*, unsigned, TrieNodePreOrderComparator>::iterator cursor;

public:

    /*
     * This iterator traverses on the terminal nodes close by the active nodes of prefixActiveNodeSet
     * to find those within edit-distance threshold.
     */
    LeafNodeSetIteratorForComplete(PrefixActiveNodeSet *prefixActiveNodeSet, unsigned bound){
    	_initLeafNodeSetIterator(prefixActiveNodeSet, bound);
    }

    void next() {
        if (isDone())
            return;
        cursor ++;
    }

    bool isDone() {
        //if (cursor >= leafNodesVector.size())
        if (cursor == leafNodesWithinDistance.end())
            return true;
        return false;
    }

    size_t size() const{
        return leafNodesWithinDistance.size();
    }

    void getItem(const TrieNode *&leafNode, unsigned &distance) {
        if (isDone()) {
            leafNode = NULL;
            distance = 0;
        } else {
            leafNode = cursor->first;
            distance = cursor->second;
        }
    }

private:
    void _initLeafNodeSetIterator(PrefixActiveNodeSet *prefixActiveNodeSet, unsigned bound) {

        const TrieNode *trieNode;
        unsigned distance;

        ActiveNodeSetIterator ani(prefixActiveNodeSet, bound);
        for (; !ani.isDone(); ani.next()) {
            ani.getItem(trieNode, distance);
            unsigned panDistance = prefixActiveNodeSet->getEditdistanceofPrefix(trieNode);
			computeTerminalNodesWithinDistance(leafNodesWithinDistance,trieNode , distance, panDistance, bound);
        }

        cursor = leafNodesWithinDistance.begin();
    }

    /*
     * This is a recursive function which traverses the Trie to find closeby terminal nodes
     * within edit-distance threshold of trieNode
     */
    void computeTerminalNodesWithinDistance(map<const TrieNode*, unsigned, TrieNodePreOrderComparator> & activeNodesWithinEditDistance,
    		const TrieNode* trieNode, unsigned editDistance, unsigned panDistance, unsigned bound){
        if (trieNode->isTerminalNode()){
			map<const TrieNode*, unsigned, TrieNodePreOrderComparator>::iterator nodeIter = activeNodesWithinEditDistance.find(trieNode);
			unsigned realNewEditDistance = editDistance > panDistance ? editDistance : panDistance;
        	if(nodeIter != activeNodesWithinEditDistance.end()){ // use the minimal edis-distance
        		activeNodesWithinEditDistance[trieNode] = std::min<unsigned>(realNewEditDistance , activeNodesWithinEditDistance[trieNode]);
        	}else{
        		activeNodesWithinEditDistance[trieNode] = realNewEditDistance;
        	}
        }
        if (panDistance < bound) {
            for (unsigned int childIterator = 0; childIterator < trieNode->getChildrenCount(); childIterator++) {
                const TrieNode *child = trieNode->getChild(childIterator);
                computeTerminalNodesWithinDistance(activeNodesWithinEditDistance, child, editDistance, panDistance + 1, bound);
            }
        }
    }

};

/*
 *   This class keeps a readview of the quadtree (quadTreeRootNodeSharedPtr)
 *   So as long as we use the quadtree nodes in quadTreeNodeSet vector we have the readview.
 *   and it prevents to delete any node in this readview until we finish using this class.
 */
class GeoBusyNodeSet{
private:
	boost::shared_ptr<QuadTreeRootNodeAndFreeLists> quadTreeRootNodeSharedPtr;
	vector<QuadTreeNode*> quadTreeNodeSet;

public:
	GeoBusyNodeSet(boost::shared_ptr<QuadTreeRootNodeAndFreeLists> &quadTreeRootNodeSharedPtr){
		this->quadTreeRootNodeSharedPtr = quadTreeRootNodeSharedPtr;
	}

	void computeQuadTreeNodeSet(Shape& range){
		this->quadTreeRootNodeSharedPtr->root->rangeQuery(this->quadTreeNodeSet,range);
	}

	vector<QuadTreeNode*>* getQuadTreeNodeSet(){
		return &(this->quadTreeNodeSet);
	}

	unsigned sizeOfQuadTreeNodeSet(){
		return this->quadTreeNodeSet.size();
	}
};




}
}

#endif //__ACTIVENODE_H__
