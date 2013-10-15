
// $Id: Trie.h 3456 2013-06-14 02:11:13Z jiaying $

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

#ifndef __TRIE_H__
#define __TRIE_H__

///@file Trie.cpp
///@brief Trie and TrieNode Implementation

//#include "index.h"

//#include <algorithm/ActiveNode.h>

#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/cstdint.hpp>

#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <utility>
#include <cassert>
#include <set>
#include <cassert>
#include <queue>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include "util/mypthread.h"
#include "util/Assert.h"
#include "util/encoding.h"
#include "index/InvertedIndex.h"

using std::endl;
using std::set;
using std::vector;
using std::map;
using std::queue;
using namespace boost::serialization;

namespace srch2
{
namespace instantsearch
{

// this character is reserved as a special symbol for the trie
#define TRIE_MARKER_CHARACTER ('$')

// this class is mainly used in geo index
class Prefix
{
public:
    unsigned minId;
    unsigned maxId;

    Prefix() {}
    // TODO suggested by Jianfeng, if Prefix doesn't have any subclass, we might don't need virtual here
    //      can save some space
    virtual ~Prefix() {};
    Prefix(unsigned inputMinId, unsigned inputMaxId):minId(inputMinId),maxId(inputMaxId) {}

    inline bool operator==(const Prefix &p) const {
        return minId == p.minId
               && maxId == p.maxId;
    }

    inline bool operator!=(const Prefix &p) const {
        return !operator==(p);
    }

    inline bool operator<(const Prefix &p) const {
        return minId < p.minId
               || (minId == p.minId && maxId > p.maxId);
    }

    inline bool isAncestor(const Prefix &p) const {
        return minId <= p.minId && p.maxId <= maxId;
    }

    inline bool isStrictAncestor(const Prefix &p) const {
        return this->isAncestor(p) && *this != p;
    }

    inline void broaden(unsigned idLeft, unsigned idRight, unsigned specialBigValue) {
        if (idLeft < minId)
            minId = idLeft;

        if (idRight != specialBigValue && idRight > maxId)
            maxId = idRight;
    }

    inline void replace(unsigned oldId, unsigned newId) {
        if (minId == oldId)
            minId = newId;
        if (maxId == oldId)
            maxId = newId;
    }

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & this->minId;
        ar & this->maxId;
    }
};


class TrieNode
{
public:
    unsigned char terminalFlag1bDepth7b;
    CharType character;

    // Node sub-trie value
    float nodeSubTrieValue;
    /// Terminal-node members
    unsigned id;
    unsigned invertedListOffset;

    /// interial-node memebers
    TrieNode *leftMostDescendant;
    TrieNode *rightMostDescendant;
    std::vector<TrieNode*> childrenPointerList;

    // The following functions are used to get/set the leftInsertCounter and
    // rightInsertCounter for each leaf node in the trie.
    // They are used to assign an integer id for a new keyword inserted
    // into the trie.
    //
    // Context:
    // Suppose we want to insert a new keyword C into the range between
    // keyword A and keyword B, where A < B.  We need to assign a new integer ID
    // for C.  A naive solution of taking the average of the IDs of A and B
    // could quickly force us to get into the situation where we "run out of integer
    // IDs."  Thus we want to take a smarter approach to assigning this ID.
    //
    // Solution:
    // Each leaf node keeps two values, leftInsertCounter and
    // rightInsertCounter, which keep track of the number of keyword inserted
    // on the left and right, respectively.  Both values are inialized to
    // to be 1 during the commit phase of the trie, even for the leftmost node and
    // the rightmost node in the trie.
    //
    // For the new keyword C, we use the following formula to assign its new ID:
    //
    //   x = A.rightInsertCounter;
    //   y = B.leftInsertCounter;
    //   C.ID = A.ID + (B.ID - A.ID) * (x / (x + y));
    //   A.rightInsertCounter ++;
    //   B.leftInsertCounter ++;
    //
    // Intuition: as more keywords are inserted to the left side of a keyword,
    // we should push the new ID to the left.  We treat these two counters
    // as two weights, and use them to push the new integer to the keyword with
    // a smaller weight.
    inline unsigned getLeftInsertCounter() const {
        return insertCounters >> 16;
    }

