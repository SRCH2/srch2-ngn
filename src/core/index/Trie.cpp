/* $Id$
 *
 * Trie.cpp
 *
 *  Created on: 2013-4-6
 *      Author: Jiaying Wang
 */

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

#include "index/Trie.h"
#include "util/Logger.h"
using srch2::util::Logger;
// we need to include inverted index in here to get information about list frequencies to do query suggestions
#include "index/InvertedIndex.h"
#include "index/ForwardIndex.h"

namespace srch2
{
namespace instantsearch
{
// for boost serialization
TrieNode::TrieNode()
{
    this->leftMostDescendant = NULL;
    this->rightMostDescendant = NULL;
    this->id = 0;
    this->setNodeProbabilityValue(0);
    this->setMaximumScoreOfLeafNodes((half)0);
    this->setNumberOfTerminalNodes(0);
    this->invertedListOffset = 0;
    //this->character = '$'; // dummy character. charT on depth=0 is always invalid.
    this->setDepth(0);
    this->setTerminalFlag(false);
    this->insertCounters = 0;
    this->setLeftInsertCounter(1); // default values: 1
    this->setRightInsertCounter(1);
    this->isCopy = false;
}

TrieNode::TrieNode(bool create_root)
{
    if (!create_root)
        return;

    this->leftMostDescendant = NULL;
    this->rightMostDescendant = NULL;
    this->id = 0;
    this->setNodeProbabilityValue(0);
    this->setMaximumScoreOfLeafNodes((half)0);
    this->setNumberOfTerminalNodes(0);
    this->invertedListOffset = 0;
    this->character = TRIE_MARKER_CHARACTER; // dummy character. charT on depth=0 is always invalid.
    this->setDepth(0);
    this->setTerminalFlag(false);
    this->insertCounters = 0;
    this->setLeftInsertCounter(1); // default values: 1
    this->setRightInsertCounter(1);
    this->isCopy = false;
}

TrieNode::TrieNode(int depth, CharType character, bool isCopy)
{
    this->leftMostDescendant = NULL;
    this->rightMostDescendant = NULL;
    this->id = 0;
    this->setNodeProbabilityValue(0);
    this->setMaximumScoreOfLeafNodes((half)0);
    this->setNumberOfTerminalNodes(0);
    this->invertedListOffset = 0;
    this->character = character;

    this->setDepth(depth);
    this->setTerminalFlag(false);
    this->insertCounters = 0;
    this->setLeftInsertCounter(1); // default values: 1
    this->setRightInsertCounter(1);
    this->isCopy = isCopy;
}

TrieNode::TrieNode(const TrieNode *src, bool isCopy)
{
    this->character = src->character;
    this->id = src->id;
    this->setNodeProbabilityValue(src->getNodeProbabilityValue());
    this->setMaximumScoreOfLeafNodes(src->getMaximumScoreOfLeafNodes());
    this->setNumberOfTerminalNodes(src->getNumberOfTerminalNodes());
    this->invertedListOffset = src->invertedListOffset;
    this->leftMostDescendant = src->leftMostDescendant;
    this->rightMostDescendant = src->rightMostDescendant;
    this->terminalFlag1bDepth7b = src->terminalFlag1bDepth7b;
    this->childrenPointerList.resize(src->childrenPointerList.size());
    std::copy(src->childrenPointerList.begin(), src->childrenPointerList.end(), this->childrenPointerList.begin());
    this->insertCounters = 0;
    this->setLeftInsertCounter(src->getLeftInsertCounter());
    this->setRightInsertCounter(src->getRightInsertCounter());
    this->isCopy = isCopy;
}

TrieNode::~TrieNode()
{
    /*for (unsigned childIterator = 0; childIterator < this->getChildrenCount(); childIterator++ )
    {
        delete this->getChild(childIterator);
    }*/

    this->childrenPointerList.clear();
    this->leftMostDescendant = NULL;
    this->rightMostDescendant = NULL;
    //this->parent = NULL;
}


bool TrieNode::isRoot() const
{
    if (this->getDepth() == 0) {
        ASSERT(this->character == TRIE_MARKER_CHARACTER);
        return true;
    }

    return false;
}

void TrieNode::print_TrieNode() const
{
    if (this == NULL)
        return;

    Logger::debug("X|%d|%d|%d|%d",
                  this->id, this->invertedListOffset,
                  this->getCharacter(), this->getDepth());
    if (this->leftMostDescendant != NULL) {
        Logger::debug("|min|%d", this->getMinId());
    }
    if (this->leftMostDescendant != NULL) {
        Logger::debug("|max|%d", this->getMaxId());
    }

    for (unsigned it =0; it != this->childrenPointerList.size(); ++it) {
        const TrieNode* childNode = this->childrenPointerList.at(it);
        Logger::debug("%d" ,childNode->character);
    }
}

bool trieNodeComparatorBasedOnProbabilityValue(const TrieNode * left , const TrieNode * right){
	return left->getNodeProbabilityValue() > right->getNodeProbabilityValue();
}

// this function uses a weighted DFS (which means children are visited based on their probabilityValue) and collects all frontier terminal nodes in its way.
// stopping condition is that the number of terminal nodes are >= numberOfSuggestionsToFind
void TrieNode::findMostPopularSuggestionsInThisSubTrie(const TrieNode * suggestionActiveNode, unsigned ed, vector< SuggestionInfo > & suggestions,
		const int numberOfSuggestionsToFind) const{

	vector<const TrieNode *> nonTerminalChildrenVector;
	//1. First iterate on children and add terminal children to suggestions.
	// in the same time insert non-terminal nodes to a heap
	for(int childIterator =0; childIterator< this->getChildrenCount() ; childIterator ++){
		const TrieNode * child = this->getChild(childIterator);
		if(child->isTerminalNode()){
			suggestions.push_back(SuggestionInfo(ed , child->getNodeProbabilityValue() , suggestionActiveNode, child));
		}else{
			nonTerminalChildrenVector.push_back(child);
		}
	}
	// sort the non-terminal nodes.
	std::sort(nonTerminalChildrenVector.begin() , nonTerminalChildrenVector.end() , trieNodeComparatorBasedOnProbabilityValue);
	// 2. Now move on non-terminal children in descending order based on their probability value
	// and call this function (recursive call)
	for(vector<const TrieNode *>::iterator nonTerminalChild = nonTerminalChildrenVector.begin() ;
			nonTerminalChild != nonTerminalChildrenVector.end() ; ++nonTerminalChild){
		(*nonTerminalChild)->findMostPopularSuggestionsInThisSubTrie(suggestionActiveNode, ed , suggestions , numberOfSuggestionsToFind);
		if(suggestions.size() >= numberOfSuggestionsToFind){
			return;
		}

	}
	return;
}


void TrieNode::addChild(CharType character, TrieNode *childNode)
{
    int childPosition = findChildNodePosition(character);
    if (childPosition > 0) // if it is already there, do nothing
        return;
    childrenPointerList.insert(childrenPointerList.begin() + (-childPosition - 1), childNode);
}

void TrieNode::addChild(int position, TrieNode *childNode)
{
    childrenPointerList.insert(childrenPointerList.begin() + position, childNode);
}

// Use carefully to save from memory leaks, used only in non-blocking single writer mode.
void TrieNode::setChild(int position, TrieNode* childNode)
{
    assert((unsigned)position < this->childrenPointerList.size());
    //assert(position < this->childPointerList.size());
    this->childrenPointerList.at(position) = childNode;
}

TrieNode *TrieNode::findChild(CharType childCharacter) const
{
    //assert(childrenPointerList.size() > 0);
    int childPosition = findChildNodePosition(childCharacter);
    if (childPosition >= 0) // if it is already there, do nothing
        return childrenPointerList[childPosition];
    return NULL; // failed to find chidlNode
}

int TrieNode::findChildNodePosition(CharType childCharacter) const
{
    int first = 0;
    int last = childrenPointerList.size() - 1;
    while (first <= last) {
        int mid = (first + last) / 2;  // compute mid point.
        if (childCharacter > childrenPointerList[mid]->character)
            first = mid + 1;  // repeat search in top half.
        else if (childCharacter < childrenPointerList[mid]->character)
            last = mid - 1; // repeat search in bottom half.
        else
            return mid;     // found it. return position
    }
    return -(first + 1);    // failed to find key
}

unsigned TrieNode::getByteSizeOfCurrentNode() const
{
	return sizeof(TrieNode) + childrenPointerList.capacity() * sizeof(TrieNode *);
}


unsigned TrieNode::getNumberOfBytes() const
{
    if (this == NULL) {
        return(0);
    } else {
        unsigned int childIterator = 0;
        unsigned int sizeCounter = 0;
        sizeCounter += this->getByteSizeOfCurrentNode();
        for ( ; childIterator < this->getChildrenCount(); childIterator++ ) {
            sizeCounter += this->getChild(childIterator)->getNumberOfBytes();
        }
        return sizeCounter;
    }
}

unsigned TrieNode::getNumberOfNodes() const
{
    if (this == NULL) {
        return 0;
    } else {
        unsigned int childIterator = 0;
        unsigned int counter = 0;
        counter += 1;
        for ( ; childIterator < this->getChildrenCount(); childIterator++ ) {
            counter += this->getChild(childIterator)->getNumberOfNodes();
        }
        return counter;
    }
}

unsigned TrieNode::getfinalKeywordIdCounter() const
{
    if (this == NULL) {
        return(0);
    } else {
        unsigned int childIterator = 0;
        unsigned int counter = 0;
        if (this->isTerminalNode()) {
            counter += 1;
        }
        for (; childIterator < this->getChildrenCount(); childIterator++) {
            counter += this->getChild(childIterator)->getfinalKeywordIdCounter();
        }
        return counter;
    }
}

TrieNode *TrieNode::findLowerBoundChildByMinId(unsigned minId) const
{
    int childPosition = findLowerBoundChildNodePositionByMinId(minId);
    if (childPosition >= 0) // if it is already there, do nothing
        return childrenPointerList[childPosition];
    return NULL; // failed to find chidlNode
}

int TrieNode::findLowerBoundChildNodePositionByMinId(unsigned minId) const
{
    int first = 0;
    int last = childrenPointerList.size() - 1;
    while (first <= last) {
        int mid = (first + last) / 2;  // compute mid point.
        int midMinId = childrenPointerList[mid]->getMinId();
        if (minId > midMinId)
            first = mid + 1;  // repeat search in top half.
        else if (minId < midMinId)
            last = mid - 1; // repeat search in bottom half.
        else
            return mid;     // found it. return position
    }
    return first - 1;    // failed to find key, return lower bound
}

void TrieNode::resetCopyFlag(){
	// We don't need concurrency control for this flag, since the
	// flag is only used by writers (not readers), and there can
	// be only one writer in the system at any time. 
	this->isCopy = false;
	// If one child has isCopy == false, then all the nodes under
	// that subtrie should have isCopy == false. 
	// The reason is that trie nodes were copied starting from the
	// root during insertions, and it's impossible to have 
	// a trie node with isCopy == true while its parent has isCopy
	// == false. 
	for(unsigned i = 0; i < this->childrenPointerList.size() ; i++){
		if(this->childrenPointerList[i] != NULL && this->childrenPointerList[i]->isCopy == true){
			this->childrenPointerList[i]->resetCopyFlag();
		}
	}
}



TrieRootNodeAndFreeList::TrieRootNodeAndFreeList()
{
    bool create_root = true;
    this->root = new TrieNode(create_root);
}

TrieRootNodeAndFreeList::TrieRootNodeAndFreeList(const TrieNode *src)
{
    this->root = new TrieNode(src);
}


TrieRootNodeAndFreeList::~TrieRootNodeAndFreeList()
{
    // Delete free_list members
    for (vector<const TrieNode* >::iterator it = free_list.begin();
            it != free_list.end(); ++it) {
        delete *it;
    }
    delete root;
}

TrieNodePath::TrieNodePath()
{
    this->path = NULL;
}

TrieNodePath::TrieNodePath(vector<TrieNode* > *p)
{
    this->path = p;
}

void TrieNodePath::clean()
{
    if (this->path != NULL)
        delete this->path;
    this->path = NULL;
}

// get the last trie node on the path
TrieNode * TrieNodePath::getLastTrieNode() const
{
    if (path == NULL)
        return NULL;

    return path->back();
}


Trie::Trie()
{
    // We create a root (for the write view) by copying the trie root of the read view.
    // Initially both root views have an empty trie with a "$" sign at the root.
    this->root_readview.reset( new TrieRootNodeAndFreeList() );
    this->root_writeview = new TrieNode(this->root_readview.get()->root);
    this->numberOfTerminalNodes = 0;
    this->oldIdToNewIdMapVector = NULL;
    this->commited = false;
    this->mergeRequired = 0;

    this->counterForReassignedKeywordIds = MAX_ALLOCATED_KEYWORD_ID + 1; // init the counter
    pthread_spin_init(&m_spinlock, 0);
}

Trie::~Trie()
{
    if (this->oldIdToNewIdMapVector != NULL)
        delete this->oldIdToNewIdMapVector;

    TrieNode *root = this->root_writeview;

    for (unsigned childIterator = 0; childIterator < root->getChildrenCount(); ++childIterator ) {
        TrieNode* childNode = root->getChild(childIterator);
        this->deleteTrieNode( childNode );
    }

    /* Free root_writeview pointer. The if condition is a defensive check to make sure
     * that we do not get into a double free situation. 'root_readview' is a shared_pointer
     * which automatically deletes the pointer it is holding.
     */
    if (this->root_writeview != this->root_readview->root
    		&& this->root_writeview != NULL)
        delete this->root_writeview;

    pthread_spin_destroy(&m_spinlock);
}

void Trie::deleteTrieNode(TrieNode* &trieNode)
{
    if (trieNode == NULL)
        return;
    else {
        for (unsigned childIterator = 0; childIterator < trieNode->getChildrenCount(); ++childIterator ) {
            TrieNode* childNode = trieNode->getChild(childIterator);
            this->deleteTrieNode( childNode );
        }
    }
    delete trieNode;
    trieNode = NULL;
}

void Trie::getTrieRootNode_ReadView(boost::shared_ptr<TrieRootNodeAndFreeList >& trieRootNode_ReadView) const
{
    pthread_spin_lock(&m_spinlock);
    trieRootNode_ReadView = this->root_readview;
    pthread_spin_unlock(&m_spinlock);
}

TrieNode* Trie::getTrieRootNode_WriteView() const
{
    return this->root_writeview;
}

// Helper function for addKeyword function. Called by both addKeyword(...) and addKeyword_ThreadSafe(...)
// The leaf (or possibly internal) node is added (or turned marked to be a complete keyword) but it doesn't have an ID, this function
// computes the id to be assigned to the new keyword and also modifies the left and right most descendants of its ancestors if necessary.
void Trie::addKeyword_SetPrevIdNexIdByPathTrace(vector<TrieNode* > &pathTrace,
        bool isNewTrieNode, TrieNode *node)
{
    TrieNode* prevNode = NULL;
    TrieNode* nextNode = NULL;
    TrieNodePath currentTnp(&pathTrace);

    // Jamshid : It means we are passed the load phase. we are doing incremental update
    if (this->commited) {
        // Find previous and next keyword id to assign a id to the current keyword.
        // Note: getPreviousKeywordId and getNextKeywordId are not symmetric.
        // The Asymmetry is caused by the following corner case:
        // If the root of the a subTrie is a terminal node, its id is the minId of the subTrie.

        // This block of code is used to get the previous keyword id.
        TrieNodePath prevTnp;
        if (getLeftTnp(currentTnp, prevTnp) == false) { // leftmost string in the trie
            prevNode = NULL;
        } else {
            //prevNode = prevTnp.path->at(prevTnp.path->size() - 1);
            prevNode = prevTnp.path->back();
        }
        prevTnp.clean();

        // This block of code is used to get the nextKeywordID

        // Corner case: an already existing trie node is made into a terminal node.
        // For example, say "cats" keyword is already in Trie and we
        // add "cat". No new trie nodes are created by we 
        // make an existing trienode into a terminal node.
        if (!isNewTrieNode) {
            unsigned pathTraceIterator = pathTrace.size() - 1;
            TrieNode *terminalNode = pathTrace.at(pathTraceIterator);
            nextNode = terminalNode->getLeftMostDescendant();
        }
        // Usual case, to get the nextKeywordID
        else {
            TrieNodePath nextTnp;
            if (getRightTnp(currentTnp, nextTnp) == false) { // rightmost string in the trie
                nextNode = NULL;
            } else {
                nextNode = nextTnp.path->at(nextTnp.path->size() - 1);
            }
            nextTnp.clean();
        }
    }

    // This block of code sets the leftRightMostDescendant of the trieNodes
    unsigned pathTraceIterator = pathTrace.size() - 1;
    TrieNode *terminalNode = pathTrace.at(pathTraceIterator);
    TrieNode *childNode = terminalNode;

    // For the terminal node, it is its own leftMostNode
    terminalNode->setLeftMostDescendant(terminalNode);

    // If child count is zero, it is its own rightMostNode
    if (terminalNode->getChildrenCount() == 0)
        terminalNode->setRightMostDescendant(terminalNode);
    else //If there are children, currentNode's rightMostChild is the lastChild's rightMostDescendant
        terminalNode->setRightMostDescendant(terminalNode->getChild(terminalNode->getChildrenCount() - 1)->getRightMostDescendant() );

    // Iterate up the trie and set the leftrightMostChild pointers where necessary.
    bool isDone = false;
    while (!isDone) {
        --pathTraceIterator;
        TrieNode *parentNode = pathTrace.at(pathTraceIterator);
        bool isFirstChild = parentNode->isFirstChild(childNode);
        bool isLastChild = parentNode->isLastChild(childNode);

        if (isFirstChild && !parentNode->isTerminalNode() )
            parentNode->setLeftMostDescendant(childNode->getLeftMostDescendant());

        if (isLastChild )
            parentNode->setRightMostDescendant(childNode->getRightMostDescendant());

        childNode = parentNode;

        if (pathTraceIterator == 0)
            isDone = true;
    }

    /// if the node previously was not a terminal node, set it to a terminal node and also assign it the keywordId
    if (!(node->isTerminalNode())) {
        // before commit we just set a temperory ID, then during commit phase all the keywords get a new ID
        if (!this->commited) {
            node->setId(this->numberOfTerminalNodes);
            node->setInvertedListOffset(this->numberOfTerminalNodes);
        } else {
            unsigned newKeywordId = this->computeIdForNewKeyword(prevNode, nextNode);

            // check if the new id for the keyword is preserving the order
            if (prevNode != NULL && nextNode != NULL
                    && newKeywordId > prevNode->getId() && newKeywordId < nextNode->getId()) {
                node->setId(newKeywordId);
            } else {
                // assign a new id for the new keyword
                // this->counterForReassignedKeywordIds starts from a very large number, it's a temp id
                node->setId(this->counterForReassignedKeywordIds);
                this->counterForReassignedKeywordIds++;
                ASSERT(this->counterForReassignedKeywordIds != MAX_KEYWORD_ID); // we are using MAX_KEYWORD_ID as a special value in geo update

                // remember this keyword and its trie node
                vector<TrieNode* > *newPathTrace = new vector<TrieNode* >(pathTrace);
                trieNodesToReassign.push_back(TrieNodePath(newPathTrace));

                printTriePath(newPathTrace);
            }

            // inverted list offset is an offset in a directory, it's not eqaul to keyword ID
            node->setInvertedListOffset(this->numberOfTerminalNodes);
        }

        node->setTerminalFlag(true);
        this->numberOfTerminalNodes += 1;
    }
}

// Test if there are keywords whose ids need to be reassigned
bool Trie::needToReassignKeywordIds()
{
    if (trieNodesToReassign.size() > 0)
        return true;

    return false;
}

void Trie::printTriePath(vector<TrieNode* > *pathTrace)
{
    /* cout << "trie path = "; */
    /* if (pathTrace == NULL) */
    /*   cout << " NULL "; */
    /* else { */
    /*   for (unsigned i = 0; i < pathTrace->size(); i ++) */
    /*     cout << pathTrace->at(i)->character; */
    /*   cout << ". Leaf node id = " << pathTrace->at(pathTrace->size() - 1)->getId() << "\n"; */
    /* } */
}

unsigned Trie::computeIdForNewKeyword(TrieNode* prevNode, TrieNode* nextNode)
{
    if (prevNode == NULL || nextNode == NULL)
        return 0;

    unsigned prevId = prevNode->getId();
    unsigned nextId = nextNode->getId();

    //corner cases
    // ??? what is the first condition ? Is it not covered by the next two conditions ?
    if (prevId >= nextId ||
            isLargerThanUpperBound(prevId) ||
            isLargerThanUpperBound(nextId) ) // prevNode or nextNode is waiting to be reassigned
        return prevId;

    // Normal case. Compute the weighted centroid.
    unsigned prevWeight, nextWeight;


    // This mechanism is used to insert new ids uniformly in the space between two keywords.
    // increment the two counters
    prevWeight = prevNode->getRightInsertCounter();
    prevNode->setRightInsertCounter(prevWeight + 1);

    nextWeight = nextNode->getLeftInsertCounter();
    nextNode->setLeftInsertCounter(nextWeight + 1);

    //unsigned newKeywordId = (prevId/2) + (nextId/2); // naive way: take the average
    unsigned newKeywordId =
        (unsigned)(prevId + (double)(nextId - prevId) * ((double)prevWeight / ((double) prevWeight + (double)nextWeight)));

    return newKeywordId;
}

// Remove those characters whose ascii is equal to
// the trie's marker character.
// If the keyword has more than 127 characters, just take
// the first 127 characters, since that's the limit of the
// trie depth.
void Trie::cleanString(const vector<CharType> &oldString, vector<CharType> &cleanedString) const
{
    cleanedString.clear();
    for (unsigned i = 0; i < oldString.size() && i < this->TRIE_MAX_DEPTH; i ++) {
        CharType c = oldString.at(i);
        // enable it if we only allow characters whose ASCII code <= 127
        //if ((c & 0x80) == 0 && c != TRIE_MARKER_CHARACTER)
        if (c != TRIE_MARKER_CHARACTER)
            cleanedString.push_back(c);
    }
}


/**
 * insert the keyword into the trie
 * returns the keywordID of entered keyword. keywordIDs start from 0.
 *
 * If the keyword is empty, returns 0, as empty srting is not a valid input string.
 *
 * Note that first entered valid keyword will have keywordid 0.
 *
 * Do not use after calling commit
 */
unsigned Trie::addKeyword(const std::vector<CharType> &keyword, unsigned &invertedListOffset)
{
    /// corner case to check invalid empty string
    if (this->commited || keyword.size() == 0)
        return 0;

    TrieNode *node = this->getTrieRootNode_WriteView();
    int depthCounter = 1;

    vector<CharType> cleanedString;
    cleanString(keyword, cleanedString); // remove bad characters


    bool isNewTerminalNode = false;
    vector<TrieNode* > pathTrace;
    vector<CharType>::iterator charTypeIterator = cleanedString.begin();
    while ( charTypeIterator != cleanedString.end()) {
        assert (node != NULL);
        TrieNode *childNode;

        int childPosition = node->findChildNodePosition((CharType) *charTypeIterator);
        if (childPosition >= 0 && !isNewTerminalNode)
            childNode = node->getChild(childPosition);
        else {
            // create a TrieNode with terminal flag set to false.
            childNode = new TrieNode(depthCounter, (CharType) *charTypeIterator);
            node->addChild(-childPosition-1, childNode);
            isNewTerminalNode = true;
        }
        pathTrace.push_back(node);
        node = childNode;

        ++depthCounter;
        charTypeIterator++;
    }
    pathTrace.push_back(node);
    printTriePath(&pathTrace);

    this->addKeyword_SetPrevIdNexIdByPathTrace(pathTrace, isNewTerminalNode, node);

    // return invertedListOffset and keywordId
    invertedListOffset = node->getInvertedListOffset();
    return node->getId();
}

unsigned Trie::addKeyword(const std::string &keyword, unsigned &invertedListOffset)
{
    return addKeyword(getCharTypeVector(keyword), invertedListOffset);
}


// We add this extra level for addKeyword_ThreadSafe, because
// 1) the GeoIndex update needs isNewTerminalNode, so it can call addKeyword_ThreadSafe_Inner
// 2) we don't want to change how other places use addKeyword_ThreadSafe
unsigned Trie::addKeyword_ThreadSafe(const std::vector<CharType> &keyword, unsigned &invertedListOffset)
{
    bool isNewTrieNode = false;
    bool isNewInternalTerminalNode = false;
    return addKeyword_ThreadSafe(keyword, invertedListOffset, isNewTrieNode, isNewInternalTerminalNode);
}

unsigned Trie::addKeyword_ThreadSafe(const std::vector<CharType> &keyword, unsigned &invertedListOffset, bool &isNewTrieNode, bool &isNewInternalTerminalNode)
{
    /// corner case to check invalid empty string
    if (keyword.size() == 0)
        return 0;

    this->mergeRequired = true;

    boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNode_ReadView;
    this->getTrieRootNode_ReadView(trieRootNode_ReadView);

    // ThreadSafe
    TrieNode *node;
    int depthCounter = 1;

    vector<CharType> cleanedString;
    cleanString(keyword, cleanedString); // remove bad characters


    // it's a map from original nodes in the trie to the copy nodes in the pathTrace
    // The reason we need this map is related to reassign ID. Since we create a new cloned path each time we
    // add a keyword, we need this map from original nodes to new nodes to merge new paths (to map original nodes to
    // same new nodes ...)
    OldToNewTrieNodeMap oldToNewTrieNodeMap;
    // it keeps a copy of the path to the new node
    vector<TrieNode* > pathTrace;

    TrieNode *nodeCopy  = this->getTrieRootNode_WriteView();
    vector<CharType>::iterator charTypeIterator = cleanedString.begin();
    while (charTypeIterator != cleanedString.end()) {
        assert (nodeCopy != NULL);

        TrieNode *childNodeCopy;
        int childPosition = nodeCopy->findChildNodePosition((CharType) *charTypeIterator);

        if (childPosition >= 0 /*&& !isNewTrieNode*/) {
            ASSERT(isNewTrieNode == false);
            // clone a new trie node
            TrieNode *childNode = nodeCopy->getChild(childPosition);

	    // If the child node is already a copy from the read view,
	    // we don't need to copy it again during the new insertion. 
            if(childNode->isCopy){
            	childNodeCopy = childNode;
            }else{
            	childNodeCopy = new TrieNode(childNode, true);
            	oldToNewTrieNodeMap[childNode] = childNodeCopy; // remember the mapping
            	nodeCopy->setChild(childPosition, childNodeCopy);

            	// Add to free_list for future deletion
            	trieRootNode_ReadView->free_list.push_back(childNode);
            }

        } else {
            // create a TrieNode with terminal flag set to false.
            childNodeCopy = new TrieNode(depthCounter, (CharType) *charTypeIterator, true);
            nodeCopy->addChild(-childPosition-1, childNodeCopy);
            isNewTrieNode = true;
        }
        pathTrace.push_back(nodeCopy);
        nodeCopy = childNodeCopy;
        ++depthCounter;
        charTypeIterator++;
    }
    pathTrace.push_back(nodeCopy);
    node = nodeCopy;

    if (!node->isTerminalNode() && !isNewTrieNode)
        isNewInternalTerminalNode = true;

    // for each trie node on the pathTrace, map its leftMostNode and
    // rightMostNode to their corresponding new node
    for (unsigned iter = 0; iter < pathTrace.size(); ++iter) {
        TrieNode* newNodeLeftMost = pathTrace.at(iter)->getLeftMostDescendant();
        if (oldToNewTrieNodeMap.find(newNodeLeftMost) != oldToNewTrieNodeMap.end())
            pathTrace.at(iter)->setLeftMostDescendant(oldToNewTrieNodeMap.find(newNodeLeftMost)->second);

        TrieNode* newNodeRightMost = pathTrace.at(iter)->getRightMostDescendant();
        if (oldToNewTrieNodeMap.find(newNodeRightMost) != oldToNewTrieNodeMap.end())
            pathTrace.at(iter)->setRightMostDescendant(oldToNewTrieNodeMap.find(newNodeRightMost)->second);
    }

    this->addKeyword_SetPrevIdNexIdByPathTrace(pathTrace, isNewTrieNode, node);

    // For each trie node whose id needs to be assigned,
    // we need to use oldToNewTrieNodeMap to map trie nodes on its PathTrace to the
    // corresponding newly cloned trie nodes, so that the old trie nodes can be
    // freed in the read view.
    // TODO what is the logic behind oldToNewTrieNodeMap and this function ? ???????????
    remapPathForTrieNodesToReassign(oldToNewTrieNodeMap);

    //return invertedListOffset and keywordId
    invertedListOffset = node->getInvertedListOffset();

    // Flip the dummy root node to the newly created path
    // this->root.reset(trieRootNode_WriteView);

    return node->getId();
}

unsigned Trie::addKeyword_ThreadSafe(const std::string &keyword, unsigned &invertedListOffset)
{
    bool isNewTrieNode = false;
    bool isNewInternalTerminalNode = false;
    return addKeyword_ThreadSafe(getCharTypeVector(keyword), invertedListOffset, isNewTrieNode, isNewInternalTerminalNode);
}


void Trie::remapPathForTrieNodesToReassign(OldToNewTrieNodeMap &oldToNewTrieNodeMap)
{
    // If no new trie node is created, do nothing.
    if (oldToNewTrieNodeMap.empty())
        return;

    for (unsigned i = 0; i < trieNodesToReassign.size(); i ++) {
        // get one pathTrace
        vector<TrieNode* > *pathTrace = trieNodesToReassign.at(i).path;
        // go through the trie nodes on the path
        for (unsigned nodeIter = 0; nodeIter < pathTrace->size(); nodeIter ++) {
            TrieNode* originalTrieNode = pathTrace->at(nodeIter);

            // set it to the new trie node if needed
            if (oldToNewTrieNodeMap.find(originalTrieNode) != oldToNewTrieNodeMap.end()) {
                TrieNode* newTrieNode = oldToNewTrieNodeMap.find(originalTrieNode)->second;
                (*pathTrace)[nodeIter] = newTrieNode;
            }
        }
    }
}

const TrieNode *Trie::getTrieNode(const TrieNode* rootReadView, const std::vector<CharType> &keyword) const
{
    const TrieNode *parentNode = rootReadView;
    std::vector<CharType>::const_iterator charTypeIterator = keyword.begin();
    while (charTypeIterator != keyword.end()) {
        TrieNode *childNode = parentNode->findChild((CharType) *charTypeIterator);
        if (childNode == NULL) {
            return NULL;
        }
        parentNode = childNode;
        ++charTypeIterator;
    }
    return parentNode;
}

const TrieNode *Trie::getTrieNodeFromUtf8String(const TrieNode* rootReadView, const std::string &keywordStr) const
{
    vector<CharType> keyword;
    utf8StringToCharTypeVector(keywordStr, keyword);
    const TrieNode *parentNode = rootReadView;
    std::vector<CharType>::const_iterator charTypeIterator = keyword.begin();
    while (charTypeIterator != keyword.end()) {
        TrieNode *childNode = parentNode->findChild((CharType) *charTypeIterator);
        if (childNode == NULL) {
            return NULL;
        }
        parentNode = childNode;
        ++charTypeIterator;
    }
    return parentNode;
}

int Trie::getNumberOfBytes() const
{
    boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNode_ReadView;
    this->getTrieRootNode_ReadView(trieRootNode_ReadView);
    const TrieNode *root = trieRootNode_ReadView->root;
    return root->getNumberOfBytes();
}

int Trie::getNumberOfNodes() const
{
    boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNode_ReadView;
    this->getTrieRootNode_ReadView(trieRootNode_ReadView);
    const TrieNode *root = trieRootNode_ReadView->root;

    return root->getNumberOfNodes();
}

int Trie::getfinalKeywordIdCounter() const
{
    boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNode_ReadView;
    this->getTrieRootNode_ReadView(trieRootNode_ReadView);
    const TrieNode *root = trieRootNode_ReadView->root;

    return root->getfinalKeywordIdCounter();
}

/* Commit phase functions
 * Used in commitSubTrie
 */

void Trie::commitSubTrie(TrieNode *node, unsigned &finalKeywordIdCounter, const unsigned sparsity)
{
    assert(node != NULL);
    if ( node->isTerminalNode() ) {
        /// FinalKeywordIdCounter maintains the counter for new keywordIds.
        unsigned sparseNewNodeId = (finalKeywordIdCounter + 1) * sparsity;

        this->oldIdToNewIdMapVector->at(node->getId()) = sparseNewNodeId;
        node->setId(sparseNewNodeId);
        finalKeywordIdCounter += 1;

        // initilizing the two insert counters to 1
        node->setLeftInsertCounter(1);
        node->setRightInsertCounter(1);
    }

    for (unsigned childIterator = 0; childIterator < node->getChildrenCount(); childIterator++ ) {
        this->commitSubTrie(node->getChild(childIterator), finalKeywordIdCounter, sparsity);
    }
}

void Trie::checkCorrectness(const TrieNodePath &tnpp)
{
    /*TrieNode* node = tnpp.trieNode;
    vector<TrieNode* > *path = tnpp.path;
    cout << "checkCorrectness: path.size() = " << path->size() << "; ";
    for (unsigned i = 0; i < path->size(); i ++)
      cout << path->at(i)->character;
    cout << "\n";

    ASSERT(node == path->at(path->size() - 1));

    for (unsigned i = 0; i < path->size(); i ++) {
      unsigned depth = path->at(i)->getDepth();
      ASSERT(i == depth);
      (void)i;
      }*/
}

// Get the left immediate neighbour TrieNode along with its path from root
bool Trie::getLeftTnp(const TrieNodePath &currentTnp, TrieNodePath &prevTnp)
{
    // check the correctness of the currentTnp
    checkCorrectness(currentTnp);

    unsigned pathTraceIterator = currentTnp.path->size() - 1;
    TrieNode* childNode = currentTnp.path->at(pathTraceIterator);

    --pathTraceIterator;
    TrieNode* parentNode = currentTnp.path->at(pathTraceIterator);

    // Iterate up until we reach the root or a terminal node or a non-firstChild node.
    while ( parentNode->isFirstChild(childNode) && (pathTraceIterator != 0)
            && (!parentNode->isTerminalNode()) ) {
        childNode = parentNode;
        --pathTraceIterator;
        parentNode = currentTnp.path->at(pathTraceIterator);

        ASSERT(parentNode->getDepth() == pathTraceIterator);
    }

    // The inserted keyword is the first keyword lexicographically
    if (parentNode->getDepth() == 0 && parentNode->isFirstChild(childNode)) {
        return false; // tell the caller that no left node is found
    }

    // if the parent node is a root, or a terminal node and its first child is "childNode",
    // then this parent node is the previous node
    if (parentNode->isTerminalNode() && parentNode->isFirstChild(childNode)) {
        // copy the path of the current TNPP all the way to the parent node
        prevTnp.path = new vector<TrieNode* >(currentTnp.path->begin(),
                                              currentTnp.path->begin() + pathTraceIterator + 1);
        checkCorrectness(prevTnp);

        return true;
    }

    // The left sibling's right-most descendent is the previous node of the current node.
    // First, copy the path up to the parent node.
    prevTnp.path = new vector<TrieNode* >(currentTnp.path->begin(),
                                          currentTnp.path->begin() + pathTraceIterator + 1);
    // Traverse the subtrie rooted at child[position - 1] and find its rightmost descendant
    int position = parentNode->findChildNodePosition(childNode->getCharacter());

    ASSERT(position >= 1);
    TrieNode* nodeIter = parentNode->getChild(position - 1);
    prevTnp.path->push_back(nodeIter);

    while (nodeIter->getChildrenCount() > 0) {
        TrieNode* rightmostChild = nodeIter->getChild(nodeIter->getChildrenCount() - 1);
        prevTnp.path->push_back(rightmostChild);
        nodeIter = rightmostChild;
    }

    checkCorrectness(prevTnp);

    return true;
}

// append to a TrieNodePath the trie nodes from the current "node" to its leftmost node
void Trie::extendTnpFromTrieNode(TrieNode* node, TrieNodePath &tnp)
{
    tnp.path->push_back(node);
    while (!node->isTerminalNode() && node->getChildrenCount() > 0) {
        TrieNode* leftmostChild = node->getChild(0);
        tnp.path->push_back(leftmostChild);
        node = leftmostChild;
    }

    ASSERT(node->isTerminalNode()); // it has to be a terminal node
}

// Get the right immediate neighbour TrieNode along with its path from root
bool Trie::getRightTnp(const TrieNodePath &currentTnp, TrieNodePath &nextTnp)
{
    checkCorrectness(currentTnp);

    unsigned pathTraceIterator = currentTnp.path->size() - 1;
    TrieNode* lastNode = currentTnp.path->at(pathTraceIterator);

    // if this node is a terminal node, and it has children, then its right TNP should be the
    // left most node of its first child
    if (lastNode->isTerminalNode() && lastNode->getChildrenCount() > 0) {
        // First copy the path up to the current node.
        nextTnp.path = new vector<TrieNode* >(currentTnp.path->begin(),
                                              currentTnp.path->begin() + pathTraceIterator + 1);
        extendTnpFromTrieNode(lastNode->getChild(0), nextTnp);
        return true;
    }

    // otherwise, we need to traverse up to find its first ancetor who has a right sibling
    TrieNode* childNode = lastNode;

    --pathTraceIterator;
    TrieNode* parentNode = currentTnp.path->at(pathTraceIterator);

    // Iterate up until we reach the root or a non-lastChild node.
    while (pathTraceIterator != 0 && (parentNode->isLastChild(childNode))) {
        childNode = parentNode;
        --pathTraceIterator;
        parentNode = currentTnp.path->at(pathTraceIterator);
    }

    // The inserted keyword is the last keyword lexicographically
    if (pathTraceIterator == 0 && (parentNode->isLastChild(childNode)))
        return false; // tell the caller that no right node is found

    // The right sibling's left-most descendent is the next node of the current node
    // First copy the path up to the parent node.
    nextTnp.path = new vector<TrieNode* >(currentTnp.path->begin(),
                                          currentTnp.path->begin() + pathTraceIterator + 1);

    // Traverse the subtrie rooted at child[position + 1] and find its leftmost descendant
    int position = parentNode->findChildNodePosition(childNode->getCharacter());
    ASSERT(position >= 0);
    ASSERT(position <= parentNode->getChildrenCount() - 2); // cannot be the last one
    extendTnpFromTrieNode(parentNode->getChild(position + 1), nextTnp);

    checkCorrectness(nextTnp);
    return true;
}

// See if the TrieNode is in the trieNodeIdMapper,
// which stores all the TrieNodes that to be reassigned Ids.
// If yes, return the the new Id in the map; if no, return the original Id
unsigned Trie::getTrueId(TrieNode *node, const map<TrieNode *, unsigned> &trieNodeIdMapper)
{
    map< TrieNode *, unsigned >::const_iterator iter = trieNodeIdMapper.find(node);
    if (iter == trieNodeIdMapper.end())
        return node->getId();
    else
        return iter->second;
}

// check if the Id is a temporary large Id, which is larger than MAX_ALLOCATED_KEYWORD_ID
bool Trie::isLargerThanUpperBound(unsigned id)
{
    return id > MAX_ALLOCATED_KEYWORD_ID;
}

// Calculate the sparsity in the current reassignRange to see if it's sparse enough
bool Trie::needToReassignMore(TrieNode *leftNode, TrieNode *rightNode, const unsigned size, const map<TrieNode *, unsigned> &trieNodeIdMapper)
{
    if (size < 2)
        return true;

    unsigned leftId = getTrueId(leftNode, trieNodeIdMapper);
    // if it's a large temporary id, then we've already reached the left bound
    if ( isLargerThanUpperBound(leftId) )
        leftId = 0;

    unsigned rightId = getTrueId(rightNode, trieNodeIdMapper);
    // if it's a large temporary id, then we've already reached the right bound
    if ( isLargerThanUpperBound(rightId) )
        rightId = MAX_ALLOCATED_KEYWORD_ID;

    return ((rightId - leftId) / (size - 1)) < KEYWORD_ID_SPARSITY;
}

// Keep including more TrieNodes on the left side into reassignRange,
// untill we reach a node that's not with a temporary reassigned id
bool Trie::assignMoreNodesOnTheLeft(TrieNodePath &leftSide, vector<TrieNode *> &reassignRange, const map<TrieNode *, unsigned> &trieNodeIdMapper)
{
    do {
        TrieNodePath newLeftSide;

        if (!getLeftTnp(leftSide, newLeftSide)) // if hit the left bound of the whole available range
            return false;

        // clean the old leftSide
        leftSide.clean();

        leftSide = newLeftSide;
        reassignRange.insert(reassignRange.begin(), leftSide.getLastTrieNode());

        // loop until we find a left node that's not with a temporary reassigned id
    } while (isLargerThanUpperBound( getTrueId(leftSide.getLastTrieNode(), trieNodeIdMapper) ));

    return true;
}

// Keep including more TrieNodes on the right side into reassignRange,
// untill we reach a node that's not with a temporary reassigned id
bool Trie::assignMoreNodesOnTheRight(TrieNodePath &rightSide, vector<TrieNode *> &reassignRange, const map<TrieNode *, unsigned> &trieNodeIdMapper)
{
    do {
        TrieNodePath newRightSide;

        if (!getRightTnp(rightSide, newRightSide)) // if hit the right bound of the whole available range
            return false;

        // clean the old rightSide
        rightSide.clean();

        rightSide = newRightSide;

        reassignRange.push_back(rightSide.getLastTrieNode());

        // loop until we find a right node that's not with a temporary reassigned id
    } while (isLargerThanUpperBound( getTrueId(rightSide.getLastTrieNode(), trieNodeIdMapper) ));

    return true;
}

// Uniformly distribute Ids for all the TrieNodes in reassignRange
// according to what we have on the right and on the left and the number of trie nodes in the range we
// assign new ids uniformly.
void Trie::doReassignment(const vector<TrieNode *> &reassignRange, map<TrieNode *, unsigned> &trieNodeIdMapper)
{
    unsigned leftId = getTrueId(reassignRange[0], trieNodeIdMapper);
    // if it's the case where the left id is a large temporary reassigned id, which means it's the leftmost bound,
    // then set the id to 0
    if (isLargerThanUpperBound(leftId)) {
        leftId = 0;
        trieNodeIdMapper[reassignRange[0]] = 0;
    }

    unsigned rightId = getTrueId(reassignRange[reassignRange.size()-1], trieNodeIdMapper);
    // if it's the case where the right id is a large temporary reassigned id, which means it's the rightmost bound,
    // then set the id to MAX_ALLOCATED_KEYWORD_ID
    if (isLargerThanUpperBound(rightId)) {
        rightId = MAX_ALLOCATED_KEYWORD_ID;
        trieNodeIdMapper[reassignRange[reassignRange.size()-1]] = MAX_ALLOCATED_KEYWORD_ID;
    }

    if (reassignRange.size() > 1) {
    	// calculate the distance between each Id
    	ASSERT(leftId < rightId);
    	unsigned pace = (rightId - leftId) / (reassignRange.size()-1);

    	// store the reassigned Id in a map
    	for (unsigned i = 1; i < reassignRange.size() - 1; i++)
    		trieNodeIdMapper[reassignRange[i]] = leftId + i * pace;
    } else {
    	// when trie is empty and first insertion only has one keyword leftId and rightId both are zero
    	// and reassignRange.size() is 1
    	// So in this special case we just assign id "MAX_ALLOCATED_KEYWORD_ID/2" to this node
    	trieNodeIdMapper[reassignRange[0]] = MAX_ALLOCATED_KEYWORD_ID/2;
    }

}

// For all the TrieNodes we need to reassign,
// we first look at their left neighbours to see if we can adjust one or some of their Ids
// to make more Id space so that we can keep the sparsity after inserting the new TrieNode
// If we run out of space on the entire left side, we will then try the right side
/*
 * @param iter is the iterator on trieNodesToReassign : the keyword to reassign id
 */
void Trie::pushNeighborsForMoreSpace(const unsigned iter, map<TrieNode *, unsigned> &trieNodeIdMapper)
{
    TrieNodePath leftSide;
    TrieNodePath rightSide;

    // Jamshid : We will store all the TrieNodes that we need to reassign Ids in reassignRange
    vector<TrieNode *> reassignRange;

    // Jamshid : Includes the newly inserted TrieNode , after finding the range completely the first and the last
    // one are non-temperoray ids. Except for the case where the current node is already on the leftmost/rightmost position
    // in the trie
    reassignRange.push_back(trieNodesToReassign[iter].getLastTrieNode());

    // Jamshid : Starting from the newly inserted TrieNode,
    // keep moving and including leftward until we reach a node not with a temporary reassigned id
    // TODO why do we check for temporary ids on the left ? Because we move from left to right and we shouldn't
    // see any temporary ids on the left (pro : it makes it symmetric)
    bool leftAvailable = getLeftTnp(trieNodesToReassign[iter], leftSide);
    if (leftAvailable) {
        reassignRange.insert(reassignRange.begin(), leftSide.getLastTrieNode());
        // make sure it's not a node with temporary large id
        if ( isLargerThanUpperBound( getTrueId(leftSide.getLastTrieNode(), trieNodeIdMapper) ) )
            leftAvailable = assignMoreNodesOnTheLeft(leftSide, reassignRange, trieNodeIdMapper);
        //printTriePath(leftSide.path);
    }

    // Starting from the newly inserted TrieNode,
    // keep moving and including rightward until we reach a node not with a temporary reassigned id
    bool rightAvailable = getRightTnp(trieNodesToReassign[iter], rightSide);
    if (rightAvailable) {
        reassignRange.push_back(rightSide.getLastTrieNode());
        // make sure it's not a node with temporary large id
        if ( isLargerThanUpperBound( getTrueId(rightSide.getLastTrieNode(), trieNodeIdMapper) ) )
            rightAvailable = assignMoreNodesOnTheRight(rightSide, reassignRange, trieNodeIdMapper);
        //printTriePath(rightSide.path);
    }

    // Jamshid : Based on the current TrieNodes in reassignRange, we tell if reassigning Ids to them
    // can make the Id space sparse enough.
    // If yes, we do the reassignment; if no, we include more neighbor TrieNodes into the range
    while (needToReassignMore(reassignRange[0], reassignRange[reassignRange.size()-1], reassignRange.size(), trieNodeIdMapper)) {
        // Try to include left neighbours first
        if (leftAvailable) {
            leftAvailable = assignMoreNodesOnTheLeft(leftSide, reassignRange, trieNodeIdMapper);
            if (leftAvailable) {
                continue;
            }
        }

        // Include right neighbors if we've run out of space on the left side
        if (rightAvailable) {
            rightAvailable = assignMoreNodesOnTheRight(rightSide, reassignRange, trieNodeIdMapper);
            if (rightAvailable) {
                continue;
            }
        }

        // TODO run out of the whole space
        // reduce the sparsity threshould and reassign all the Ids
        break;
    }

    // After we decided the reassignRange, do the reassignment
    doReassignment(reassignRange, trieNodeIdMapper);

    leftSide.clean();
    rightSide.clean();
}

// The trieNodeIdMapper stores those newly inserted TrieNodes that don't have Id space
// with their temporary Ids. In this function we will adjust their neighbors' Ids to
// make space for them and assign new Ids to them and their adjusted neighbors.
/*
 * @param trieNodeIdMapper will keep the final correct ids of the trie nodes.
 */
void Trie::reassignKeywordIds(map<TrieNode *, unsigned> &trieNodeIdMapper)
{
    // We generate the mapper in three steps:

    // step 1: sort trieNodesToReassign based on their string values
    // Jamshid : TrieNodesToReassign stores all the TrieNodes we need to reassign Ids
    // along with the paths from the Trie's root.
    // We sort them based on the TrieNode encoding order.
    // alphabetical order
    std::sort(trieNodesToReassign.begin(), trieNodesToReassign.end());

    // step 2: reassign ids to trie nodes by "pushing" its trie neighbors
    // For all the TrieNodes we need to reassign, we first try to push their
    // left neighbours to make more Id space. If we run out of space on the
    // entire left side, we will then try to push right
    for (unsigned iter = 0; iter < trieNodesToReassign.size(); iter ++) {
        printTriePath(trieNodesToReassign[iter].path);
        // Jamshid : the reason to have this condition : if we don't have room on left, we push to the right and some
        // new trie nodes in right also might obtain their id, so it is possible to visit a new trie node in this vector
        // that already has an id
        // Note : the mapper might also store some old nodes which are pushed to one of the directions.
        if (trieNodeIdMapper.find(trieNodesToReassign[iter].getLastTrieNode()) == trieNodeIdMapper.end())
            pushNeighborsForMoreSpace(iter, trieNodeIdMapper);
    }

    // step 3: clean up
    // Because for each TrieNode we reasigned Id to, we created a copy of
    // their paths from the Trie's root and stored the pointers to the copies
    // in trieNodesToReassign, after the reassignment we need to clean up
    // those copies.
    for (int i = 0; i < trieNodesToReassign.size(); i ++)
        trieNodesToReassign.at(i).clean();

    trieNodesToReassign.clear();
}


void Trie::calculateNodeHistogramValuesFromChildren(const InvertedIndex * invertedIndex ,
		const ForwardIndex * forwardIndex ,
		const unsigned totalNumberOfRecords){
    boost::shared_ptr<TrieRootNodeAndFreeList > trieRootNode_ReadView;
    this->getTrieRootNode_ReadView(trieRootNode_ReadView);
    TrieNode *root = trieRootNode_ReadView->root;
    if(root == NULL){
    	return;
    }
    calculateNodeHistogramValuesFromChildren(root, invertedIndex  , forwardIndex, totalNumberOfRecords);
}

void Trie::calculateNodeHistogramValuesFromChildren(TrieNode *node,
		const InvertedIndex * invertedIndex ,
		const ForwardIndex * forwardIndex ,
		const unsigned totalNumberOfRecords){
    if(node == NULL){
    	return;
    }
    // first iterate on children an calculate this value for them
    for(unsigned childIterator = 0; childIterator < node->getChildrenCount() ; childIterator ++){
    	calculateNodeHistogramValuesFromChildren(node->getChild(childIterator) , invertedIndex , forwardIndex, totalNumberOfRecords);
    }

    // now we should initialize the value of this node
    if(node->isTerminalNode()){
    	// this case means this node is a terminal node
    	// NOTE: this node can still have children because internal nodes in this trie can also be terminal nodes.
    	if(invertedIndex == NULL || forwardIndex == NULL){ // this case happens in M1
            // if inverted index is null, nodeSubTrieValue is actually the frequency of leaf nodes.
            node->initializeInternalNodeHistogramValues(HistogramAggregationTypeSummation, 1);
    	}else{ // this is the case of A1
    		shared_ptr<vectorview<InvertedListContainerPtr> > invertedListDirectoryReadView;
    		invertedIndex->getInvertedIndexDirectory_ReadView(invertedListDirectoryReadView);
    	    shared_ptr<vectorview<ForwardListPtr> > forwardIndexDirectoryReadView;
    	    forwardIndex->getForwardListDirectory_ReadView(forwardIndexDirectoryReadView);
    	    shared_ptr<vectorview<unsigned> > invertedIndexKeywordIdsReadView;
    	    invertedIndex->getInvertedIndexKeywordIds_ReadView(invertedIndexKeywordIdsReadView);
			shared_ptr<vectorview<unsigned> > invertedListReadView;
			invertedIndex->getInvertedListReadView(invertedListDirectoryReadView,
					node->getInvertedListOffset(), invertedListReadView);

			float termRecordStaticScore = 0;
			vector<unsigned> matchedAttrsList;
			// move on inverted list to find the first record which is valid
			unsigned invertedListCursor = 0;
			while(invertedListCursor < invertedListReadView->size()){
				unsigned recordId = invertedListReadView->getElement(invertedListCursor++);
				// check if the record is valid
				// forwardIndexDirectoryReadView
				unsigned keywordOffset = invertedIndex->getKeywordOffset(forwardIndexDirectoryReadView,
						invertedIndexKeywordIdsReadView, recordId, node->getInvertedListOffset());

				// if the record is not valid (e.g., deleted), ignore it.
				if (keywordOffset == FORWARDLIST_NOTVALID)
					continue;
				vector<unsigned> filterAttributeList;
				if (invertedIndex->isValidTermPositionHit(forwardIndexDirectoryReadView, recordId,
						keywordOffset,
						filterAttributeList, ATTRIBUTES_OP_OR,  matchedAttrsList, termRecordStaticScore)) { // 0x7fffffff means OR on all attributes
					break;
				}
			}
			// now that we have the static score, add the static score to the value of this node

			// we use the probability of this terminal node to occur in a record as the initial value
			if(totalNumberOfRecords == 0){
				// if there no records at all, termRecordStaticScore must be zero here
				ASSERT(termRecordStaticScore == 0);
				node->initializeInternalNodeHistogramValues(HistogramAggregationTypeJointProbability, 0 , (half)0);
			}else{
				float pTerminalNode = (1.0 * invertedListReadView->size()) / totalNumberOfRecords ;
				node->initializeInternalNodeHistogramValues(HistogramAggregationTypeJointProbability, pTerminalNode , (half)termRecordStaticScore);
			}
    	}
    }else{ // non-terminal node, if it's non-terminal, it still needs to be initialized.
    	if(invertedIndex == NULL){ // it is the case of M1
			node->initializeInternalNodeHistogramValues(HistogramAggregationTypeSummation);
    	}else{ // it is the case of A1
    		node->initializeInternalNodeHistogramValues(HistogramAggregationTypeJointProbability);
    	}
    }

    // now update the value of this node from its children
    if(invertedIndex == NULL){
    	node->updateInternalNodeHistogramValues(HistogramAggregationTypeSummation);
    }else{
    	node->updateInternalNodeHistogramValues(HistogramAggregationTypeJointProbability);
    }
    return;
}

void TrieNode::updateInternalNodeHistogramValues(HistogramAggregationType aggrType){

	if(this->getChildrenCount() == 0) return;

	float aggregatedProbabilityValueSoFar = this->getChild(0)->getNodeProbabilityValue();
	// iterate on children and aggregate the values
    for (unsigned int childIterator = 1 ; childIterator < this->getChildrenCount(); childIterator++ ) {
        switch (aggrType) {
			case HistogramAggregationTypeSummation:
				aggregatedProbabilityValueSoFar = aggregateValueBySummation(aggregatedProbabilityValueSoFar , this->getChild(childIterator)->getNodeProbabilityValue());
				break;
			case HistogramAggregationTypeJointProbability:
				aggregatedProbabilityValueSoFar =
						aggregateValueByJointProbability(aggregatedProbabilityValueSoFar , this->getChild(childIterator)->getNodeProbabilityValue());
				break;
		}
    }
    // and also use the value that this node currently has
    switch (aggrType) {
		case HistogramAggregationTypeSummation:
			aggregatedProbabilityValueSoFar = aggregateValueBySummation(aggregatedProbabilityValueSoFar , this->getNodeProbabilityValue());
			break;
		case HistogramAggregationTypeJointProbability:
			aggregatedProbabilityValueSoFar =
					aggregateValueByJointProbability(aggregatedProbabilityValueSoFar , this->getNodeProbabilityValue());
			break;
	}

    // set the result in the class member
    this->setNodeProbabilityValue(aggregatedProbabilityValueSoFar);

    // also update the maximumScoreOfLeafNodes
    this->updateInternalNodeMaximumScoreOfLeafNodes();

    // update the number of terminal nodes
	// iterate on children and aggregate the values
    unsigned totalNumberOfLeadNodes = this->getNumberOfTerminalNodes();
    for (unsigned int childIterator = 1 ; childIterator < this->getChildrenCount(); childIterator++ ) {
    	totalNumberOfLeadNodes += this->getChild(childIterator)->getNumberOfTerminalNodes();
    }
    this->setNumberOfTerminalNodes(totalNumberOfLeadNodes);
}


// updates the maximum score of leaf nodes based on the values coming from children and
// and returns true if anything changes and should be propagated up the trie
bool TrieNode::updateInternalNodeMaximumScoreOfLeafNodes(){
	if(this->getChildrenCount() == 0) return false;

	half aggregatedMaximumScoreForLeafNodesSoFar = this->getChild(0)->getMaximumScoreOfLeafNodes();
	// iterate on children and aggregate the values
	for (unsigned int childIterator = 1 ; childIterator < this->getChildrenCount(); childIterator++ ) {
		// aggregate the maximum score of leaf nodes by taking the maximum of scores of children
		aggregatedMaximumScoreForLeafNodesSoFar = aggregateValueByTakingMaximum(aggregatedMaximumScoreForLeafNodesSoFar ,
        		this->getChild(childIterator)->getMaximumScoreOfLeafNodes());
    }
    // aggregate the maximum score of leaf nodes by taking the maximum of scores of children and the current score of this node
    aggregatedMaximumScoreForLeafNodesSoFar = aggregateValueByTakingMaximum(aggregatedMaximumScoreForLeafNodesSoFar ,
    		this->getMaximumScoreOfLeafNodes());

    // set the result in the class member
    if(aggregatedMaximumScoreForLeafNodesSoFar > this->getMaximumScoreOfLeafNodes()){
		this->setMaximumScoreOfLeafNodes(aggregatedMaximumScoreForLeafNodesSoFar);
    	return true;
    }
    return false;
}
void TrieNode::initializeInternalNodeHistogramValues(HistogramAggregationType aggrType ,
		float initValueFromArg,
		half initValueFromArgForMaxScore){
	float initValue = 0;
	if(initValueFromArg != -1){
		initValue = initValueFromArg;
	}else{
		switch (aggrType) {
			case HistogramAggregationTypeSummation:
				initValue = 0;
				break;
			case HistogramAggregationTypeJointProbability:
				initValue = 0;
				break;
		}
	}
    this->setNodeProbabilityValue(initValue);
    // initialize the maximum score of leaf nodes
    this->setMaximumScoreOfLeafNodes(initValueFromArgForMaxScore);
    // initialize the number of leaf nodes
    if(this->isTerminalNode() == true){ // this node is a terminal node itself
    	this->setNumberOfTerminalNodes(1);
    }else{
    	this->setNumberOfTerminalNodes(0);
    }
}

void Trie::printTrieNodeSubTrieValues(std::vector<CharType> & prefix , TrieNode * root , unsigned depth){
	for(int i=0;i<depth ; i++){
		std::cout << "-" ;
	}
	prefix.push_back(root->getCharacter());
	string str = getUtf8String(prefix);
	std::cout << str << "(" << root->getNodeProbabilityValue() << ")" << std::endl;
	if(! root->isTerminalNode()){
	    unsigned childIterator = 0;
	    for(; childIterator < root->getChildrenCount() ; childIterator ++){
	    	printTrieNodeSubTrieValues(prefix, root->getChild(childIterator) , depth+1);
	    }
	}
	prefix.pop_back();
}


void Trie::merge(const InvertedIndex * invertedIndex ,
		const ForwardIndex * forwardIndex ,
		const unsigned totalNumberOfRecords  , bool updateHistogram)
{

	// We change the isCopy of the nodes in the write view.
	this->root_writeview->resetCopyFlag();
	// if it's the time for updating histogram (because we don't do it for all merges, it's for example every 10 merges)
	// then update the histogram information in Trie.
	if(updateHistogram == true){
		this->calculateNodeHistogramValuesFromChildren(invertedIndex , forwardIndex , totalNumberOfRecords);
	}
    // In each merge, we first put the current read view to the end of the queue,
    // and reset the current read view. Then we go through the read views one by one
    // in the order of their arrival. For each read view, we check its reference count.
    // If the count is > 1, then it means there are readers that are still using it,
    // so we do nothing and return. If the read view's reference count is 1,
    // then it means the current merge thread is the last thread using this read view,
    // so we can delete it and move onto the next read view on the queue.
    // We repeat the process until either we reach the end of the queue or we
    // find a read view with a reference count > 1.
    this->oldReadViewQueue.push(this->root_readview);
    pthread_spin_lock(&m_spinlock);
    this->root_readview.reset(new TrieRootNodeAndFreeList(this->root_writeview));
    // We can safely release the lock now, since the only chance the read view can be modified is during merge().
    // But merge() can only happen when another writer comes in, and we assume at any time only one writer can come in.
    // So this case cannot happen.
    pthread_spin_unlock(&m_spinlock);
    while (!this->oldReadViewQueue.empty() && this->oldReadViewQueue.front().unique()) {
        this->oldReadViewQueue.pop();
    }
    if(this->root_writeview){
        delete this->root_writeview;
    }

    this->root_writeview = new TrieNode(this->root_readview.get()->root);
}

void Trie::commit()
{
    ASSERT(commited == false);
    // we remove the old readview's root first
    delete this->root_readview->root;
    // We change the isCopy of the nodes in the write view.
    this->root_writeview->resetCopyFlag();
    this->root_readview->root = root_writeview;
    // We create a new write view's root by copying the root of the read review
    this->root_writeview = new TrieNode(this->root_readview->root);
    /**
      * 1. Traverse the Trie using depth first.
      * 2. Set new IDs, update InvertedList offsets.
      * 3. Call _setLeftRightMostDescendants(this->root)
      */
    //unsigned sparsity = getMaxSparsity();
    //std::cout<<"commiting trie..\n";

    this->oldIdToNewIdMapVector = new std::vector<unsigned>();
    this->oldIdToNewIdMapVector->assign(this->numberOfTerminalNodes, 0);

    unsigned sparsityFactor = (this->numberOfTerminalNodes > 1)?(this->numberOfTerminalNodes+5):5;
    sparsityFactor = (MAX_ALLOCATED_KEYWORD_ID/sparsityFactor);  // divide the integer space evenly

    unsigned finalKeywordIdCounter = 0;
    this->commitSubTrie(this->root_readview.get()->root, finalKeywordIdCounter, sparsityFactor);
}

void Trie::finalCommit_finalizeHistogramInformation(const InvertedIndex * invertedIndex ,
		const ForwardIndex * forwardIndex,
		const unsigned totalNumberOfResults ){
	// traverse the trie in preorder to calculate nodeSubTrieValue
	calculateNodeHistogramValuesFromChildren(invertedIndex , forwardIndex , totalNumberOfResults);
	// now set the commit flag to true to indicate commit is finished
    this->commited = true;
}


void Trie::applyKeywordIdMapperOnEmptyLeafNodes(map<unsigned, unsigned> &keywordIdMapper) {
      for (int i = 0; i < emptyLeafNodeIds.size(); i++) {
        map<unsigned, unsigned>::const_iterator keywordIdMapperIter =
           keywordIdMapper.find(emptyLeafNodeIds.at(i));
        // if this keyword ID is in the mapper, we use the new id
        if (keywordIdMapperIter != keywordIdMapper.end())
           emptyLeafNodeIds.at(i) = keywordIdMapperIter->second;
      }
 }

void Trie::removeDeletedNodes()
{
    // sort the ids of the empty leaf nodes
    std::sort(emptyLeafNodeIds.begin(), emptyLeafNodeIds.end());

    TrieNode *writeViewRoot = this->getTrieRootNode_WriteView();
    removeDeletedNodes(writeViewRoot);
}

// return TRUE if the subtrie of t becomes empty, and FALSE otherwise
bool Trie::removeDeletedNodes(TrieNode *trieNode)
{
    // [child_0] [child_1] ... [child_k]   --- sorted
    //   /   \     /   \          /  \
    // min   max  min  max       min  max
    //
    // emptyLeafNodeIds:
    //  [v_0, v_1, v_2, ..., v_n]   --- sorted
    //
    // Find a range of children [a,b] that need to shrink based on their
    // [min, max] interval and the list emptyLeafNodeIds. A child needs to shrink
    // if its interval overlaps with the id of one of the empty leaf nodes
    unsigned minEmptyNodeId = emptyLeafNodeIds.front();
    unsigned maxEmptyNodeId = emptyLeafNodeIds.back();

    // do a binary search to find the first child "a"
    int low = 0;
    int high = trieNode->getChildrenCount() - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (trieNode->getChild(mid)->getMinId() >= minEmptyNodeId)
            high = mid - 1;
        else
            low = mid + 1;
    }

