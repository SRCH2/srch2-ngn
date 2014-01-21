// $Id$ 07/11/13 Jamshid

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

 * Copyright © 2013 SRCH2 Inc. All rights reserved
 */

#ifndef __WRAPPER_TREEOPERATIONS_H__
#define __WRAPPER_TREEOPERATIONS_H__

#include "instantsearch/Constants.h"
#include "ParsedParameterContainer.h"


using namespace std;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class TreeOperations{
public:
	/*
	 * This function removes nodeToRemove from a tree that its root is 'root'
	 * if the root changes, the 'root' variable is assigned to the new root (which
	 *  might be null in case all the tree is deleted)
	 *
	 *
	 * Example:
	 *
	 * [OR]___ {hello}
	 *  |
	 *  |__[AND]__ {to}
	 *       |
	 *       |____ {yellow}
	 *
	 *  after removing stopwords by some module like analyzer, function will be called
	 *   and then the tree should look like :
	 *
	 *  [OR]___ {hello}
	 *    |
	 *    |____ {yellow}
	 */
	static void removeFromTree(ParseTreeNode * nodeToRemove, ParseTreeNode *& root){
		if(nodeToRemove == NULL) return;
		if(nodeToRemove->children.size() == 0){
			// first remove this pointer from parent list
			removeFromParentChildren(nodeToRemove);
			// save the pointer to parent
			ParseTreeNode * parent = nodeToRemove->parent;
			// delete the object
			delete nodeToRemove;
			if(root == nodeToRemove){
				root = NULL;
				return;
			}
			// try deleting the parent
			removeFromTree(parent,root);
		}else if(nodeToRemove->children.size() == 1){
			//ASSERT(nodeToRemove->type != NOT);
			if(nodeToRemove == root){ // single child is going to be root now
				// set the child as the new root
				root = nodeToRemove->children.at(0);
				root->parent = NULL;
				// remove child from children list and delete the node
				nodeToRemove->children.clear();
				delete nodeToRemove;
				return;
			}else{
				// save the pointer to parent
				ParseTreeNode * parent = nodeToRemove->parent;
				// set the parent of this single child to nodeToRemove's own parent
				nodeToRemove->children.at(0)->parent = parent;
				// change to pointer to nodeToRemove to its single child in parent's list
				changePointerInParentChildrenList(nodeToRemove , nodeToRemove->children.at(0));
				// remove child from it's children list
				nodeToRemove->children.clear();
				// delete nodeToRemove
				delete nodeToRemove;
				if(root == nodeToRemove){
					root = NULL;
					return;
				}
				// try removing the parent
				removeFromTree(parent,root);
			}
		} // if it has more than 1 child, we cannot remove it
	}


	/*
	 * This function applies two rewrite rules to the tree
	 * 1. It combines pairs of NOT together
	 * Example :
	 * [NOT]___[NOT]___[NOT]____{A}
	 * becomes :
	 * [NOT]___{A}
	 *
	 * 2. It merges same-operator levels together
	 * Example :
	 * [AND]____ ...
	 *   |
	 *   |___[AND]_____ {A}
	 *   |       |_____ {B}
	 *   |
	 *   |...
	 *
	 * Becomes :
	 *
	 * [AND]____ ...
	 *   |______ {A}
	 *   |______ {B}
	 *   |...
	 *
	 */
	static ParseTreeNode * mergeSameOperatorLevels(ParseTreeNode * subTreeRoot){
		if(subTreeRoot == NULL) return subTreeRoot;

		ParseTreeNode * parent = subTreeRoot->parent;

		vector<ParseTreeNode *> newChildrenList;
		for(vector<ParseTreeNode *>::iterator child = subTreeRoot->children.begin() ; child != subTreeRoot->children.end() ; ++child){
			*child = mergeSameOperatorLevels(*child);

			/*
			 * Example :
			 * [NOT]___[NOT]___[NOT]____{A}
			 * becomes :
			 * [NOT]___{A}
			 */
			if(subTreeRoot->type == (*child)->type && subTreeRoot->type == LogicalPlanNodeTypeNot){

				// set the parent of child of child to parent of subTreeRoot
				if((*child)->children.size() != 1 || subTreeRoot->children.size() != 1){ // NOT has no terms, or more than 1 term, wrong!!!
					//ASSERT(false);
					cout << "!!!!!!!!" << endl;
					return subTreeRoot;
				}
				ParseTreeNode * childOfChild = (*child)->children.at(0);
				childOfChild->parent = parent;
				// if parent is not NULL, change to pointer of subTreeRoot in parent's list to childOfchild
				if(parent != NULL){
					changePointerInParentChildrenList(subTreeRoot, childOfChild);
				}
				// clear child's children list
				(*child)->children.clear();
				// delete subTreeRoot and (*child)
				delete subTreeRoot; // since child is also in subTreeRoot's children list, it'll get deleted too
				subTreeRoot = childOfChild;
				return subTreeRoot;
			}


			/*
			 * Example :
			 * [AND]____ ...
			 *   |
			 *   |___[AND]_____ {A}
			 *   |       |_____ {B}
			 *   |
			 *   |...
			 *
			 * Becomes :
			 *
			 * [AND]____ ...
			 *   |______ {A}
			 *   |______ {B}
			 *   |...
			 */
			if((subTreeRoot->type == (*child)->type && subTreeRoot->type == LogicalPlanNodeTypeAnd)
					|| (subTreeRoot->type == (*child)->type && subTreeRoot->type == LogicalPlanNodeTypeOr)){
				// move on all children of 'child' and set their parents to 'subTreeRoot'
				for(vector<ParseTreeNode *>::iterator childOfChild = (*child)->children.begin() ; childOfChild != (*child)->children.end() ; ++childOfChild){
					(*childOfChild)->parent = subTreeRoot;
				}
				// inject all children of 'child' instead of 'child' itself in 'subTreeRoot's children list
				newChildrenList.insert(newChildrenList.end() , (*child)->children.begin() , (*child)->children.end() );
				// clear 'child's children list so that when we delete it, it doesn't delete the children
				(*child)->children.clear();
				// delete child
				delete *(child);
			}else{
				newChildrenList.push_back(*child);
			}
		}
		// now clear children list and copy the new list
		subTreeRoot->children.clear();
		subTreeRoot->children.insert(subTreeRoot->children.begin() , newChildrenList.begin() , newChildrenList.end());

		return subTreeRoot;
	}


private:

	/*
	 * This function removes nodeToRemove from its parent's children list
	 */
	static void removeFromParentChildren(ParseTreeNode * nodeToRemove){
		if(nodeToRemove == NULL) return;
		ParseTreeNode * parent = nodeToRemove->parent;
		if(parent == NULL) return;
		parent->children.erase(std::remove(parent->children.begin(), parent->children.end(), nodeToRemove), parent->children.end());
	}

	/*
	 * This function changes the pointer to nodeFrom to nodeTo in the children list of nodeFrom's parent.
	 */
	static void changePointerInParentChildrenList(ParseTreeNode * nodeFrom, ParseTreeNode * nodeTo){
		ParseTreeNode * parent = nodeFrom->parent;
		if(parent == NULL) return;
		for(vector<ParseTreeNode *>::iterator childPtr = parent->children.begin() ; childPtr != parent->children.end(); ++childPtr){
			if(*childPtr == nodeFrom){
				*childPtr = nodeTo;
				return;
			}
		}
	}
};

}
}

#endif // __WRAPPER_TREEOPERATIONS_H__