    inline unsigned getRightInsertCounter() const {
        return (insertCounters) & 0xffff ;
    }

    inline void setLeftInsertCounter(const unsigned counter) {
        insertCounters = (insertCounters & 0xffff) + ((counter & 0xffff) << 16);
    }

    inline void setRightInsertCounter(const unsigned counter) {
        insertCounters = (insertCounters & 0xffff0000) + (counter & 0xffff);
    }

private:
    // a compact representation of leftInsertCounter and rightInsertCounter
    unsigned insertCounters;

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & terminalFlag1bDepth7b;
        ar & character;
        ar & nodeSubTrieValue;
        ar & id;
        ar & invertedListOffset;
        ar & leftMostDescendant;
        ar & rightMostDescendant;
        ar & childrenPointerList;
    }

    // for boost serialization
    TrieNode();

public:
    TrieNode(bool create_root);

    TrieNode(int depth, CharType character);

    TrieNode(const TrieNode *src);

    ~TrieNode();


    bool isRoot() const;

    void print_TrieNode() const;

    //getters and setters
    //void setCharacter(charT character) { this->character = character; }
    inline CharType getCharacter() const {
        return this->character;
    }

    // assert depth < 128
    inline void setDepth(unsigned depth) {
        this->terminalFlag1bDepth7b &= 0x80;
        this->terminalFlag1bDepth7b += depth & 0x7f;
    }
    inline unsigned getDepth() const {
        return static_cast<int>(this->terminalFlag1bDepth7b & 0x7f);
    }

    //Terminal node methods
    inline void setInvertedListOffset(unsigned invertedListOffset) {
        this->invertedListOffset = invertedListOffset;
    }
    inline unsigned getInvertedListOffset() const {
        return this->invertedListOffset;
    }

    /// a "terminalnode" is a trie node that corresponds to a complete keyword
    inline bool isTerminalNode() const {
        return ((this->terminalFlag1bDepth7b & 0x80) == (0x80));
    }
    inline void setTerminalFlag(bool flag) {
        if ( flag ) {
            this->terminalFlag1bDepth7b |= 0x80;
        } else {
            this->terminalFlag1bDepth7b &= 0x7f;
        }
    }

    // interial-node methods
    inline void setLeftMostDescendant(TrieNode *leftMostDescendant) {
        this->leftMostDescendant = leftMostDescendant;
    }
    inline void setRightMostDescendant(TrieNode *rightMostDescendant) {
        this->rightMostDescendant = rightMostDescendant;
    }


    inline TrieNode *getLeftMostDescendant() const {
        return this->leftMostDescendant;
    }
    inline TrieNode *getRightMostDescendant() const {
        return this->rightMostDescendant;
    }

    inline unsigned getMinId() const {
        return this->leftMostDescendant->getId();
    }
    inline unsigned getMaxId() const {
        return this->rightMostDescendant->getId();
    }

    ///friend + templates creates a mess of a code. So, using a normal getter and a const getter
    inline TrieNode *getChild(const int position) {
        return childrenPointerList[position];
    }
    inline TrieNode const *getChild(const int position) const {
        return childrenPointerList[position];
    }

    inline unsigned getChildrenCount() const {
        return childrenPointerList.size();
    }

    // since the child could be copied, we test equality using their characters
    inline bool isFirstChild(const TrieNode *child) const {
        // return child == childrenPointerList;
        return child->getCharacter() == childrenPointerList.front()->getCharacter();
    }

    // since the child could be copied, we test equality using their characters
    inline bool isLastChild(const TrieNode *child) const {
        // return child == childrenPointerList.back();
        return child->getCharacter() == childrenPointerList.back()->getCharacter();
    }

    inline unsigned getId() const {  /*assert(this!= NULL);*/
        return this->id;
    }
    inline void setId( unsigned id ) { /*assert(this!= NULL);*/
        this->id = id;
    }

    inline float getNodeSubTrieValue() {
    	return this->nodeSubTrieValue;
    }

    inline void setNodeSubTrieValue(float nodeSubTrieValue) {
    	this->nodeSubTrieValue = nodeSubTrieValue;
    }

    void addNewNodeSubTrieValueToAggregatedValue(float newValue){
    	this->setNodeSubTrieValue(this->getNodeSubTrieValue() + newValue);
    }