    // scan the children whose [min, max] interval
    // overlaps [minEmptyNodeId, maxEmptyNodeId]
    int childCursor = low;
    while (childCursor < trieNode->getChildrenCount()
          && trieNode->getChild(childCursor)->getMinId() <= maxEmptyNodeId) {
        // Interval relationship:
        //             [minId,                  maxId]
    	// [minEmptyNodeId,    maxEmptyNodeId]
        ASSERT(trieNode->getChild(childCursor)->getMinId() >= minEmptyNodeId);
        int childCount = trieNode->getChildrenCount();
        if (removeDeletedNodes(trieNode->getChild(childCursor))) {
           // this subtrie is empty. Then delete this child,
           // shift the children from the right to the left.
           delete trieNode->getChild(childCursor);
           for (int i = childCursor; i < trieNode->getChildrenCount() - 1; i++)
               trieNode->setChild(i, trieNode->getChild(i+1));
        } else {
           childCursor ++;
        }
    }

    if (trieNode->isTerminalNode()) {
      // if it is one of the empty leaf nodes, then it should no longer be a terminal node
      if (std::binary_search(emptyLeafNodeIds.begin(), emptyLeafNodeIds.end(), trieNode->id)) {
          trieNode->setTerminalFlag(false);
      }
    }

    // This subtrie becomes empty if it doesn't have children any more
    // and it is not a terminal node
    if (trieNode->getChildrenCount() == 0 && trieNode->isTerminalNode() == false) {
        return true;
    }

