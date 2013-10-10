//$Id: Trie_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

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

#include "index/Trie.h"
#include "util/Assert.h"

#include <iostream>
#include <functional>
#include <vector>
#include <cstring>
#include <cassert>

using namespace std;
using namespace srch2::instantsearch;

/*
 * The final trie in test1 should look like the following:
 * interialnode = (minid,maxid)
 * terminalnode = (id, minid, maxid)$
 *
 *
 *    root                  | t(112,112,112)$
 *      |                   |
 *      |       (32,112) (32,112)
 *      ----------- c ----- a  ----- n (32,32,96)$
 *      |                            |
 *      |                            |
 *      a(16,16)                     ---- a ----- d ----- a(48,48,48)$
 *      |                            |  (48,48) (48,48)
 *      |                            |
 *      n(16,16)                     ---- c ----- e ----- r(64,64,64)$
 *      |                            |   (64,64) (64,64)
 *      |                            |
 *      d(16,16,16)$                  ---- s(80,80,80)$
 *                                   |
 *                                   |
 *                                   ---- t ----- e ----- e ----- n(96,96,96)$
 *                                     (96,96)  (96,96) (96,96)
 *
 *
 */

void test1()
{
    const string filenameTrie("testTrieSerialize");
    string prefix1;
    string prefix2;

    Trie *trie1 = new Trie();

    string keyword1, keyword2, keyword3, keyword4, keyword5;
    keyword1.assign("cancer");
    keyword2.assign("canada");
    keyword3.assign("canteen");
    keyword4.assign("can");
    keyword5.assign("cat");

    unsigned keywordId1,keywordId2,keywordId3,keywordId4,keywordId5,keywordId6,keywordId7,keywordId8;

    const TrieNode *terminalNodeOfKeywordId;

    //TODO take care of upper case
    unsigned invertedIndexOffset;
    keywordId1 = trie1->addKeyword("cancer", invertedIndexOffset);
    keywordId2 = trie1->addKeyword("canada", invertedIndexOffset);
    keywordId3 = trie1->addKeyword("canteen", invertedIndexOffset);
    keywordId4 = trie1->addKeyword("cancer", invertedIndexOffset);
    keywordId5 = trie1->addKeyword("can", invertedIndexOffset);
    keywordId6 = trie1->addKeyword("cat", invertedIndexOffset);
    keywordId7 = trie1->addKeyword("and", invertedIndexOffset);
    keywordId8 = trie1->addKeyword("cans", invertedIndexOffset);

    string tmp = "";
    ASSERT(trie1->addKeyword(tmp,invertedIndexOffset) == 0); // empty string is not a valid input keyword

    ASSERT(keywordId1 == 0);
    ASSERT(keywordId2 == 1);
    ASSERT(keywordId3 == 2);
    ASSERT(keywordId4 == 0);
    ASSERT(keywordId5 == 3);
    ASSERT(keywordId6 == 4);
    ASSERT(keywordId7 == 5);
    ASSERT(keywordId8 == 6);

    trie1->commit();
    trie1->finalCommit(NULL);
    trie1->print_Trie();

    typedef boost::shared_ptr<TrieRootNodeAndFreeList > TrieRootNodeSharedPtr;
    TrieRootNodeSharedPtr rootSharedPtr1;
    trie1->getTrieRootNode_ReadView(rootSharedPtr1);
    TrieNode *root1 = rootSharedPtr1->root;

    terminalNodeOfKeywordId = trie1->getTrieNodeFromUtf8String( root1, "junk");
    ASSERT (terminalNodeOfKeywordId == NULL);

    keywordId1 = trie1->getTrieNodeFromUtf8String( root1, "cancer")->getId();
    keywordId2 = trie1->getTrieNodeFromUtf8String( root1, "canada")->getId();
    keywordId3 = trie1->getTrieNodeFromUtf8String( root1, "canteen")->getId();
    keywordId4 = trie1->getTrieNodeFromUtf8String( root1, "cancer")->getId();
    keywordId5 = trie1->getTrieNodeFromUtf8String( root1, "can")->getId();
    keywordId6 = trie1->getTrieNodeFromUtf8String( root1, "cat")->getId();
    keywordId7 = trie1->getTrieNodeFromUtf8String( root1, "and")->getId();
    keywordId8 = trie1->getTrieNodeFromUtf8String( root1, "cans")->getId();

    ASSERT (trie1->getTrieNodeFromUtf8String( root1, "and")->getId() < trie1->getTrieNodeFromUtf8String( root1, "can")->getId());
    ASSERT (trie1->getTrieNodeFromUtf8String( root1, "canada")->getId() < trie1->getTrieNodeFromUtf8String( root1, "cancer")->getId());
    ASSERT (trie1->getTrieNodeFromUtf8String( root1, "can")->getId() < trie1->getTrieNodeFromUtf8String( root1, "canada")->getId());
    ASSERT (trie1->getTrieNodeFromUtf8String( root1, "cancer")->getId() < trie1->getTrieNodeFromUtf8String( root1, "canteen")->getId());
    ASSERT (trie1->getTrieNodeFromUtf8String( root1, "canteen")->getId() < trie1->getTrieNodeFromUtf8String( root1, "cat")->getId());

    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "a")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "a")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "an")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "an")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "and")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "and")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "c")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "c")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "ca")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "ca")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "can")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "can")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cans")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cans")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cana")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cana")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canad")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "canad")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canada")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "canada")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canc")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "canc")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cance")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cance")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cant")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cant")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cante")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cante")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cantee")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cantee")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canteen")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "canteen")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "c")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "c")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "ca")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "ca")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cat")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cat")->getMaxId());

    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "a")->getMinId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "a")->getMaxId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "an")->getMinId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "an")->getMaxId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "and")->getMinId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "and")->getMaxId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cans")->getMinId() == keywordId8);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cans")->getMaxId() == keywordId8);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cana")->getMinId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cana")->getMaxId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canad")->getMinId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canad")->getMaxId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canada")->getMinId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canada")->getMaxId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canc")->getMinId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canc")->getMaxId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cance")->getMinId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cance")->getMaxId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMinId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMaxId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cant")->getMinId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cant")->getMaxId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cante")->getMinId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cante")->getMaxId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cantee")->getMinId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cantee")->getMaxId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canteen")->getMinId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canteen")->getMaxId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cat")->getMinId() == keywordId6);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cat")->getMaxId() == keywordId6);

    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "c")->getMinId() == keywordId5);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "c")->getMaxId() == keywordId6);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "ca")->getMinId() == keywordId5);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "ca")->getMaxId() == keywordId6);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "can")->getMinId() == keywordId5);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "can")->getMaxId() == keywordId3);

    cout << "-------------" << endl;

    Trie::save(*trie1,filenameTrie);
    if (terminalNodeOfKeywordId!=NULL)
        delete terminalNodeOfKeywordId;
    delete trie1;

    Trie *trie2 = new Trie();
    Trie::load(*trie2,filenameTrie);
    trie1 = trie2;

    trie1->getTrieRootNode_ReadView(rootSharedPtr1);
    root1 = rootSharedPtr1->root;

    ASSERT (trie1->getTrieNodeFromUtf8String( root1, "and")->getId() < trie1->getTrieNodeFromUtf8String( root1, "can")->getId());
    ASSERT (trie1->getTrieNodeFromUtf8String( root1, "canada")->getId() < trie1->getTrieNodeFromUtf8String( root1, "cancer")->getId());
    ASSERT (trie1->getTrieNodeFromUtf8String( root1, "can")->getId() < trie1->getTrieNodeFromUtf8String( root1, "canada")->getId());
    ASSERT (trie1->getTrieNodeFromUtf8String( root1, "cancer")->getId() < trie1->getTrieNodeFromUtf8String( root1, "canteen")->getId());
    ASSERT (trie1->getTrieNodeFromUtf8String( root1, "canteen")->getId() < trie1->getTrieNodeFromUtf8String( root1, "cat")->getId());

    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "a")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "a")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "an")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "an")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "and")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "and")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "c")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "c")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "ca")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "ca")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "can")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "can")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cans")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cans")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cana")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cana")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canad")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "canad")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canada")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "canada")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canc")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "canc")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cance")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cance")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cant")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cant")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cante")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cante")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cantee")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cantee")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canteen")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "canteen")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "c")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "c")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "ca")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "ca")->getMaxId());
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cat")->getMinId() <= trie1->getTrieNodeFromUtf8String( root1, "cat")->getMaxId());

    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "a")->getMinId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "a")->getMaxId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "an")->getMinId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "an")->getMaxId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "and")->getMinId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "and")->getMaxId() == keywordId7);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cans")->getMinId() == keywordId8);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cans")->getMaxId() == keywordId8);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cana")->getMinId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cana")->getMaxId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canad")->getMinId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canad")->getMaxId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canada")->getMinId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canada")->getMaxId() == keywordId2);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canc")->getMinId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canc")->getMaxId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cance")->getMinId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cance")->getMaxId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMinId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMaxId() == keywordId4);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cant")->getMinId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cant")->getMaxId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cante")->getMinId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cante")->getMaxId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cantee")->getMinId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cantee")->getMaxId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canteen")->getMinId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "canteen")->getMaxId() == keywordId3);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cat")->getMinId() == keywordId6);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "cat")->getMaxId() == keywordId6);

    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "c")->getMinId() == keywordId5);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "c")->getMaxId() == keywordId6);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "ca")->getMinId() == keywordId5);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "ca")->getMaxId() == keywordId6);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "can")->getMinId() == keywordId5);
    ASSERT(trie1->getTrieNodeFromUtf8String( root1, "can")->getMaxId() == keywordId3);

    delete trie2;
}