    void addNewNodeSubTrieValueToAggregatedValueByUsingJointProbability(float newP){
    	// P(A or B) = P(A) + P(B) - P(A and B) = P(A) + P(B) - P(A)*P(B) // estimating P(A and B) with P(A)*P(B)
    	this->setNodeSubTrieValue(this->getNodeSubTrieValue() + newP - this->getNodeSubTrieValue() * newP);
    }
    void updateNodeSubTrieValueAggregatedValue(){
    	// iterate on children and aggregate the values
        unsigned int childIterator = 0;
        float updatedAggregatedValue = 0;
        for ( ; childIterator < this->getChildrenCount(); childIterator++ ) {
            updatedAggregatedValue += this->getChild(childIterator)->getNodeSubTrieValue();
        }
        // set the result in the class member
        this->setNodeSubTrieValue(updatedAggregatedValue);
    }

    void updateNodeSubTrieValueAggregatedValueByUsingJointProbability(){
    	if(this->getChildrenCount() == 0){
    		return;
    	}
    	// P(A or B or C or ... or F) =
    	//                     P(A) + P(B or C .. or F) - P(A) + P(B or C .. or F) = ... (RECURSIVE)
    	// iterate on children and aggregate the values
    	float startingChildProbability = this->getChild(0)->nodeSubTrieValue;
    	float restOfNodesJointProbability = nodeSubTrieValueAggregatedValueByUsingJointProbabilityRecursive(1);

        // set the result in the class member
        // P(A)+P(B)+...+P(F) - P(A)*P(B)*...*P(F)
        this->setNodeSubTrieValue(startingChildProbability + restOfNodesJointProbability - startingChildProbability*restOfNodesJointProbability);
    }

    float nodeSubTrieValueAggregatedValueByUsingJointProbabilityRecursive(unsigned startingChild = 0){
    	if(startingChild >= this->getChildrenCount()){
    		return 0;
    	}
    	float startingChildProbability = this->getChild(startingChild)->nodeSubTrieValue;
    	float restOfNodesJointProbability = nodeSubTrieValueAggregatedValueByUsingJointProbabilityRecursive(startingChild + 1);
    	return startingChildProbability + restOfNodesJointProbability - startingChildProbability*restOfNodesJointProbability;
    }



    void findMostPopularSuggestionsInThisSubTrie(unsigned ed, vector<pair< pair< float , unsigned > , const TrieNode *> > & suggestions,const int numberOfSuggestionsToFind = 10) const;

    void addChild(CharType character, TrieNode *childNode);

    void addChild(int position, TrieNode *childNode);

    // Use carefully to save from memory leaks, used only in non-blocking single writer mode.
    void setChild(int position, TrieNode* childNode);

    TrieNode *findChild(CharType childCharacter) const;

    int findChildNodePosition(CharType childCharacter) const;

    unsigned getByteSizeOfCurrentNode() const;


    unsigned getNumberOfBytes() const;

    unsigned getNumberOfNodes() const;

    unsigned getfinalKeywordIdCounter() const;

    const TrieNode *findLowerBoundChildByMinId(unsigned minId) const;

    int findLowerBoundChildNodePositionByMinId(unsigned minId) const;

};

class TrieRootNodeAndFreeList
{
public:
    vector<const TrieNode* > free_list;
    TrieNode *root;

    TrieRootNodeAndFreeList();

    TrieRootNodeAndFreeList(const TrieNode *src);

    ~TrieRootNodeAndFreeList();

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & root;
        //ar & free_list; // We don't serialise the free_list members. They are to be deleted.
    }

};

class TrieNodePath
{
public:
    TrieNodePath();

    TrieNodePath(vector<TrieNode* > *p);

    ~TrieNodePath() {};

    void clean();

    // get the last trie node on the path
    TrieNode * getLastTrieNode() const;

    inline bool operator<(const TrieNodePath &tnp) const {
        if (this->getLastTrieNode()->character == TRIE_MARKER_CHARACTER)  // TODO: to check
            return true;

        if (tnp.getLastTrieNode()->character == TRIE_MARKER_CHARACTER)
            return false;

        unsigned i = 0;
        while (i < path->size() && i < tnp.path->size()) {
            if (path->at(i)->character < tnp.path->at(i)->character)
                return true;

            if (path->at(i)->character > tnp.path->at(i)->character)
                return false;

            i ++;
        }

        if (i == path->size() && i < tnp.path->size())
            return true;


        return false;
    }

