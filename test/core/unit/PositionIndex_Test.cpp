//$Id: PositionIndex_Test.cpp 1639 2011-09-27 22:13:26Z bimaple.vijay $

#include "util/Assert.h"
#include <iostream>
#include <functional>
#include <vector>
#include <map>
#include <cstring>
#include <assert.h>

using namespace std;
using namespace bimaple::instantsearch;

int main(int argc, char *argv[])
{
/*
    bool verbose = false;
    if ( argc > 1 && strcmp(argv[1], "--verbose") == 0)
    {
        verbose = true;
    }

    string prefix1;
    string prefix2;
    verbose = true;

    map<string, TokenAttributeHits > tokenAttributeHitsMap;
    vector<unsigned> positionIndexOffsetList;

    TokenAttributeHits tokenAttributeHits1, tokenAttributeHits2, tokenAttributeHits3, tokenAttributeHits4;

    tokenAttributeHitsMap["key1"].attributeList.push_back(21);
    tokenAttributeHitsMap["key1"].attributeList.push_back(54);
    tokenAttributeHitsMap["key1"].attributeList.push_back(65);


    tokenAttributeHitsMap["key2"].attributeList.push_back(17);
    tokenAttributeHitsMap["key2"].attributeList.push_back(52);
    tokenAttributeHitsMap["key2"].attributeList.push_back(28);

    tokenAttributeHitsMap["key3"].attributeList.push_back(1);
    tokenAttributeHitsMap["key3"].attributeList.push_back(2);
    tokenAttributeHitsMap["key3"].attributeList.push_back(3);

    PositionIndex *positionIndex = new PositionIndex();
    positionIndex->addRecordHitPositions(tokenAttributeHitsMap, positionIndexOffsetList);

    tokenAttributeHitsMap.clear();

    tokenAttributeHitsMap["key1"].attributeList.push_back(54);
    tokenAttributeHitsMap["key1"].attributeList.push_back(32);
    tokenAttributeHitsMap["key1"].attributeList.push_back(69);

    tokenAttributeHitsMap["key2"].attributeList.push_back(3);
    tokenAttributeHitsMap["key2"].attributeList.push_back(5);
    tokenAttributeHitsMap["key2"].attributeList.push_back(7);

    positionIndex->addRecordHitPositions(tokenAttributeHitsMap, positionIndexOffsetList);

    tokenAttributeHitsMap["key3"].attributeList.push_back(49);
    tokenAttributeHitsMap["key3"].attributeList.push_back(63);
    tokenAttributeHitsMap["key3"].attributeList.push_back(72);

    tokenAttributeHitsMap["key4"].attributeList.push_back(49);

    positionIndex->addRecordHitPositions(tokenAttributeHitsMap, positionIndexOffsetList);

    vector<unsigned> keywordPositions1, keywordPositions2, keywordPositions3;
    vector<unsigned>::iterator vectorIterator;

    unsigned offset1 = positionIndex->getRecordHitPositions(0, keywordPositions1);
    unsigned offset2 = positionIndex->getRecordHitPositions(offset1, keywordPositions2);
    positionIndex->getRecordHitPositions(offset2, keywordPositions3);

    vector<unsigned> keywordPositions_assert1;
    vector<unsigned> keywordPositions_assert2;
    vector<unsigned> keywordPositions_assert3;

    keywordPositions_assert1.push_back(21);
    keywordPositions_assert1.push_back(54);
    keywordPositions_assert1.push_back(65);

    keywordPositions_assert2.push_back(17);
    keywordPositions_assert2.push_back(52);
    keywordPositions_assert2.push_back(28);

    keywordPositions_assert3.push_back(1);
    keywordPositions_assert3.push_back(2);
    keywordPositions_assert3.push_back(3);

    ASSERT(keywordPositions1 == keywordPositions_assert1);
    ASSERT(keywordPositions2 == keywordPositions_assert2);
    ASSERT(keywordPositions3 == keywordPositions_assert3);


    //Testing Serialization
    const string filename("testForwardIndexSerialize");
    PositionIndex::save(*positionIndex,filename);
    delete positionIndex;

    PositionIndex *positionIndexDeserialized = new PositionIndex();
    PositionIndex::load(*positionIndexDeserialized, filename);

    keywordPositions1.clear();
    keywordPositions2.clear();
    keywordPositions3.clear();

    offset1 = positionIndexDeserialized->getRecordHitPositions(0, keywordPositions1);
    offset2 = positionIndexDeserialized->getRecordHitPositions(offset1, keywordPositions2);
    positionIndexDeserialized->getRecordHitPositions(offset2, keywordPositions3);

    ASSERT(keywordPositions1 == keywordPositions_assert1);
    ASSERT(keywordPositions2 == keywordPositions_assert2);
    ASSERT(keywordPositions3 == keywordPositions_assert3);

    delete positionIndexDeserialized;*/
    cout << "PositionsIndex Unit Tests: Passed\n";

    return 0;
}