void printTest2(Trie *trie1, string input)
{
    typedef boost::shared_ptr<TrieRootNodeAndFreeList > TrieRootNodeSharedPtr;
    TrieRootNodeSharedPtr rootSharedPtr1;
    trie1->getTrieRootNode_ReadView(rootSharedPtr1);
    TrieNode *root1 = rootSharedPtr1->root;

    std::cout << input << "|"
              << trie1->getTrieNodeFromUtf8String( root1, input)->getId() << "|"
              << trie1->getTrieNodeFromUtf8String( root1, input)->getMinId() << "|"
              << trie1->getTrieNodeFromUtf8String( root1, input)->getMaxId() << endl;
}
void test2()
{
    Trie *trie1 = new Trie();

    unsigned invertedIndexOffset;

    //"Tom Smith and Jack Lennon come Yesterday Once More"
    trie1->addKeyword("tom", invertedIndexOffset);
    trie1->addKeyword("smith", invertedIndexOffset);
    trie1->addKeyword("and", invertedIndexOffset);
    trie1->addKeyword("jack", invertedIndexOffset);
    trie1->addKeyword("lennon", invertedIndexOffset);
    trie1->addKeyword("come", invertedIndexOffset);
    trie1->addKeyword("yesterday", invertedIndexOffset);
    trie1->addKeyword("once", invertedIndexOffset);
    trie1->addKeyword("more", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "George Harris");
    record->setAttributeValue(1, "Here comes the sun");
     */
    trie1->addKeyword("george", invertedIndexOffset);
    trie1->addKeyword("harris", invertedIndexOffset);
    trie1->addKeyword("here", invertedIndexOffset);
    trie1->addKeyword("comes", invertedIndexOffset);
    trie1->addKeyword("the", invertedIndexOffset);
    trie1->addKeyword("sun", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "Pink Floyd");
    record->setAttributeValue(1, "Shine on you crazy diamond");
     */
    trie1->addKeyword("pink", invertedIndexOffset);
    trie1->addKeyword("floyd", invertedIndexOffset);
    trie1->addKeyword("shine", invertedIndexOffset);
    trie1->addKeyword("on", invertedIndexOffset);
    trie1->addKeyword("you", invertedIndexOffset);
    trie1->addKeyword("crazy", invertedIndexOffset);
    trie1->addKeyword("diamond", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "Uriah Hepp");
    record->setAttributeValue(1, "Come Shine away Melinda ");
     */
    trie1->addKeyword("uriah", invertedIndexOffset);
    trie1->addKeyword("hepp", invertedIndexOffset);
    trie1->addKeyword("come", invertedIndexOffset);
    trie1->addKeyword("shine", invertedIndexOffset);
    trie1->addKeyword("away", invertedIndexOffset);
    trie1->addKeyword("melinda", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "Pinksyponzi Floydsyponzi");
    record->setAttributeValue(1, "Shinesyponzi on - Wish you were here");
     */
    trie1->addKeyword("pinksyponzi", invertedIndexOffset);
    trie1->addKeyword("floydsyponzi", invertedIndexOffset);
    trie1->addKeyword("shinesyponzi", invertedIndexOffset);
    trie1->addKeyword("on", invertedIndexOffset);
    trie1->addKeyword("wish", invertedIndexOffset);
    trie1->addKeyword("you", invertedIndexOffset);
    trie1->addKeyword("were", invertedIndexOffset);
    trie1->addKeyword("here", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "U2 2345 Pink");
    record->setAttributeValue(1, "with or without you");
     */
    trie1->addKeyword("u2", invertedIndexOffset);
    trie1->addKeyword("2345", invertedIndexOffset);
    trie1->addKeyword("pink", invertedIndexOffset);
    trie1->addKeyword("with", invertedIndexOffset);
    trie1->addKeyword("or", invertedIndexOffset);
    trie1->addKeyword("without", invertedIndexOffset);
    trie1->addKeyword("you", invertedIndexOffset);

    /*record->setAttributeValue(0, "Led Zepplelin");
    record->setAttributeValue(1, "Stairway to Heaven pink floyd");
     */
    trie1->addKeyword("led", invertedIndexOffset);
    trie1->addKeyword("zepplelin", invertedIndexOffset);
    trie1->addKeyword("stairway", invertedIndexOffset);
    trie1->addKeyword("to", invertedIndexOffset);
    trie1->addKeyword("heaven", invertedIndexOffset);
    trie1->addKeyword("pink", invertedIndexOffset);
    trie1->addKeyword("floyd", invertedIndexOffset);

    /*record->setAttributeValue(0, "Jimi Hendrix");
    record->setAttributeValue(1, "Little wing");
     */
    trie1->addKeyword("jimi", invertedIndexOffset);
    trie1->addKeyword("hendrix", invertedIndexOffset);
    trie1->addKeyword("little", invertedIndexOffset);
    trie1->addKeyword("wing", invertedIndexOffset);
    trie1->commit();
    trie1->finalCommit(NULL);
    trie1->print_Trie();

    cout<<"\nBefore Commit:" << std::endl;

    trie1->addKeyword("winger", invertedIndexOffset);
    trie1->addKeyword("wing", invertedIndexOffset);
    trie1->addKeyword("aaaa", invertedIndexOffset);
    trie1->addKeyword("qqqq", invertedIndexOffset);
    trie1->addKeyword("zzzz", invertedIndexOffset);

    trie1->addKeyword("steve", invertedIndexOffset);
    trie1->addKeyword("jobs", invertedIndexOffset);
    trie1->addKeyword("stanford", invertedIndexOffset);
    trie1->addKeyword("speech", invertedIndexOffset);

    trie1->addKeyword("000000000000000000000", invertedIndexOffset);
    trie1->addKeyword("00000000000000000000", invertedIndexOffset);
    trie1->addKeyword("0000000000000000000", invertedIndexOffset);
    trie1->addKeyword("00000000000000000", invertedIndexOffset);
    trie1->addKeyword("0000000000000000022", invertedIndexOffset);
    trie1->addKeyword("0000000000000000", invertedIndexOffset);
    trie1->addKeyword("000000000000022", invertedIndexOffset);
    trie1->addKeyword("00000000000", invertedIndexOffset);
    trie1->addKeyword("0000000000088", invertedIndexOffset);

    trie1->merge(NULL, false);

    cout<<"\nAfter Commit and Update:\n" << std::endl;

    trie1->print_Trie();

    delete trie1;

}