    vector<TrieNode* > *path;
};

class Trie
{
public:
    // we partition the integer space into two ranges;
    //  Range 1: [0 - x], for keywords already with integer ids;
    //  Range 2: [x + 1, 2^32 - 1], for new keywords that can get an order-preserving integer ids;
    // Here "x" is MAX_ALLOCATED_KEYWORD_ID.
    static const unsigned MAX_KEYWORD_ID = ~0;  // should be 2^32 - 1
    static const unsigned MAX_ALLOCATED_KEYWORD_ID = 0xffff0000;

    static const unsigned KEYWORD_ID_SPARSITY = 10;

    // we cannot take keywords more than 127 characters (not including the marker character in the root)
    static const unsigned TRIE_MAX_DEPTH = 127;

private:
    boost::shared_ptr<TrieRootNodeAndFreeList> root_readview;
    TrieNode *root_writeview;
    mutable pthread_spinlock_t m_spinlock;

    // We keep the old read views in a queue. The goal is to make sure trie nodes in these views
    // can be freed in the order the read views were added into the queue.
    queue<boost::shared_ptr<TrieRootNodeAndFreeList> > oldReadViewQueue;

    unsigned numberOfTerminalNodes;

    typedef TrieNode* TrieNodePtr;
    typedef map<TrieNodePtr,TrieNodePtr> OldToNewTrieNodeMap;

    unsigned counterForReassignedKeywordIds; // a counter for the new keywords that need to be reassigned
    // a vector of trie nodes who keyword ids need to be reassigned
    std::vector<TrieNodePath > trieNodesToReassign;

    //BuildPhase Variables
    //used in forwardIndexCommit
    std::vector<unsigned> *oldIdToNewIdMapVector;

    bool commited;
    bool merge_required;

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const {
        // We do NOT need to serialize the "committed" flag since the trie should have committed.
        // We do NOT need to serialize the "oldIdToNewIdMapVector" since it's only used before the commit.
        ar << numberOfTerminalNodes;
        ar << root_readview;
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version) {
        ar >> numberOfTerminalNodes;
        // We do NOT need to read the "committed" flag from the disk since the trie should have committed and the flag should true.
        // We do NOT need to read the "oldIdToNewIdMapVector" from the disk since it's only used before the commit and is no longer needed.
        commited = true;
        ar >> root_readview;
        this->root_writeview = new TrieNode(this->root_readview.get()->root);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version) {
        boost::serialization::split_member(ar, *this, file_version);
    }

public:

    Trie();

    virtual ~Trie();

    void deleteTrieNode(TrieNode* &trieNode);

    void getTrieRootNode_ReadView(boost::shared_ptr<TrieRootNodeAndFreeList >& trieRootNode_ReadView) const;

    TrieNode* getTrieRootNode_WriteView() const;

    // Helper function for addKeyword function. Called by both addKeyword(...) and addKeyword_ThreadSafe(...)
    // The leaf (or possibly internal) node is added (or turned marked to be a complete keyword) but it doesn't have an ID, this function
    // computes the id to be assigned to the new keyword and also modifies the left and right most descendants of its ancestors if necessary.
    void addKeyword_SetPrevIdNexIdByPathTrace(vector<TrieNode* > &pathTrace,
            bool isNewTrieNode, TrieNode *node);

    // Test if there are keywords whose ids need to be reassigned
    bool needToReassignKeywordIds();

    void printTriePath(vector<TrieNode* > *pathTrace);

    unsigned computeIdForNewKeyword(TrieNode* prevNode, TrieNode* nextNode);

    // Remove those characters whose ascii is equal to
    // the trie's marker character.
    // If the keyword has more than 127 characters, just take
    // the first 127 characters, since that's the limit of the
    // trie depth.
    void cleanString(const vector<CharType> &oldString, vector<CharType> &cleanedString) const;

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
    unsigned addKeyword(const std::vector<CharType> &keyword, unsigned &invertedListOffset);