    // this subtrie is not empty.

    // Set the two pointers
    if (trieNode->isTerminalNode())
        trieNode->setLeftMostDescendant(trieNode); // set it to itself
    else // t.leftMostNode = t.firstChild.leftMostNode;
        trieNode->setLeftMostDescendant(trieNode->getChild(0)->getLeftMostDescendant());

    // t.rightMostNode = t.lastChild.rightMostNode
    if (trieNode->getChildrenCount() > 0) {
        unsigned childCount = trieNode->getChildrenCount();
        trieNode->setRightMostDescendant(trieNode->getChild(childCount - 1)->getRightMostDescendant());
    } else {
        trieNode->setRightMostDescendant(trieNode); // set it to itself
    }

    // tell the caller this subtrie is not empty
    return false;
}

const std::vector<unsigned> *Trie::getOldIdToNewIdMapVector() const
{
    return this->oldIdToNewIdMapVector;
}

void Trie::deleteOldIdToNewIdMapVector()
{
    if (this->oldIdToNewIdMapVector != NULL) {
        delete this->oldIdToNewIdMapVector;

        this->oldIdToNewIdMapVector = NULL;
    }
}

void Trie::getAncestorPrefixes(const Prefix &prefix, std::vector<Prefix> &ancestorPrefixes) const
{
    // get root node
    boost::shared_ptr<TrieRootNodeAndFreeList > rootNode_readView;
    this->getTrieRootNode_ReadView(rootNode_readView);
    const TrieNode* node = rootNode_readView->root;

    // traverse down the trie
    while (node != NULL && node->getMaxId() >= prefix.maxId) {
        node = node->findLowerBoundChildByMinId(prefix.minId);
        //Check if the child is a terminal node or has more than one children
        // TODO remove node->isTerminalNode() ? add comments
        if ( node != NULL && (node->isTerminalNode() || node->childrenPointerList.size() > 1) ) {
            // skip the node with the same minId and maxId
            if ( !(prefix.minId == node->getMinId() && prefix.maxId == node->getMaxId()) ) {
                ancestorPrefixes.push_back( Prefix(node->getMinId(), node->getMaxId()) );
                if (!ancestorPrefixes.back().isAncestor(prefix)) {
                    // for (int i=0; i<ancestorPrefixes.size();i++)
                    //cout << ancestorPrefixes[i].minId << " " << ancestorPrefixes[i].maxId << endl;

                    //cout << "---" << prefix.minId << " " << prefix.maxId << endl;
                    ASSERT(false);
                }
            } else
                return;
        }
    }
}

// TODO optimize
bool Trie::getParentPrefix(const Prefix &prefix, Prefix &parentPrefix) const
{
    std::vector<Prefix> ancestorPrefixes;
    getAncestorPrefixes(prefix, ancestorPrefixes);
    if (ancestorPrefixes.size() == 0)
        return false;

    parentPrefix = ancestorPrefixes.back();
    return true;
}

bool Trie::isDirectChildPrefix(const Prefix &parent, const Prefix &descendant) const
{
    std::vector<Prefix> ancestorPrefixes;
    this->getAncestorPrefixes(descendant, ancestorPrefixes);

    if (ancestorPrefixes.size() == 0
            || ancestorPrefixes.back() != parent)
        return false;

    return true;
}

// tell if the prefix can denote a terminal trieNode
bool Trie::isPrefixACompleteKeyword(const Prefix &prefix) const
{
    if (prefix.minId == prefix.maxId)
        return true; // it's a leaf node

    // get root node
    boost::shared_ptr<TrieRootNodeAndFreeList > rootNode_readView;
    this->getTrieRootNode_ReadView(rootNode_readView);
    const TrieNode* node = rootNode_readView->root;

    // traverse down the trie
    while ( node != NULL
            && node->getMaxId() >= prefix.maxId) { // don't need to traverse below the prefix
        if (node->isTerminalNode() &&
                prefix.minId == node->getMinId() && prefix.maxId == node->getMaxId())
            return true;

        node = node->findLowerBoundChildByMinId(prefix.minId);
    }

    return false;
}

// return 1 left most
//        2 right most
//        0 none
unsigned Trie::ifBreakOldParentPrefix(const std::vector<CharType> &keyword, vector<Prefix> *oldParentOrSelfAndAncs, bool &hadExactlyOneChild) const
{
    if (keyword.size() == 0)
        return 0;

    const TrieNode *node = this->getTrieRootNode_WriteView();

    vector<CharType> cleanedString;
    cleanString(keyword, cleanedString); // remove bad characters
    vector<CharType>::iterator charTypeIterator = cleanedString.begin();
    while (charTypeIterator != cleanedString.end()) {
        assert(node != NULL);

        int childPosition = node->findChildNodePosition((CharType) *charTypeIterator);

        if (childPosition >= 0) {
            node = node->getChild(childPosition);
            Prefix newPrefix(node->getMinId(), node->getMaxId());
            if (oldParentOrSelfAndAncs->size() == 0 || oldParentOrSelfAndAncs->back() != newPrefix)
                oldParentOrSelfAndAncs->push_back(newPrefix);  // CHEN: Add the current prefix to the vector
            ++charTypeIterator;
        } else
            break; // CHEN: Found the first trie node that needs to add a new child node
    }

    if (node->getChildrenCount() == 1)
        hadExactlyOneChild = true;
    else
        hadExactlyOneChild = false;

    if (charTypeIterator == cleanedString.end()) // it's an existing internal trie node turned into a terminal trie node
        return 1;  // CHEN: It's an existing keyword.  Breaking from left

    if (keyword.size() == 1 // a new one-letter-word, no parent to break
            || oldParentOrSelfAndAncs->size() == 0) // a new trie branch
        return 0;  // CHEN: Didn't break the interval

    if (node->getChildrenCount() == 0) // append an existing leaf trie node
        return 2; // CHEN: Added a new keyword

    if (!node->isTerminalNode() // only if the old parent is not a terminal node can the new node possibly break from left
            && ((CharType) *charTypeIterator) < node->getChild(0)->getCharacter())
        return 1; // CHEN: Breaking from left
    if (((CharType) *charTypeIterator) > node->getChild(node->getChildrenCount()-1)->getCharacter())
        return 2; // CHEN: Breaking from right

    return 0; // CHEN: Didn't break the interval
}

void Trie::getPrefixFromKeywordId(unsigned keywordId, Prefix &prefix) const
{
    unsigned minId, maxId, length;
    getKeywordMinMaxIdLength(keywordId, minId, maxId, length);
    prefix.minId = minId;
    prefix.maxId = maxId;
}

unsigned Trie::getKeywordLength(unsigned keywordId) const
{
    unsigned minId, maxId, length;
    getKeywordMinMaxIdLength(keywordId, minId, maxId, length);
    return length;
}

// Finds the node corresponding to the keywordId and returns its mindId, maxId.
// If keywordId not found returns (0, MAX_ALLOCATED_KEYWORD_ID).
void Trie::getKeywordMinMaxIdLength(unsigned keywordId, unsigned &minId, unsigned &maxId,
                                    unsigned &length) const
{
    // get root node
    boost::shared_ptr<TrieRootNodeAndFreeList > rootNode_readView;
    this->getTrieRootNode_ReadView(rootNode_readView);
    const TrieNode* node = rootNode_readView->root;

    // traverse down the trie
    while (node != NULL) {
        node = node->findLowerBoundChildByMinId(keywordId);
        if (node != NULL && node->isTerminalNode() && keywordId == node->getMinId()) {
            minId = node->getMinId();
            maxId = node->getMaxId();
            length = node->getDepth();
            return;
        }
    }

    // should not reach here
    Logger::debug("Failed to find the keyword on Trie: %d ", keywordId);
    ASSERT(false);
    minId = 0;
    maxId = MAX_ALLOCATED_KEYWORD_ID;
    length = 0;
    return;
}
// for WriteView
// Finds the node corresponding to the keywordId and returns its mindId, maxId.
// If keywordId not found returns (0, MAX_ALLOCATED_KEYWORD_ID).
// TODO : If this function is no longer needed, remove it.
void Trie::getKeywordMinMaxIdLength_WriteView(unsigned keywordId, unsigned &minId, unsigned &maxId,
        unsigned &length) const
{
    // get root node
    const TrieNode* node = this->getTrieRootNode_WriteView();

    // traverse down the trie
    while (node != NULL) {
        node = node->findLowerBoundChildByMinId(keywordId);
        if (node != NULL && node->isTerminalNode() && keywordId == node->getMinId()) {
            minId = node->getMinId();
            maxId = node->getMaxId();
            length = node->getDepth();
            return;
        }
    }

    minId = 0;
    maxId = MAX_ALLOCATED_KEYWORD_ID;
    length = 0;
    return;
}

// for WriteView
// Finds the node corresponding to the keywordId and returns its mindId, maxId.
//  If keywordId is not found, the path should be returned empty.
// otherwise, it's guaranteed that first element in path is root and last element is the corresponding trie node.
void Trie::getKeywordCorrespondingPathToTrieNode_WriteView(unsigned keywordId, TrieNodePath * trieNodePath) const
{
	// path should be empty in the beginning
	ASSERT(trieNodePath != NULL || trieNodePath->path->size() == 0);
    // get root node
    TrieNode* node = this->getTrieRootNode_WriteView();
    // traverse down the trie
    while (node != NULL) {
		trieNodePath->path->push_back(node);
        node = node->findLowerBoundChildByMinId(keywordId);
        if (node != NULL && node->isTerminalNode() && keywordId == node->getMinId()) {
        	trieNodePath->path->push_back(node);
            return;
        }
    }
    //  If keywordId is not found, the path should be returned empty.
    trieNodePath->path->clear();
    return;
}

void Trie::updateMaximumScoreOfLeafNodesForKeyword_WriteView(unsigned keywordId , half newScore){
	// first find the trie node of keywordId
	TrieNodePath pathToCorrespondingTrieNodeWithKeywordId;
	pathToCorrespondingTrieNodeWithKeywordId.path = new vector<TrieNode *>();
	getKeywordCorrespondingPathToTrieNode_WriteView(keywordId , &pathToCorrespondingTrieNodeWithKeywordId);

	// check to see if keywordId was valid and we actually have a corresponding trie node
	if(pathToCorrespondingTrieNodeWithKeywordId.path->size() == 0){
		delete pathToCorrespondingTrieNodeWithKeywordId.path;
		return;
	}

	// now update the node and propagate the change up
	// propagating the changes up the trie
	for(int ancestorIter = pathToCorrespondingTrieNodeWithKeywordId.path->size()-1 ; ancestorIter >= 0 ; --ancestorIter){
		// get the ancestor trie node
		TrieNode * ancestorNode = pathToCorrespondingTrieNodeWithKeywordId.path->at(ancestorIter);
		// get the value after taking the maximum
		half maxScore = ancestorNode->aggregateValueByTakingMaximum(ancestorNode->getMaximumScoreOfLeafNodes() , newScore);
		// see if anything changes. If it does, set the new value
		if(maxScore	> ancestorNode->getMaximumScoreOfLeafNodes()){ // this trie node must be updated, note that here maxScore is equal to newScore
			ancestorNode->setMaximumScoreOfLeafNodes(newScore);
		}else{
			// no need to propagate any change up the trie
			delete pathToCorrespondingTrieNodeWithKeywordId.path;
			return;
		}
	}
	delete pathToCorrespondingTrieNodeWithKeywordId.path;
}

// Finds the node corresponding to the keywordId and returns its mindId, maxId.
// If keywordId not found returns (0, MAX_ALLOCATED_KEYWORD_ID).
void Trie::getPrefixString(const TrieNode* rootReadView, const TrieNode* trieNode, std::vector<CharType> &in) const
{
    in.clear();
    if (trieNode == NULL)
        return;

    unsigned minId = trieNode->getMinId();
    unsigned prefix_node_depth = trieNode->getDepth();
    in.resize(prefix_node_depth); // allocate space for the input string

    const TrieNode* nodeIter = rootReadView;
    while (nodeIter->getDepth() < prefix_node_depth) {
        nodeIter = nodeIter->findLowerBoundChildByMinId(minId);
        ASSERT(nodeIter != NULL);
        ASSERT(nodeIter->getDepth() <= prefix_node_depth);

        in.at(nodeIter->getDepth() - 1) = nodeIter->getCharacter();
    }

    return;
}

void Trie::getPrefixString(const TrieNode* rootReadView, const TrieNode* trieNode, std::string &in) const
{
    vector<CharType> temp;
    getPrefixString(rootReadView, trieNode, temp);
    charTypeVectorToUtf8String(temp, in);
}

/// NOT THREADSAFE WITH WRITERS. Used by QuadTree and unit test cases
void Trie::getPrefixString_NotThreadSafe(const TrieNode* trieNode, std::vector<CharType> &in) const
{
    in.clear();
    unsigned minId = trieNode->getMinId();
    //std::string *prefix =  new string();
    in.resize(trieNode->getDepth());
    if (trieNode->getDepth() == 0) {
        return;
    } else {
        // get root node
        boost::shared_ptr<TrieRootNodeAndFreeList > rootNode_readView;
        this->getTrieRootNode_ReadView(rootNode_readView);
        const TrieNode* nodeIter = rootNode_readView->root;
        //const TrieNode* nodeIter = rootReadView;
        // traverse down the trie
        // First iteration, skip root.
        bool skip_root = true;
        do {
            if (skip_root) {
                skip_root = false; // TODO: check this logic
            } else {
                nodeIter = nodeIter->findLowerBoundChildByMinId(minId);
                if (nodeIter != NULL) {
                    in.at(nodeIter->getDepth() - 1) = nodeIter->getCharacter();
                } else {
                    break;
                }
            }
        } while ( nodeIter->getDepth() < trieNode->getDepth() );
        //return *prefix;
        //in = prefix;
        return;
    }
}

void Trie::getPrefixString_NotThreadSafe(const TrieNode* trieNode, std::string &in) const
{
    vector<CharType> temp;
    getPrefixString_NotThreadSafe(trieNode, temp);
    charTypeVectorToUtf8String(temp, in);
}

void Trie::printSubTrie(const TrieNode *root, const TrieNode *node, set<unsigned>& keywordIds) const
{
    ASSERT (node != NULL);
    if (node == NULL)
        return;
    string temp;
    this->getPrefixString(root, node, temp);
    Logger::debug("(%d,%f,%s)", node->getId() , node->getNodeProbabilityValue() , temp.c_str());
    if ( node->isTerminalNode() ) {
        if (!keywordIds.count(node->getId()))
            keywordIds.insert(node->getId());
        else {
            vector<CharType> temp;
            this->getPrefixString(root, node, temp);
        }
        vector<CharType> temp;
        this->getPrefixString(root, node, temp);
        if ( this->getTrieNode(root, temp)->getMinId() > this->getTrieNode(root, temp)->getMaxId() ) {
            //std::cout << "wrong!!-->|";
        }
    } else {
        vector<CharType> temp;
        this->getPrefixString(root, node, temp);

        if ( this->getTrieNode(root, temp)->getMinId() > this->getTrieNode(root, temp)->getMaxId() ) {
            //std::cout << "wrong!!-->|";
        }
    }

    for (unsigned childIterator = 0; childIterator < node->getChildrenCount(); childIterator++ ) {
        this->printSubTrie(root, node->getChild(childIterator), keywordIds);
    }
}

void Trie::print_Trie() const
{
    typedef boost::shared_ptr<TrieRootNodeAndFreeList > TrieRootNodeSharedPtr;
    TrieRootNodeSharedPtr rootSharedPtr;
    this->getTrieRootNode_ReadView(rootSharedPtr);
    TrieNode *root = rootSharedPtr->root;

    set<unsigned> keywordIds;
    Logger::debug("------------------------------" );
    //this->printSubTrie(this->getRoot());
    this->printSubTrie(root, root, keywordIds);

    Logger::debug("------------------------------" );
}

}
}