void test2_ThreadSafe()
{
    Trie *trie1 = new Trie();

    unsigned invertedIndexOffset;

    //"Tom Smith and Jack Lennon come Yesterday Once More"
    trie1->addKeyword("tom", invertedIndexOffset);
    trie1->addKeyword("smith", invertedIndexOffset);
    trie1->addKeyword("and", invertedIndexOffset);
    trie1->addKeyword("jack", invertedIndexOffset);
    trie1->addKeyword("lennon", invertedIndexOffset);
    trie1->addKeyword("come", invertedIndexOffset);
    trie1->addKeyword("yesterday", invertedIndexOffset);
    trie1->addKeyword("once", invertedIndexOffset);
    trie1->addKeyword("more", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "George Harris");
    record->setAttributeValue(1, "Here comes the sun");
     */
    trie1->addKeyword("george", invertedIndexOffset);
    trie1->addKeyword("harris", invertedIndexOffset);
    trie1->addKeyword("here", invertedIndexOffset);
    trie1->addKeyword("comes", invertedIndexOffset);
    trie1->addKeyword("the", invertedIndexOffset);
    trie1->addKeyword("sun", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "Pink Floyd");
    record->setAttributeValue(1, "Shine on you crazy diamond");
     */
    trie1->addKeyword("pink", invertedIndexOffset);
    trie1->addKeyword("floyd", invertedIndexOffset);
    trie1->addKeyword("shine", invertedIndexOffset);
    trie1->addKeyword("on", invertedIndexOffset);
    trie1->addKeyword("you", invertedIndexOffset);
    trie1->addKeyword("crazy", invertedIndexOffset);
    trie1->addKeyword("diamond", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "Uriah Hepp");
    record->setAttributeValue(1, "Come Shine away Melinda ");
     */
    trie1->addKeyword("uriah", invertedIndexOffset);
    trie1->addKeyword("hepp", invertedIndexOffset);
    trie1->addKeyword("come", invertedIndexOffset);
    trie1->addKeyword("shine", invertedIndexOffset);
    trie1->addKeyword("away", invertedIndexOffset);
    trie1->addKeyword("melinda", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "Pinksyponzi Floydsyponzi");
    record->setAttributeValue(1, "Shinesyponzi on - Wish you were here");
     */
    trie1->addKeyword("pinksyponzi", invertedIndexOffset);
    trie1->addKeyword("floydsyponzi", invertedIndexOffset);
    trie1->addKeyword("shinesyponzi", invertedIndexOffset);
    trie1->addKeyword("on", invertedIndexOffset);
    trie1->addKeyword("wish", invertedIndexOffset);
    trie1->addKeyword("you", invertedIndexOffset);
    trie1->addKeyword("were", invertedIndexOffset);
    trie1->addKeyword("here", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "U2 2345 Pink");
    record->setAttributeValue(1, "with or without you");
     */
    trie1->addKeyword("u2", invertedIndexOffset);
    trie1->addKeyword("2345", invertedIndexOffset);
    trie1->addKeyword("pink", invertedIndexOffset);
    trie1->addKeyword("with", invertedIndexOffset);
    trie1->addKeyword("or", invertedIndexOffset);
    trie1->addKeyword("without", invertedIndexOffset);
    trie1->addKeyword("you", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "Led Zepplelin");
    record->setAttributeValue(1, "Stairway to Heaven pink floyd");
     */
    trie1->addKeyword("led", invertedIndexOffset);
    trie1->addKeyword("zepplelin", invertedIndexOffset);
    trie1->addKeyword("stairway", invertedIndexOffset);
    trie1->addKeyword("to", invertedIndexOffset);
    trie1->addKeyword("heaven", invertedIndexOffset);
    trie1->addKeyword("pink", invertedIndexOffset);
    trie1->addKeyword("floyd", invertedIndexOffset);

    /*
    record->setAttributeValue(0, "Jimi Hendrix");
    record->setAttributeValue(1, "Little wing");
     */
    trie1->addKeyword("jimi", invertedIndexOffset);
    trie1->addKeyword("hendrix", invertedIndexOffset);
    trie1->addKeyword("little", invertedIndexOffset);
    trie1->addKeyword("wing", invertedIndexOffset);
    trie1->commit();
    trie1->finalCommit(NULL);
    trie1->print_Trie();

    cout<<"\nBefore Commit:" << std::endl;

    trie1->addKeyword_ThreadSafe("winger", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("wing", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("aaaa", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("qqqq", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("zzzz", invertedIndexOffset);

    trie1->addKeyword_ThreadSafe("steve", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("jobs", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("stanford", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("speech", invertedIndexOffset);

    trie1->addKeyword_ThreadSafe("000000000000000000000", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("00000000000000000000", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("0000000000000000000", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("00000000000000000", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("0000000000000000022", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("0000000000000000", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("000000000000022", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("00000000000", invertedIndexOffset);
    trie1->addKeyword_ThreadSafe("0000000000088", invertedIndexOffset);

    trie1->merge(NULL, false);

    map<TrieNode *, unsigned> trieNodeIdMapper;
    trie1->reassignKeywordIds(trieNodeIdMapper);
    for (map<TrieNode *, unsigned>::iterator iter = trieNodeIdMapper.begin();
         iter != trieNodeIdMapper.end(); ++ iter)
    {
         TrieNode *node = iter->first;
         unsigned newKeywordId = iter->second;
         //std::cout << "ForwardIndex:reassign, " << node->getId() << " -> " << newKeywordId << std::endl;
         node->setId(newKeywordId); // set the new keyword Id
    }

    cout<<"\nAfter Commit and Update:\n" << std::endl;

    trie1->print_Trie();

    delete trie1;

}

void test3()
{
    Trie *trie1 = new Trie();

    unsigned invertedIndexOffset;

    //"Tom Smith and Jack Lennon come Yesterday Once More"
    trie1->addKeyword("tom", invertedIndexOffset);
    trie1->addKeyword("smith", invertedIndexOffset);
    trie1->addKeyword("and", invertedIndexOffset);
    trie1->addKeyword("jack", invertedIndexOffset);
    trie1->addKeyword("lennon", invertedIndexOffset);
    trie1->addKeyword("come", invertedIndexOffset);
    trie1->addKeyword("yesterday", invertedIndexOffset);
    trie1->addKeyword("once", invertedIndexOffset);
    trie1->addKeyword("more", invertedIndexOffset);

    trie1->addKeyword("jimi", invertedIndexOffset);
    trie1->addKeyword("hendrix", invertedIndexOffset);
    trie1->addKeyword("little", invertedIndexOffset);
    trie1->addKeyword("wing", invertedIndexOffset);
    trie1->commit();
    trie1->finalCommit(NULL);
    trie1->print_Trie();

    cout<<"\nBefore Commit:" << std::endl;

    trie1->addKeyword("steve", invertedIndexOffset);
    trie1->addKeyword("jobs", invertedIndexOffset);
    trie1->addKeyword("stanford", invertedIndexOffset);
    trie1->addKeyword("speech", invertedIndexOffset);

    cout<<"\nAfter Commit and Update:\n" << std::endl;

    trie1->print_Trie();

    delete trie1;

}

void test4()
{
    cout << "\n\nTrie Update with serialization" << endl;
    const string filenameTrie("testTrieSerialize");
    Trie *trie1 = new Trie();

    unsigned invertedIndexOffset;

    //"Tom Smith and Jack Lennon come Yesterday Once More"
    trie1->addKeyword("tom", invertedIndexOffset);
    trie1->addKeyword("smith", invertedIndexOffset);
    trie1->addKeyword("and", invertedIndexOffset);
    trie1->addKeyword("jack", invertedIndexOffset);
    trie1->addKeyword("lennon", invertedIndexOffset);
    trie1->addKeyword("come", invertedIndexOffset);
    trie1->addKeyword("yesterday", invertedIndexOffset);
    trie1->addKeyword("once", invertedIndexOffset);
    trie1->addKeyword("more", invertedIndexOffset);

    trie1->addKeyword("jimi", invertedIndexOffset);
    trie1->addKeyword("hendrix", invertedIndexOffset);
    trie1->addKeyword("little", invertedIndexOffset);
    trie1->addKeyword("wing", invertedIndexOffset);
    trie1->commit();
    trie1->finalCommit(NULL);
    trie1->print_Trie();

    Trie::save(*trie1,filenameTrie);
    delete trie1;

    Trie *trie2 = new Trie();
    Trie::load(*trie2,filenameTrie);

    cout<<"\nBefore Commit:" << std::endl;
    trie2->addKeyword("steve", invertedIndexOffset);
    trie2->addKeyword("jobs", invertedIndexOffset);
    trie2->addKeyword("stanford", invertedIndexOffset);
    trie2->addKeyword("speech", invertedIndexOffset);

    cout<<"\nAfter Commit and Update:\n" << std::endl;

    trie2->print_Trie();

    delete trie2;

}

void test5()
{
    Trie *trie1 = new Trie();

    string keyword1, keyword2, keyword3, keyword4, keyword5;
    keyword1.assign("cancer");
    keyword2.assign("canada");
    keyword3.assign("canteen");
    keyword4.assign("can");
    keyword5.assign("cat");

    unsigned keywordId1,keywordId2,keywordId3,keywordId4,keywordId5,keywordId6,keywordId7,keywordId8;

    //TODO take care of upper case
    unsigned invertedIndexOffset;
    keywordId1 = trie1->addKeyword("cancer", invertedIndexOffset);
    keywordId2 = trie1->addKeyword("canada", invertedIndexOffset);
    keywordId3 = trie1->addKeyword("canteen", invertedIndexOffset);
    keywordId4 = trie1->addKeyword("cancer", invertedIndexOffset);
    keywordId5 = trie1->addKeyword("can", invertedIndexOffset);
    keywordId6 = trie1->addKeyword("cat", invertedIndexOffset);
    keywordId7 = trie1->addKeyword("and", invertedIndexOffset);
    keywordId8 = trie1->addKeyword("cans", invertedIndexOffset);

    trie1->commit();
    trie1->finalCommit(NULL);
    trie1->print_Trie();

    /*
     * The final trie should look like the following:
     *
     *    root
     *      |
     *      |       (32,112) (32,112)
     *      ----------- c ----- a  ----- n (32)(32,96)
     *      |                   |        |
     *      |                   |        |
     *      a(16,16)            t        ---- a ----- d ----- a(48)(48,48)
     *      |            (112,112)(112)  |  (48,48) (48,48)
     *      |                            |
     *      n(16,16)                     ---- c ----- e ----- r(64)(64,64)
     *      |                            |   (64,64) (64,64)
     *      |                            |
     *      d(16,16)(16)                  ---- s(80)(80,80)
     *                                   |
     *                                   |
     *                                   ---- t ----- e ----- e ----- n(96)(96,96)
     *                                     (96,96)  (96,96) (96,96)
     */

    vector<Prefix> ancestorPrefixes;

    typedef boost::shared_ptr<TrieRootNodeAndFreeList > TrieRootNodeSharedPtr;
    TrieRootNodeSharedPtr rootSharedPtr1;
    trie1->getTrieRootNode_ReadView(rootSharedPtr1);
    TrieNode *root1 = rootSharedPtr1->root;

    Prefix p1 = Prefix(trie1->getTrieNodeFromUtf8String( root1, "and")->getMinId(), trie1->getTrieNodeFromUtf8String( root1, "and")->getMaxId());
    trie1->getAncestorPrefixes(p1, ancestorPrefixes);
    assert( ancestorPrefixes.size() == 0 );

    Prefix p2 = Prefix(trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMinId(), trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMaxId());
    trie1->getAncestorPrefixes(p2, ancestorPrefixes);
    assert( ancestorPrefixes.size() == 2 );
    assert( ancestorPrefixes[0].minId == trie1->getTrieNodeFromUtf8String( root1, "ca")->getMinId());
    assert( ancestorPrefixes[0].maxId == trie1->getTrieNodeFromUtf8String( root1, "ca")->getMaxId());
    assert( ancestorPrefixes[1].minId == trie1->getTrieNodeFromUtf8String( root1, "can")->getMinId());
    assert( ancestorPrefixes[1].maxId == trie1->getTrieNodeFromUtf8String( root1, "can")->getMaxId());
    ancestorPrefixes.clear();

    Prefix p3 = Prefix(trie1->getTrieNodeFromUtf8String( root1, "cans")->getMinId(), trie1->getTrieNodeFromUtf8String( root1, "cans")->getMaxId());
    trie1->getAncestorPrefixes(p3, ancestorPrefixes);
    assert( ancestorPrefixes.size() == 2 );
    assert( ancestorPrefixes[0].minId == trie1->getTrieNodeFromUtf8String( root1, "ca")->getMinId());
    assert( ancestorPrefixes[0].maxId == trie1->getTrieNodeFromUtf8String( root1, "ca")->getMaxId());
    assert( ancestorPrefixes[1].minId == trie1->getTrieNodeFromUtf8String( root1, "can")->getMinId());
    assert( ancestorPrefixes[1].maxId == trie1->getTrieNodeFromUtf8String( root1, "can")->getMaxId());

    Prefix p_cancer;
    trie1->getPrefixFromKeywordId(trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMinId(), p_cancer);// get prefix for cancer
    ASSERT( p_cancer.minId == trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMinId());
    ASSERT( p_cancer.maxId == trie1->getTrieNodeFromUtf8String( root1, "cancer")->getMaxId());

    Prefix p_can;
    trie1->getPrefixFromKeywordId(trie1->getTrieNodeFromUtf8String( root1, "can")->getMinId(), p_can);// get prefix for can
    ASSERT( p_can.minId == trie1->getTrieNodeFromUtf8String( root1, "can")->getMinId());
    ASSERT( p_can.maxId == trie1->getTrieNodeFromUtf8String( root1, "can")->getMaxId());

    Prefix p_and;
    trie1->getPrefixFromKeywordId(trie1->getTrieNodeFromUtf8String( root1, "and")->getMinId(), p_and);// get prefix for and
    ASSERT( p_and.minId == trie1->getTrieNodeFromUtf8String( root1, "and")->getMinId());
    ASSERT( p_and.maxId == trie1->getTrieNodeFromUtf8String( root1, "and")->getMaxId());

    delete trie1;
}

int main(int argc, char *argv[]) {

    bool verbose = false;
    if ( argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = true;
    }
//    Logger::setLogLevel(Logger::SRCH2_LOG_DEBUG);
    //TODO logic tests
    test1();
    cout << "test1 done" << endl;

    // commit and update test
    test2();
    cout << "test2 done" << endl;

    test2_ThreadSafe();
    cout << "test2_ThreadSafe done" << endl;

    // commit and update test mimics IndexUpdater_Test
    test3();

    // commit and update test mimics IndexUpdater_Test with deserialisation
    test4();

    cout << "test5" << endl;
    // test the function getAncestorPrefixes()
    test5();

    cout << "\nTrie Unit Tests: Passed\n";

    return 0;
}