    //for easy the test
    unsigned addKeyword(const std::string &keyword, unsigned &invertedListOffset);
    unsigned addKeyword_ThreadSafe(const std::string &keyword, unsigned &invertedListOffset);

    // We add this extra level for addKeyword_ThreadSafe, because
    // 1) the GeoIndex update needs isNewTerminalNode, so it can call addKeyword_ThreadSafe_Inner
    // 2) we don't want to change how other places use addKeyword_ThreadSafe
    unsigned addKeyword_ThreadSafe(const std::vector<CharType> &keyword, unsigned &invertedListOffset);

    unsigned addKeyword_ThreadSafe(const std::vector<CharType> &keyword, unsigned &invertedListOffset,
                                   bool &isNewTrieNode, bool &isNewInternalTerminalNode);

    void remapPathForTrieNodesToReassign(OldToNewTrieNodeMap &oldToNewTrieNodeMap);

    const TrieNode *getTrieNodeFromUtf8String(const TrieNode* rootReadView, const std::string &keywordStr) const;

    const TrieNode *getTrieNode(const TrieNode* rootReadView, const std::vector<CharType> &keyword) const;

    static void load(Trie &trie, const std::string &trieFullPathFileName);

    static void save(Trie &trie, const std::string &trieFullPathFileName);

    int getNumberOfBytes() const;

    int getNumberOfNodes() const;

    int getfinalKeywordIdCounter() const;

    /* Commit phase functions
     * Used in commitSubTrie
     */

    void commitSubTrie(TrieNode *node, unsigned &finalKeywordIdCounter, const unsigned sparsity);

    void checkCorrectness(const TrieNodePath &tnpp);

    // Get the left immediate neighbour TrieNode along with its path from root
    bool getLeftTnp(const TrieNodePath &currentTnp, TrieNodePath &prevTnp);

    // append to a TrieNodePath the trie nodes from the current "node" to its leftmost node
    void extendTnpFromTrieNode(TrieNode* node, TrieNodePath &tnp);

    // Get the right immediate neighbour TrieNode along with its path from root
    bool getRightTnp(const TrieNodePath &currentTnp, TrieNodePath &nextTnp);

    // See if the TrieNode is in the trieNodeIdMapper,
    // which stores all the TrieNodes that to be reassigned Ids.
    // If yes, return the the new Id in the map; if no, return the original Id
    unsigned getTrueId(TrieNode *node, const map<TrieNode *, unsigned> &trieNodeIdMapper);

    // check if the Id is a temporary large Id, which is larger than MAX_ALLOCATED_KEYWORD_ID
    bool isLargerThanUpperBound(unsigned id);

    // Calculate the sparsity in the current reassignRange to see if it's sparse enough
    bool needToReassignMore(TrieNode *leftNode, TrieNode *rightNode, const unsigned size,
                            const map<TrieNode *, unsigned> &trieNodeIdMapper);

    // Keep including more TrieNodes on the left side into reassignRange,
    // untill we reach a node that's not with a temporary reassigned id
    bool assignMoreNodesOnTheLeft(TrieNodePath &leftSide, vector<TrieNode *> &reassignRange,
                                  const map<TrieNode *, unsigned> &trieNodeIdMapper);

    // Keep including more TrieNodes on the right side into reassignRange,
    // untill we reach a node that's not with a temporary reassigned id
    bool assignMoreNodesOnTheRight(TrieNodePath &rightSide, vector<TrieNode *> &reassignRange,
                                   const map<TrieNode *, unsigned> &trieNodeIdMapper);

    // Uniformly distribute Ids for all the TrieNodes in reassignRange
    // according to what we have on the right and on the left and the number of trie nodes in the range we
    // assign new ids uniformly.
    void doReassignment(const vector<TrieNode *> &reassignRange, map<TrieNode *, unsigned> &trieNodeIdMapper);

    // For all the TrieNodes we need to reassign,
    // we first look at their left neighbours to see if we can adjust one or some of their Ids
    // to make more Id space so that we can keep the sparsity after inserting the new TrieNode
    // If we run out of space on the entire left side, we will then try the right side
    /*
     * @param iter is the iterator on trieNodesToReassign : the keyword to reassign id
     */
    void pushNeighborsForMoreSpace(const unsigned iter, map<TrieNode *, unsigned> &trieNodeIdMapper);

    // The trieNodeIdMapper stores those newly inserted TrieNodes that don't have Id space
    // with their temporary Ids. In this function we will adjust their neighbors' Ids to
    // make space for them and assign new Ids to them and their adjusted neighbors.
    /*
     * @param trieNodeIdMapper will keep the final correct ids of the trie nodes.
     */
    void reassignKeywordIds(map<TrieNode *, unsigned> &trieNodeIdMapper);

    /*
     * These two functions are supposed to be called in the last step of commit (after nulk load)
     * because they assume inverted and forward indices are ready. Unless invertedIndex
     * is NULL, in which case this value is actually just the frequency of leaf nodes in each subtrie.
     * The traverse the trie in pre-order to calculate the nodeSubTrieValue for each TrieNode
     */

    void calculateTrieNodeSubTrieValues(const InvertedIndex * invertedIndex ,  const unsigned totalNumberOfRecords);
    void calculateTrieNodeSubTrieValuesForANode(TrieNode *root, const InvertedIndex * invertedIndex , const unsigned totalNumberOfRecords );
    void printTrieNodeSubTrieValues(std::vector<CharType> & prefix , TrieNode * root , unsigned depth = 0);

    void merge(const InvertedIndex * invertedIndex , const unsigned totalNumberOfResults  , bool updateHistogram);

    void commit();

    /*
     * Calls calculateTrieNodeSubTrieValues and sets commit flag of trie to true
     * Final commit must be called after InvertedInde and ForwardIndex commits are done unless invertedIndex
     * is NULL, in which case this value is actually just the frequency of leaf nodes in each subtrie.
     */
    void finalCommit(const InvertedIndex * invertedIndex , const unsigned totalNumberOfResults );

    const std::vector<unsigned> *getOldIdToNewIdMapVector() const;

    void deleteOldIdToNewIdMapVector();

    void getAncestorPrefixes(const Prefix &prefix, std::vector<Prefix> &ancestorPrefixes) const;

    // TODO optimize
    bool getParentPrefix(const Prefix &prefix, Prefix &parentPrefix) const;

    bool isDirectChildPrefix(const Prefix &parent, const Prefix &descendant) const;

    // tell if the prefix can denote a terminal trieNode
    bool isPrefixACompleteKeyword(const Prefix &prefix) const;

    // return 1 left most
    //        2 right most
    //        0 none
    unsigned ifBreakOldParentPrefix(const std::vector<CharType> &keyword, vector<Prefix> *oldParentOrSelfAndAncs,
                                    bool &hadExactlyOneChild) const;

    void getPrefixFromKeywordId(unsigned keywordId, Prefix &prefix) const;

    unsigned getKeywordLength(unsigned keywordId) const;

    // Finds the node corresponding to the keywordId and returns its mindId, maxId.
    // If keywordId not found returns (0, MAX_ALLOCATED_KEYWORD_ID).
    void getKeywordMinMaxIdLength(unsigned keywordId, unsigned &minId, unsigned &maxId,
                                  unsigned &length) const;
    // for WriteView
    // Finds the node corresponding to the keywordId and returns its mindId, maxId.
    // If keywordId not found returns (0, MAX_ALLOCATED_KEYWORD_ID).
    void getKeywordMinMaxIdLength_WriteView(unsigned keywordId, unsigned &minId, unsigned &maxId,
                                            unsigned &length) const;

    // Finds the node corresponding to the keywordId and returns its mindId, maxId.
    // If keywordId not found returns (0, MAX_ALLOCATED_KEYWORD_ID).
    void getPrefixString(const TrieNode* rootReadView, const TrieNode* trieNode, std::vector<CharType> &in) const;

    void getPrefixString(const TrieNode* rootReadView, const TrieNode* trieNode, std::string &in) const;

    /// NOT THREADSAFE WITH WRITERS. Used by QuadTree and unit test cases
    void getPrefixString_NotThreadSafe(const TrieNode* trieNode, std::vector<CharType> &in) const;

    void getPrefixString_NotThreadSafe(const TrieNode* trieNode, std::string &in) const;

    void printSubTrie(const TrieNode *root, const TrieNode *node, set<unsigned>& keywordIds) const;

    void print_Trie() const;
};

}
}
#endif //__TRIE_H__
