// $Id: StringEncoding_Test.cpp 3273 2013-04-19 13:59:25Z jiaying $

#include <instantsearch/Analyzer.h>
#include "operation/IndexerInternal.h"
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>
#include "util/Assert.h"
#include "IntegrationTestHelper.h"
#include <stdlib.h>

#include <time.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

string INDEX_DIR = getenv("small_index_dir");

void addSimpleChineseRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites,
			updateHistogramEveryPMerges, updateHistogramEveryQWrites,
			INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_authors",
			"Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure simple chinese characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_authors", "孔子");
	record->setSearchableAttributeValue("article_sentence",
			"学而时习之，不亦悦乎？有朋自远方来，不亦乐乎？人不知而不愠，不亦君子乎？");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "李白");
	record->setSearchableAttributeValue(2,
			"故人西辞黄鹤楼， 烟花三月下扬州。 孤帆远影碧空尽， 唯见长江天际流。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "孟子");
	record->setSearchableAttributeValue(2, "老吾老，以及人之老；幼吾幼，以及人之幼。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1104);
	record->setSearchableAttributeValue(1, "杜甫");
	record->setSearchableAttributeValue(2,
			"风急天高猿啸哀， 渚清沙白鸟飞回。 无边落木萧萧下， 不尽长江滚滚来。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1105);
	record->setSearchableAttributeValue(1, "老子");
	record->setSearchableAttributeValue(2, "道可道，非常道。名可名，非常名。无名天地之始；有名万物之母。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1106);
	record->setSearchableAttributeValue(1, "白居易");
	record->setSearchableAttributeValue(2, "离离原上草，一岁一枯荣。野火烧不尽，春风吹又生。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1107);
	record->setSearchableAttributeValue(1, "鲁迅");
	record->setSearchableAttributeValue(2, "横眉冷对千夫指，俯首甘为孺子牛。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1108);
	record->setSearchableAttributeValue(1, "孔子");
	record->setSearchableAttributeValue(2, "登东山而小鲁，登泰山而小天下。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1109);
	record->setSearchableAttributeValue(1, "孟子");
	record->setSearchableAttributeValue(2, "生，亦我所欲也，义，亦我所欲也，二者不可得兼，舍生而取义者也。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1110);
	record->setSearchableAttributeValue(1, "辛弃疾");
	record->setSearchableAttributeValue(2,
			"醉里挑灯看剑，梦会吹角连营。八百里分麾下炙，五十弦翻塞外声。沙场秋点兵。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple chinese and english
	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_authors", "Mr孔子");
	record->setSearchableAttributeValue("article_sentence",
			"学而时习之，不亦happy乎？有friend自远方来，不亦happy乎？人不知而不愠，不亦君子乎？");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//Test a record that includes both Chinese and English
	record->clear();
	record->setPrimaryKey(1202);
	record->setSearchableAttributeValue(1, "Miss 李清照");
	record->setSearchableAttributeValue(2, "生当作people杰，死亦为ghost雄。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1203);
	record->setSearchableAttributeValue(1, "");
	record->setSearchableAttributeValue(2, "生当作people杰，死亦为ghost雄。--Miss 李清照");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1204);
	record->setSearchableAttributeValue(1, "");
	record->setSearchableAttributeValue(2, "𡑞 will fail for utf16, but work for utf32");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test simple chinese
void testSimpleChinese() {
	addSimpleChineseRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);


	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();
	//We use popular Chinese poems and their authors to test if the engine can support Chinese characters. The data was obtained from www.baidu.com search "中国 诗词名句"
	//search Chinese text on the first attribute
	//Query: "孔子", hits -> 1101, 1108， 1201
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1108);
		recordIds.push_back(1201);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "孔子", 3, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "孔子", 3, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "孔子", 3, recordIds)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "孔子", 3, recordIds)
						== true);
	}

	//search Chinese text on the second attribute
	//Query: "长江", hits -> 1102, 1104
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1104);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "长江", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "长江", 2, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "长江", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "长江", 2, recordIds)
						== true);
	}

	//search English text
	//Query: "Mr", hits -> 1003, 1201
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1003);
		recordIds.push_back(1201);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "Mr", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "Mr", 2, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "Mr", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "Mr", 2, recordIds)
						== true);
	}

	//Test a record that includes both Chinese and English
	//Query: "Miss 李清照", hits -> 1003, 1201
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "Miss 李清照", 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "Miss 李清照", 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "Miss 李清照", 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "Miss 李清照", 2,
						recordIds) == true);
	}

	//search long English and Chinese text
	//Query: "Miss 李清照", hits -> 1003, 1201
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator,
						"Miss 李清照 生当作people杰，死亦为ghost雄。", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator,
						"Miss 李清照 生当作people杰，死亦为ghost雄。", 2, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator,
						"Miss 李清照 生当作people杰，死亦为ghost雄。", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator,
						"Miss 李清照 生当作people杰，死亦为ghost雄。", 2, recordIds)
						== true);
	}

	//search long English and Chinese text fuzzy test
	//Query: "Miss 李清照", hits -> 1202, 1203
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator,
						"Miss 李清照 生当作peple杰，死亦为ghost雄。", 2, recordIds)
						== false);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator,
						"Miss 李清照 生当作peple杰，死亦为ghost雄。", 2, recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator,
						"Miss 李清照 生当作peple杰，死亦为ghost雄。", 2, recordIds)
						== false);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator,
						"Miss 李清照 生当作peple杰，死亦为ghost雄。", 2, recordIds) == true);
	}

	//Update Index
	Record *record = new Record(index->getSchema());
	record->setPrimaryKey(1999);
	record->setSearchableAttributeValue(1, "中国 孔夫子");
	record->setSearchableAttributeValue(2, "登东山而小鲁，登泰山而小天下。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1998);
	record->setSearchableAttributeValue(1, "孔子");
	record->setSearchableAttributeValue(2, "登西山而小鲁，登泰山而小天下。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	sleep(mergeEveryNSeconds + 1);

	delete queryEvaluator;
	queryEvaluator = new QueryEvaluator(index, &runtimeParameters);

	//Query: "孔子", hits -> 1101, 1108, 1201，1998, 1999
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1108);
		recordIds.push_back(1201);
		recordIds.push_back(1999);
		recordIds.push_back(1998);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "孔子", 5, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "孔子", 5, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "孔子", 5, recordIds)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "孔子", 5, recordIds)
						== true);
	}

	//Query: "东山", hits -> 1108, 1999
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1108);
		recordIds.push_back(1999);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "东山", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "东山", 2, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "东山", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "东山", 2, recordIds)
						== true);
	}

	index->commit();
	index->save(INDEX_DIR);

	//Query: "孔子", hits -> 1101, 1108, 1201, 1998, 1999
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1108);
		recordIds.push_back(1201);
		recordIds.push_back(1999);
		recordIds.push_back(1998);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "孔子", 5, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "孔子", 5, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "孔子", 5, recordIds)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "孔子", 5, recordIds)
						== true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

void addSimpleZhuyinRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_authors", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_authors",
			"Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// we translate the Zhuyin from Chinese Pinyin by http://www.ifreesite.com/phonetic/
	// pure simple zhuyin characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_authors", "ㄎㄨㄥ ㄗ");
	record->setSearchableAttributeValue("article_sentence",
			"ㄒㄩㄝ ㄦ ㄕ ㄒㄧ ㄓ ㄅㄨ ㄧ ㄩㄝ ㄏㄨ ㄧㄡ ㄆㄥ ㄗ ㄩㄢ ㄈㄤ ㄌㄞ ㄅㄨ ㄧ ㄌㄜ ㄏㄨ ㄖㄣ ㄅㄨ ㄓ ㄦ ㄅㄨ ㄅㄨ ㄧ ㄐㄩㄣ ㄗ ㄏㄨ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "ㄌㄧ ㄅㄞ");
	record->setSearchableAttributeValue(2,
			"ㄍㄨ ㄖㄣ ㄒㄧ ㄘ ㄏㄨㄤ ㄏㄜ ㄌㄡ ㄧㄢ ㄏㄨㄚ ㄙㄢ ㄩㄝ ㄒㄧㄚ ㄧㄤ ㄓㄡ ㄍㄨ ㄈㄢ ㄩㄢ ㄧㄥ ㄅㄧ ㄎㄨㄥ ㄐㄧㄣ ㄨㄟ ㄐㄧㄢ ㄔㄤ ㄐㄧㄤ ㄊㄧㄢ ㄐㄧ ㄌㄧㄡ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "ㄇㄥ ㄗ");
	record->setSearchableAttributeValue(2,
			"ㄌㄠ ㄨ ㄌㄠ ㄧ ㄐㄧ ㄖㄣ ㄓ ㄌㄠ ㄧㄡ ㄨ ㄧㄡ ㄧ ㄐㄧ ㄖㄣ ㄓ ㄧㄡ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1104);
	record->setSearchableAttributeValue(1, "ㄉㄨ ㄈㄨ");
	record->setSearchableAttributeValue(2,
			"ㄈㄥ ㄐㄧ ㄊㄧㄢ ㄍㄠ ㄩㄢ ㄒㄧㄠ ㄞ ㄑㄧㄥ ㄕㄚ ㄅㄞ ㄋㄧㄠ ㄈㄟ ㄏㄨㄟ ㄨ ㄅㄧㄢ ㄌㄨㄛ ㄇㄨ ㄒㄧㄠ ㄒㄧㄠ ㄒㄧㄚ ㄅㄨ ㄐㄧㄣ ㄔㄤ ㄐㄧㄤ ㄍㄨㄣ ㄍㄨㄣ ㄌㄞ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1105);
	record->setSearchableAttributeValue(1, "ㄌㄠ ㄗ");
	record->setSearchableAttributeValue(2,
			"ㄉㄠ ㄎㄜ ㄉㄠ ㄈㄟ ㄔㄤ ㄉㄠ ㄇㄧㄥ ㄎㄜ ㄇㄧㄥ ㄈㄟ ㄔㄤ ㄇㄧㄥ ㄨ ㄇㄧㄥ ㄊㄧㄢ ㄉㄧ ㄓ ㄕ ㄧㄡ ㄇㄧㄥ ㄨㄢ ㄨ ㄓ ㄇㄨ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1106);
	record->setSearchableAttributeValue(1, "ㄅㄞ ㄐㄩ ㄧ");
	record->setSearchableAttributeValue(2,
			"ㄌㄧ ㄌㄧ ㄩㄢ ㄕㄤ ㄘㄠ ㄧ ㄙㄨㄟ ㄧ ㄎㄨ ㄖㄨㄥ ㄧㄝ ㄏㄨㄛ ㄕㄠ ㄅㄨ ㄐㄧㄣ ㄔㄨㄣ ㄈㄥ ㄔㄨㄟ ㄧㄡ ㄕㄥ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1107);
	record->setSearchableAttributeValue(1, "ㄌㄨ ㄒㄩㄣ");
	record->setSearchableAttributeValue(2,
			"ㄏㄥ ㄇㄟ ㄌㄥ ㄉㄨㄟ ㄑㄧㄢ ㄈㄨ ㄓ ㄈㄨ ㄕㄡ ㄍㄢ ㄨㄟ ㄖㄨ ㄗ ㄋㄧㄡ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1108);
	record->setSearchableAttributeValue(1, "ㄎㄨㄥ ㄗ");
	record->setSearchableAttributeValue(2,
			"ㄉㄥ ㄉㄨㄥ ㄕㄢ ㄦ ㄒㄧㄠ ㄌㄨ ㄉㄥ ㄊㄞ ㄕㄢ ㄦ ㄒㄧㄠ ㄊㄧㄢ ㄒㄧㄚ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1109);
	record->setSearchableAttributeValue(1, "ㄇㄥ ㄗ");
	record->setSearchableAttributeValue(2,
			"ㄕㄥ ㄧ ㄨㄛ ㄙㄨㄛ ㄩ ㄧㄝ ㄧ ㄧ ㄨㄛ ㄙㄨㄛ ㄩ ㄧㄝ ㄦ ㄓㄜ ㄅㄨ ㄎㄜ ㄉㄜ ㄐㄧㄢ ㄕㄜ ㄕㄥ ㄦ ㄑㄩ ㄧ ㄓㄜ ㄧㄝ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1110);
	record->setSearchableAttributeValue(1, "ㄒㄧㄣ ㄑㄧ ㄐㄧ");
	record->setSearchableAttributeValue(2,
			"ㄗㄨㄟ ㄌㄧ ㄊㄧㄠ ㄉㄥ ㄎㄢ ㄐㄧㄢ ㄇㄥ ㄏㄨㄟ ㄔㄨㄟ ㄐㄧㄠ ㄌㄧㄢ ㄧㄥ ㄅㄚ ㄅㄞ ㄌㄧ ㄈㄣ ㄒㄧㄚ ㄓ ㄨ ㄕ ㄒㄧㄢ ㄈㄢ ㄙㄞ ㄨㄞ ㄕㄥ ㄕㄚ ㄔㄤ ㄑㄧㄡ ㄉㄧㄢ ㄅㄧㄥ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Zhuyin and english
	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_authors", "Mrㄎㄨㄥ ㄗ");
	record->setSearchableAttributeValue("article_sentence",
			"ㄒㄩㄝ ㄦ ㄕ ㄒㄧ ㄓ ㄅㄨ ㄧ happy ㄏㄨ ㄧㄡ friend ㄗ ㄩㄢ ㄈㄤ ㄌㄞ ㄅㄨ ㄧ happy ㄏㄨ ㄖㄣ ㄅㄨ ㄓ ㄦ ㄅㄨ ㄅㄨ ㄧ ㄐㄩㄣ ㄗ ㄏㄨ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//Test the case where the data includes both Zhuyin and English
	record->clear();
	record->setPrimaryKey(1202);
	record->setSearchableAttributeValue(1, "Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ");
	record->setSearchableAttributeValue(2,
			"ㄕㄥ ㄉㄤ ㄗㄨㄛ people ㄐㄧㄝ ㄙ ㄧ ㄨㄟ ghost ㄒㄩㄥ。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1203);
	record->setSearchableAttributeValue(1, "");
	record->setSearchableAttributeValue(2,
			"ㄕㄥ ㄉㄤ ㄗㄨㄛ people ㄐㄧㄝ ㄙ ㄧ ㄨㄟ ghost ㄒㄩㄥ。--Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test simple Zhuyin
void testSimpleZhuyin() {
	addSimpleZhuyinRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//search Zhuyin text on the first attribute
	//Query: "ㄎㄨㄥ ㄗ", hits -> 1101,  1108， 1201
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1108);
		recordIds.push_back(1201);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 3, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 3, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 3,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 3,
						recordIds) == true);
	}

	//search Zhuyin text on the second attribute
	//Query: "ㄔㄤ ㄐㄧㄤ", hits -> 1102, 1104
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1104);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "ㄔㄤ ㄐㄧㄤ", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "ㄔㄤ ㄐㄧㄤ", 2, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "ㄔㄤ ㄐㄧㄤ", 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "ㄔㄤ ㄐㄧㄤ", 2,
						recordIds) == true);
	}

	//search English text
	//Query: "Mr", hits -> 1003, 1201
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1003);
		recordIds.push_back(1201);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "Mr", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "Mr", 2, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "Mr", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "Mr", 2, recordIds)
						== true);
	}

	//search English and Zhuyin text
	//Query: "Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ", hits -> 1003, 1201
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ", 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ", 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ", 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ", 2,
						recordIds) == true);
	}

	//search long English and Zhuyin text
	//Query: "Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ", hits -> 1003, 1201
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator,
						"Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ ㄕㄥ ㄉㄤ ㄗㄨㄛ people ㄐㄧㄝ ㄙ ㄧ ㄨㄟ ghost ㄒㄩㄥ",
						2, recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator,
						"Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ ㄕㄥ ㄉㄤ ㄗㄨㄛ people ㄐㄧㄝ ㄙ ㄧ ㄨㄟ ghost ㄒㄩㄥ",
						2, recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator,
						"Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ ㄕㄥ ㄉㄤ ㄗㄨㄛ people ㄐㄧㄝ ㄙ ㄧ ㄨㄟ ghost ㄒㄩㄥ",
						2, recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator,
						"Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ ㄕㄥ ㄉㄤ ㄗㄨㄛ people ㄐㄧㄝ ㄙ ㄧ ㄨㄟ ghost ㄒㄩㄥ",
						2, recordIds) == true);
	}

	//search long English and Chinese text fuzzy test
	//Query: "Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ", hits -> 1202, 1203
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator,
						"Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ ㄕㄥ ㄉㄤ ㄗㄨㄛ peple ㄐㄧㄝ ㄙ ㄧ ㄨㄟ ghost ㄒㄩㄥ",
						2, recordIds) == false);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator,
						"Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ ㄕㄥ ㄉㄤ ㄗㄨㄛ peple ㄐㄧㄝ ㄙ ㄧ ㄨㄟ ghost ㄒㄩㄥ",
						2, recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator,
						"Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ ㄕㄥ ㄉㄤ ㄗㄨㄛ peple ㄐㄧㄝ ㄙ ㄧ ㄨㄟ ghost ㄒㄩㄥ",
						2, recordIds) == false);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator,
						"Miss ㄌㄧ ㄑㄧㄥ ㄓㄠ ㄕㄥ ㄉㄤ ㄗㄨㄛ peple ㄐㄧㄝ ㄙ ㄧ ㄨㄟ ghost ㄒㄩㄥ",
						2, recordIds) == true);
	}

	//Update Index
	Record *record = new Record(index->getSchema());
	record->setPrimaryKey(1999);
	record->setSearchableAttributeValue(1, "ㄓㄨㄥ ㄍㄨㄛ ㄎㄨㄥ ㄈㄨ ㄗ");
	record->setSearchableAttributeValue(2,
			"ㄉㄥ ㄉㄨㄥ ㄕㄢ ㄦ ㄒㄧㄠ ㄌㄨ ㄉㄥ ㄊㄞ ㄕㄢ ㄦ ㄒㄧㄠ ㄊㄧㄢ ㄒㄧㄚ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1998);
	record->setSearchableAttributeValue(1, "ㄎㄨㄥ ㄗ");
	record->setSearchableAttributeValue(2,
			"ㄉㄥ ㄒㄧ ㄕㄢ ㄦ ㄒㄧㄠ ㄌㄨ ㄉㄥ ㄊㄞ ㄕㄢ ㄦ ㄒㄧㄠ ㄊㄧㄢ ㄒㄧㄚ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	sleep(mergeEveryNSeconds + 1);

	delete queryEvaluator;
	queryEvaluator = new QueryEvaluator(index, &runtimeParameters);

	//Query: "ㄎㄨㄥ ㄗ", hits -> 1101,  1108,  1201, 1999, 1998
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1108);
		recordIds.push_back(1201);
		recordIds.push_back(1999);
		recordIds.push_back(1998);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 5, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 5, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 5,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 5,
						recordIds) == true);
	}

	//Query: "ㄉㄨㄥ ㄕㄢ", hits ->  1108,  1999
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1108);
		recordIds.push_back(1999);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "ㄉㄨㄥ ㄕㄢ", 2, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "ㄉㄨㄥ ㄕㄢ", 2, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "ㄉㄨㄥ ㄕㄢ", 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "ㄉㄨㄥ ㄕㄢ", 2,
						recordIds) == true);
	}

	index->commit();
	index->save(INDEX_DIR);

	//Query: "ㄎㄨㄥ ㄗ", hits -> 1101,  1108,  1201, 1999, 1998
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1108);
		recordIds.push_back(1201);
		recordIds.push_back(1999);
		recordIds.push_back(1998);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 5, recordIds)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 5, recordIds)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 5,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "ㄎㄨㄥ ㄗ", 5,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addJapaneseRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure Japanese characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "彼の自然な生命の任期");
	record->setSearchableAttributeValue("article_sentence",
			"私の親愛なるチャールズ、私は単に政治的、文学的な生命のあなたの19年ので、あなたにこの作品を捧げ休暇を取る");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "マイ·ライフの日々");
	record->setSearchableAttributeValue(2,
			"ヘンリーライダーハガードは1856年6月22日に生まれ、そして1925年5月14日に死亡した");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "闇と光");
	record->setSearchableAttributeValue(2,
			"私の以前の本のレビュアーは、そのような本がこれまでに書かれているべきである理由を理解することは困難であったと述べた。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1104);
	record->setSearchableAttributeValue(1, "自伝");
	record->setSearchableAttributeValue(2,
			"それは私がこの本に短い序文を置くべきであることを十分にかもしれない。1878年の夏に私の父は彼が彼自身の人生の回顧録を書いていたことを教えてくれました。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1105);
	record->setSearchableAttributeValue(1, "アリス·B·トクラスの自伝");
	record->setSearchableAttributeValue(2,
			"私は、サンフランシスコ、カリフォルニア州で生まれた。私は温暖な気候で、常に結果に優先生活を持っているが、それは困難であり、");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1106);
	record->setSearchableAttributeValue(1, "クリントン自伝：マイ·ライフ");
	record->setSearchableAttributeValue(2,
			"愛と虐待、尊敬と憤り、ビル·クリントン、ビル·クリントンは、現代的な論争の政治的指導者である。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Japanese and english
	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "bookマイ·ライフ");
	record->setSearchableAttributeValue("article_sentence",
			"JOHNウルマンは、1720 yearにノーサンプトン、ニュージャージー州で生まれ、ヨーク、イギリス、でdieした。 ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//Test a record that includes both Japanese and English
	record->clear();
	record->setPrimaryKey(1202);
	record->setSearchableAttributeValue(1, "Miss North回帰線get");
	record->setSearchableAttributeValue(2,
			"北回帰線は、最初のパリ、フランスのオベリスクプレスによって1934 yearに出版ヘンリー·ミラーの小説です。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1203);
	record->setSearchableAttributeValue(1, "");
	record->setSearchableAttributeValue(2,
			"ガラスの城'ジャネット壁yearンスのオベリスクプレで2005回顧録である。this は、彼女を詳述し、彼女の兄弟'型破り");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Japanese
void testJapanese() {
	addJapaneseRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();
	//We use popular English novel and translate into Japanese,using the content and their titles to test if the engine can support Chinese characters. The data was obtained from www.baidu.com search "中国 诗词名句"
	//search Japanese text on the first attribute
	//Query: "ラ", hits -> 1102,1105,1106 1201,1202,1203
	{
		string query = "ラ";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1105);
		recordIds.push_back(1106);
		recordIds.push_back(1201);
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 6,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 6,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 6,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 6,
						recordIds) == true);
	}

	//search Japanese text on the second attribute
	//Query: "ル", hits -> 1101, 1105,1106,1201
	{
		string query = "ル";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1105);
		recordIds.push_back(1106);
		recordIds.push_back(1201);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 4,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 4,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 4,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 4,
						recordIds) == true);

	}

	//search English text
	//Query: "book", hits -> 1001, 1201
	{
		string query = "book";

		vector<unsigned> recordIds;
		recordIds.push_back(1001);
		recordIds.push_back(1201);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	//Test a record that includes both Japanese and English
	//Query: "yearプ", hits -> 1201, 1202,1203
	{
		string query = "yearプ";
		vector<unsigned> recordIds;
		recordIds.push_back(1201);
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
	}
	//search long English and Japanese text
	//Query: "yearンスのオベリスクプレ", hits -> 1202, 1203
	{
		string query = "yearンスのオベリスクプレ";
		vector<unsigned> recordIds;
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	//Update Index
	Record *record = new Record(index->getSchema());
	record->setPrimaryKey(1999);
	record->setSearchableAttributeValue(1, "マルコムリトル自伝、マルコムXの自伝");
	record->setSearchableAttributeValue(2,
			"マルコムXの自伝は、マルコムXとジャーナリストのアレックス·ヘイリーの間のコラボレーションの結果であった");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1998);
	record->setSearchableAttributeValue(1, "マイ·ライフ·マイ·ライフのストーリー：ヘレン·ケラーの自伝");
	record->setSearchableAttributeValue(2,
			"年に出版さマイ·ライフの物語では、アン·サリバンと、特に彼女の初期の人生、彼女の経験を詳述したヘレン·ケラーの自伝です。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	sleep(mergeEveryNSeconds + 1);

	delete queryEvaluator;
	queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	index->commit();
	index->save(INDEX_DIR);

	//Query: "ル", hits -> 1101, 1105,1106,1201,1999
	{
		string query = "ル";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1105);
		recordIds.push_back(1106);
		recordIds.push_back(1201);
		recordIds.push_back(1999);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 5,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 5,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 5,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 5,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addFrenchRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure French characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title",
			"Frankenstein Frankenstein");
	record->setSearchableAttributeValue("article_sentence",
			"Frankenstein est une violation de la vie de la nature book produit de la loi continué, les scientifiques Frankenstein remplacer le rôle du créateur dans une tentative de créer la vie dans leurs propres mains");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Flatland 2D-Unis");
	record->setSearchableAttributeValue(2,
			"En 1884, un Mingjiao Yi Butler fondée (EdwinA.Abbott 1838  1926), le ministre britannique a écrit un pamphlet intitulé");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Le jour du fléau");
	record->setSearchableAttributeValue(2,
			"le livre suit un jeune homme nommé Tod Hackett écrit qui se considère comme un peintre et artiste");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1104);
	record->setSearchableAttributeValue(1, "la Défense");
	record->setSearchableAttributeValue(2,
			"Dynamite avocat de la défense Joseph Antonelli n'a jamais perdu un procès - ou ressenti l'aiguillon de la conscience pour laisser la route libre coupable");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1105);
	record->setSearchableAttributeValue(1, "Deathworld");
	record->setSearchableAttributeValue(2,
			"Deathworld est un personnage fictif de la série Discworld A la fin du monde film de Terry Pratchett et une parodie de plusieurs autres personnifications de la mort.");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1106);
	record->setSearchableAttributeValue(1, "le Silmarillion");
	record->setSearchableAttributeValue(2,
			"Silmarillion, maintenant publié défense quatre ans après la mort de son auteur, est un récit des Jours Anciens,");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple French and english
	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"book écrit Je suis une légende");
	record->setSearchableAttributeValue("article_sentence",
			"A la fin du monde film de science-fiction sorti en 2007, réalisé par Francis Lawrence (Francis Lawrence) ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test French
void testFrench() {
	addFrenchRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//search French text on the first attribute
	//Query: "défense", hits -> 1104,1106
	{
		string query = "défense";
		vector<unsigned> recordIds;
		recordIds.push_back(1104);

		recordIds.push_back(1106);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	//search French text on the second attribute
	//Query: "écrit", hits -> 1102, 1103,1201
	{
		string query = "écrit";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1103);
		recordIds.push_back(1201);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 3,
						recordIds) == true);

	}

	//search English text
	//Query: "book", hits -> 1001, 1101,1201
	{
		string query = "book";

		vector<unsigned> recordIds;
		recordIds.push_back(1001);
		recordIds.push_back(1101);
		recordIds.push_back(1201);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
	}

	//Query: "book écrit ", hits -> 1201
	{
		string query = "book écrit ";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
	}
	//search long English and French text
	//Query: "A la fin du monde film", hits -> 1105, 1201
	{
		string query = "A la fin du monde film";
		vector<unsigned> recordIds;
		recordIds.push_back(1105);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	//Update Index
	Record *record = new Record(index->getSchema());
	record->setPrimaryKey(1999);
	record->setSearchableAttributeValue(1, "american Gods");
	record->setSearchableAttributeValue(2,
			"Une série d'aventures de l'ombre de héros à travers le territoire continental des États-Unis après avoir été libéré de prison, le processus de voyage");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1998);
	record->setSearchableAttributeValue(1, "La chasse Prey nano");
	record->setSearchableAttributeValue(2,
			"Rey est un roman de Michael Crichton fondée sur une menace nano-robotique pour genre humain");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	sleep(mergeEveryNSeconds + 1);

	delete queryEvaluator;
	queryEvaluator = new QueryEvaluator(index, &runtimeParameters);

	index->commit();
	index->save(INDEX_DIR);

	//Query: "fondée", hits -> 1102,1998
	{
		string query = "fondée";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1998);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addTranditionalChineseRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "關於自然宗教對話");
	record->setSearchableAttributeValue("article_sentence",
			"是休謨的一本重要哲學著作，代表他晚年較成熟的哲學思想");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "當天的蝗蟲");
	record->setSearchableAttributeValue(2, "本書遵循一個年輕的男子名為托德·哈克特認為自己作為一個畫家和藝術家");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Tra_Chinese and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "國防部");
	record->setSearchableAttributeValue(2,
			"炸藥的辯護律師約瑟夫·安東內利已經永遠lose了一個情況 - 或者一陣刺痛的良心讓有guilt走免費。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Silmarillion精靈寶鑽“");
	record->setSearchableAttributeValue("article_sentence",
			"長老天，或為世界first year齡的帳戶哲學。 ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Tranditional Chinese
void testTranditionalChinese() {
	addTranditionalChineseRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "學 ", hits ->1101  1201
	{
		string query = "學";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		//    recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	//Query: "year學 ", hits ->1201
	{
		string query = "year學 ";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addBulgarianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure Bulgarian characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Никога не ме пусна");
	record->setSearchableAttributeValue("article_sentence",
			"A сърцераздирателна любовна история на неспокоен, и очерта с изясняване на слабостта на човешката природа и надежда");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Виолетови Спилбърг са сини");
	record->setSearchableAttributeValue(2,
			"В този момент аз прекъсна сестра ми, както обикновено да се каже, може да има начин с думи, Шехерезада");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Bulgarian and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1,
			"The Man Спилбърг in the High Castle кървене");
	record->setSearchableAttributeValue(2,
			"Въздействие Харуки Мураками, Спилбърг всички, известна като Джин Йонг, но неговия живот се повтаря отхвърляне, обеднял");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Вагинално ");
	record->setSearchableAttributeValue("article_sentence",
			"Подобно на много от другите романи на Стивънсън кървене High обхваща историята, religion, антропологията,science, археологията  ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Bulgarian
void testBulgarian() {
	addBulgarianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "Спилбърг", hits ->  1102 1103 1201
	{
		string query = "Спилбърг";
		vector<unsigned> recordIds;

		recordIds.push_back(1102);
		recordIds.push_back(1103);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 3,
						recordIds) == true);

	}
	//Query: "кървене High", hits -> 1103 1201
	{
		string query = "кървене High ";
		vector<unsigned> recordIds;
		recordIds.push_back(1103);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addPinyinChineseRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_pinyin", 2); // searchable text
	schema->setSearchableAttribute("article_hanzi", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_pinyin",
			"shu yu na zhong bi jiao lao shi de dian ying  ");
	record->setSearchableAttributeValue("article_hanzi", "属于那种比较老式的电影");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1,
			"yi jiu si san novel nian de xi ban ya nei zhan yi cheng wei li shi chen ji");
	record->setSearchableAttributeValue(2, "一九三九年春的西班牙内战早已成为历史陈迹");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1,
			"yi ge bu ke neng  nong chong fu de gu shi");
	record->setSearchableAttributeValue(2, "一个不可能重复的故事");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1104);
	record->setSearchableAttributeValue(1,
			"shi zhong guo gu dian jun shi wen hua yi chan zhong de cui can gui bao");
	record->setSearchableAttributeValue(2, "是中国古典军事文化遗产中的璀璨瑰宝");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1105);
	record->setSearchableAttributeValue(1,
			"shi zhong guo you xiu wen hua chuang tong de zhong yao zu");
	record->setSearchableAttributeValue(2, "是中国优秀文化传统的重要组");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1106);
	record->setSearchableAttributeValue(1,
			"nong min chu shen people de du luo yi dan da wang wei");
	record->setSearchableAttributeValue(2, "农民出身的杜洛伊胆大妄为");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_pinyin",
			"shi yi bu yan shu de , feng ci xing ji qiang de xiao shuo novel");
	record->setSearchableAttributeValue("article_hanzi",
			"是一部严肃的、讽刺性极强的小说novel");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Pinyin Chinese
void testPinyinChinese() {
	addPinyinChineseRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "wen", hits -> 1104,1105
	{
		string query = "wen";
		vector<unsigned> recordIds;
		recordIds.push_back(1104);

		recordIds.push_back(1105);


		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	//Query: "nong ", hits ->  1103,1106
	{
		string query = "nong ";
		vector<unsigned> recordIds;

		recordIds.push_back(1103);
		recordIds.push_back(1106);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	//search English text
	//Query: "novel", hits -> 1102, 1201
	{
		string query = "novel";

		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	//Query: "wen hua ", hits -> 1104 1105
	{
		string query = "wen hua ";
		vector<unsigned> recordIds;

		recordIds.push_back(1104);
		recordIds.push_back(1105);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addZhuyinChineseRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_zhuyin", 2); // searchable text
	schema->setSearchableAttribute("article_hanzi", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	// pure French characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_zhuyin",
			"ㄕㄨ ㄩ ㄋㄚ ㄓㄨㄥ ㄅㄧ ㄐㄧㄠ ㄌㄠ ㄕ ㄉㄜ ㄉㄧㄢ ㄧㄥ  ");
	record->setSearchableAttributeValue("article_hanzi", "属于那种比较老式的电影");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1,
			"ㄧ ㄐㄧㄡ ㄙ ㄙㄢ NOVEL ㄋㄧㄢ ㄉㄜ ㄒㄧ ㄅㄢ ㄧㄚ ㄋㄟ ㄓㄢ ㄧ ㄔㄥ ㄨㄟ ㄌㄧ ㄕ ㄔㄣ ㄐㄧ");
	record->setSearchableAttributeValue(2, "一九三九年春的西班牙内战早已成为历史陈迹");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "ㄧ ㄍㄜ ㄅㄨ ㄎㄜ ㄋㄥ  ㄋㄨㄥ ㄔㄨㄥ ㄈㄨ ㄉㄜ ㄍㄨ ㄕ");
	record->setSearchableAttributeValue(2, "一个不可能重复的故事");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1104);
	record->setSearchableAttributeValue(1,
			"ㄕ ㄓㄨㄥ ㄍㄨㄛ ㄍㄨ ㄉㄧㄢ ㄐㄩㄣ ㄕ ㄨㄣ ㄏㄨㄚ ㄧ ㄔㄢ ㄓㄨㄥ ㄉㄜ ㄘㄨㄟ ㄘㄢ ㄍㄨㄟ ㄅㄠ");
	record->setSearchableAttributeValue(2, "是中国古典军事文化遗产中的璀璨瑰宝");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1105);
	record->setSearchableAttributeValue(1,
			"ㄕ ㄓㄨㄥ ㄍㄨㄛ ㄧㄡ ㄒㄧㄡ ㄨㄣ ㄏㄨㄚ ㄔㄨㄤ ㄊㄨㄥ ㄉㄜ ㄓㄨㄥ ㄧㄠ ㄗㄨ");
	record->setSearchableAttributeValue(2, "是中国优秀文化传统的重要组");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1106);
	record->setSearchableAttributeValue(1,
			"ㄋㄨㄥ ㄇㄧㄣ ㄔㄨ ㄕㄣ PEOPLE ㄉㄜ ㄉㄨ ㄌㄨㄛ ㄧ ㄉㄢ ㄉㄚ ㄨㄤ ㄨㄟ");
	record->setSearchableAttributeValue(2, "农民出身的杜洛伊胆大妄为");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple zhuyin and english
	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_zhuyin",
			"ㄕ ㄧ ㄅㄨ ㄧㄢ ㄕㄨ ㄉㄜ , ㄈㄥ ㄘ ㄒㄧㄥ ㄐㄧ ㄑㄧㄤ ㄉㄜ ㄒㄧㄠ ㄕㄨㄛ NOVEL");
	record->setSearchableAttributeValue("article_hanzi",
			"是一部严肃的、讽刺性极强的小说novel");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Zhuyin Chinese
void testZhuyinChinese() {
	addZhuyinChineseRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "ㄏㄨㄚ", hits -> 1104,1105
	{
		string query = "ㄏㄨㄚ";
		vector<unsigned> recordIds;
		recordIds.push_back(1104);

		recordIds.push_back(1105);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	//Query: "ㄓㄨㄥ", hits -> 1101, 1102, 1104,1105
	{
		string query = "ㄓㄨㄥ";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1104);
		recordIds.push_back(1105);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 3,
						recordIds) == true);

	}

	//search English text
	//Query: "novel", hits -> 1102, 1201
	{
		string query = "novel";

		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	//Query: "ㄏㄨㄚ ㄓㄨㄥ", hits -> 1104 1105
	{
		string query = "ㄏㄨㄚ ㄓㄨㄥ";
		vector<unsigned> recordIds;

		recordIds.push_back(1104);
		recordIds.push_back(1105);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addCroatianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure Croatian characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Priča o Genji");
	record->setSearchableAttributeValue("article_sentence",
			"A klasične literature, imala je velik utjecaj za razvoj japanske književnosti, poznat kao vrhunac japanske klasične književnosti, otvorena je u Japanu bio je tužan eru");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Krzno Država");
	record->setSearchableAttributeValue(2,
			"Kapetan Craventy dao FTE na Fort Reliance. Naši čitatelji ne smije odjednom zamisliti velika zabava");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Blokiran Deep");
	record->setSearchableAttributeValue(2,
			"Datum je između dvadeset i trideset godina.Mjesto je engleski morske luke.");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Od mora književnosti do mora“");
	record->setSearchableAttributeValue("article_sentence",
			" Slobode i nužnosti ju koriste.Motiv i shema da će uspjeti zaustaviti. ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Croatian
void testCroatian() {
	addCroatianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "književnosti ", hits ->1101  1201
	{
		string query = "književnosti";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	//Query: "čitatelji ", hits ->102
	{
		string query = "čitatelji";
		vector<unsigned> recordIds;

		recordIds.push_back(1102);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addCzechRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure Czech characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title",
			"Kde pozdě Sladké zpívali ptáci");
	record->setSearchableAttributeValue("article_sentence",
			"Kde pozdě Sladké zpívali ptáci je sci-fi román od Kate Wilhelm, publikoval v roce 1976. Části ní objevil na oběžné dráze 15 v roce 1974");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Běžný Postal");
	record->setSearchableAttributeValue(2,
			"RRY Pratchett Discworld román 33., byl propuštěn ve Spojeném království dne 25. září 2004. Na rozdíl od většiny Zeměplocha romány Pratchett");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Czech and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "všude where");
	record->setSearchableAttributeValue(2,
			"urban fantasy televizní seriál Neil Gaiman, Miss který nejprve vysílal v roce 1996 na BBC dva.。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Čaroděj ze země království Earthsea");
	record->setSearchableAttributeValue("article_sentence",
			"Le Guin a nastavit v souostroví fantasy světa Earthsea líčit dobrodružství nadějný mladý čaroděj Ged jménem. ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Czech
void testCzech() {
	addCzechRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "Miss ", hits ->1003  1103
	{
		string query = "Miss";
		vector<unsigned> recordIds;
		recordIds.push_back(1003);
		recordIds.push_back(1103);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	//Query: "království ", hits ->1102 1201
	{
		string query = "království ";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addDanishRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure Danish characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Grave Atuan");
	record->setSearchableAttributeValue("article_sentence",
			"den anden af ​​en serie af bøger skrevet af Ursula K. Le Guin og sæt i hendes fantasi skærgård Earthsea");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Den Længst Shore");
	record->setSearchableAttributeValue(2,
			"Den følger op på grave Atuan, bøger hvilket i sig selv var en efterfølger til en Wizard of Earth");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Danish and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Dræb ikke en sangfugl bird");
	record->setSearchableAttributeValue(2,
			"En af disse gammeldags film, saga filmen har en utrolig til at tro, at realismen.。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Hvem ringer klokkerne for who“");
	record->setSearchableAttributeValue("article_sentence",
			"Den spanske borgerkrig er Tomorrow disse allerede blevet en saga blot, og i spring dag har været lidt omtalt personer, og ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Danish
void testDanish() {
	addDanishRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "efterfølger", hits ->1102
	{
		string query = "efterfølger ";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
	}

	//Query: "disse ", hits ->1103 1201
	{
		string query = "disse";
		vector<unsigned> recordIds;
		recordIds.push_back(1103);
		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addDutchRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title",
			"Het Land van de Blinden en andere verhalen");
	record->setSearchableAttributeValue("article_sentence",
			" Ik zit in mijn studie schrijven, hoor ik onze Jane duwen hun weg naar beneden met een borstel en stof pan");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Naar de vuurtoren");
	record->setSearchableAttributeValue(2,
			"Dit is een van de onverdeelde toewijding aan de quasi-autobiografische stroom van bewustzijn roman");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Tra_Chinese and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Een droom van John Ball");
	record->setSearchableAttributeValue(2,
			"Een droom van John Ball (1888) is een roman van Engels schrijver William Morris over Opstand van 138 het Engels Boeren '");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Neer rückhaltlose and bewustzijn Out in Paris und Londen ");
	record->setSearchableAttributeValue("article_sentence",
			"Deze ongewone fictieve rekening, voor een groot deel autobiografisch verteld zonder zelfmedelijden");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Dutch
void testDutch() {
	addDutchRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "bewustzijn";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	{
		string query = "ongewone ";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addEstonianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Bride of Lammermoor");
	record->setSearchableAttributeValue("article_sentence",
			"romaan räägib traagiline armusuhe vahel Lucy Ashton");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Brat Farrar");
	record->setSearchableAttributeValue(2,
			"Sellega lugu salapära ja vahekontole, võõras siseneb sisemine pühapaik");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Tra_Chinese and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Bostonians");
	record->setSearchableAttributeValue(2,
			"mis pakub kvaliteetseid väljaanded mõistliku hinnaga üliõpilane ja laiemale lugejaskonnale,");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Sündinud born paguluses where");
	record->setSearchableAttributeValue("article_sentence",
			"ta kujunenud üks kõige Hendrix saavutatud realistid ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Estonian
void testEstonian() {
	addEstonianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "Hendrix ", hits ->1002  1201
	{
		string query = "Hendrix ";
		vector<unsigned> recordIds;
		recordIds.push_back(1002);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	//Query: "Sündinud ", hits ->1201
	{
		string query = "Sündinud ";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addFinnishRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure Finnish characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Haggai");
	record->setSearchableAttributeValue("article_sentence",
			"Serubbaabelille, kuvernööri Juudan poika Sealtiel, noin Caesar poika Joosua, ylimmäinen pappi sanoi");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Philemon");
	record->setSearchableAttributeValue(2,
			"Paul, Kristuksen Jeesuksen vanki, sama veli Timoteus Filemonille rakkaan työtovereita");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Finnish and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "heprea book");
	record->setSearchableAttributeValue(2,
			"Jumala muinoin profeettojen kautta monta wish come kertaa ja eri tavoin puhui isille god。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Timotei “");
	record->setSearchableAttributeValue("article_sentence",
			"Jumala Vapahtajamme Jeesus Kristus, ja odotamme elämään,Paul kuten Paavali, Kristuksen Jeesuksen apostoli ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Finnish
void testFinnish() {
	addFinnishRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "Paul", hits ->1102  1201
	{
		string query = "Paul";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	//Query: "kuvernööri", hits ->1101
	{
		string query = "kuvernööri ";
		vector<unsigned> recordIds;

		recordIds.push_back(1101);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addGermanRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure German characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Of Mice and Men");
	record->setSearchableAttributeValue("article_sentence",
			"Die Geschichte umfasst den Zeitraum von höchstens drei Tagen ist die Lage ein California Ranch");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Liebling");
	record->setSearchableAttributeValue(2,
			"Female schwarz Nu Sesi schwanger und allein aus Kentucky Slave Manor flohen nach Cincinnati, Ohio, Sklavenhalter durch den Track jagen");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple German and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Die Kunst war des Krieges art ");
	record->setSearchableAttributeValue(2,
			"Helle Juwel in der klassischen chinesischen culture militärischen Kulturerbe ist ein wichtiger Bestandteil der important feinen kulturellen Traditionen Chinas。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Schöne Freunde friend“");
	record->setSearchableAttributeValue("article_sentence",
			"Bauer Duluo Yi kühn, gefühllos und grausam, mit schönem Aussehen Duchuang Paris ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test German
void testGerman() {
	addGermanRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "Kentucky ", hits ->1102
	{
		string query = "Kentucky";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
	}

	//Query: "militärischen  important ", hits ->1103
	{
		string query = "militärischen  important";
		vector<unsigned> recordIds;

		recordIds.push_back(1103);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addGreekRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Desert Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure Greek characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title",
			"Συζητήσεις με το Θεό");
	record->setSearchableAttributeValue("article_sentence",
			"μια σειρά από βιβλία γραμμένα από Neale Donald Walsch, γραμμένο σαν ένα διάλογο στον οποίο Walsch θέτει ερωτήματα");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Ρωμαίοι");
	record->setSearchableAttributeValue(2,
			"Paul, ένας υπηρέτης του Ιησού Χριστού, καλείται να είναι Ιησούς απόστολος, αποστολή ευαγγέλιο του Θεού");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Greek and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Ρεύματα εν τη ερημία Desert");
	record->setSearchableAttributeValue(2,
			"Το βιβλίο δεν είναι μόνο το 20ο αιώνα είναι αρκετά book γνωστή moving θρησκευτικά γραπτά Επίσης αναφέρθηκε στο είναι αρκετά συγκινητικά。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Πράξεις acts“");
	record->setSearchableAttributeValue("article_sentence",
			"Ο πρώην πραγματεία έχω κάνει, O Θεόφιλος, απ 'όλα ότι ο Ιησούς άρχισε να κάνει τόσο και να διδάξουν。 ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Greek
void testGreek() {
	addGreekRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "Desert ", hits ->1003  1103
	{
		string query = "Desert ";
		vector<unsigned> recordIds;
		recordIds.push_back(1003);
		recordIds.push_back(1103);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	//Query: "Ιησούς ", hits ->1102 1201
	{
		string query = "Ιησούς";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addHungarianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	// pure Hungarian characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Framley Parsonage");
	record->setSearchableAttributeValue("article_sentence",
			"Amikor a fiatal Mark Robarts elhagyta egyetemre, az apja is jól kijelenti, hogy minden ember kezdte mondani minden jó dolog neki");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Virágzás Wilderness");
	record->setSearchableAttributeValue(2,
			"Dinny Cherrell tettek javaslatot, hogy többször. De senki sem került közel a megérintette a független szellem.");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Hungarian and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Az első és az utolsó dolog,");
	record->setSearchableAttributeValue(2,
			"Miután tanult tudomány és become különösen a biológiai teacher  tudomány megérintette néhány éve lettem tanár az iskolában a fiúk。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "a finanszírozó");
	record->setSearchableAttributeValue("article_sentence",
			"Ez a ritka régészeti könyv egy válogatást Kessinger Publishing Legacy Reprint sorozat ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Hungarian
void testHungarian() {
	addHungarianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "megérintette ", hits ->1102  1103
	{
		string query = "megérintette";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1103);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	//Query: "Kessinger ", hits ->1201
	{
		string query = "Kessinger";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addIndonesiaRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Black Magic");
	record->setSearchableAttributeValue("article_sentence",
			"Di ruang besar rumah di kota yang tenang tertentu di Flanders");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Burung Pemangsa");
	record->setSearchableAttributeValue(2,
			"berikut adalah beberapa rumah kadarnya aspek lahiriah");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "The Belton Estate Pemangsa");
	record->setSearchableAttributeValue(2,
			"Dikatakan dari mereka yang kecil dan bengkok yang didukung dalam tubuh mereka");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Binatang dan Super-hewan“");
	record->setSearchableAttributeValue("article_sentence",
			"Binatang dan Super More Binatang Smith Sacchi home salah satu karya paling terkenal ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Indonesia
void testIndonesia() {
	addIndonesiaRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	//Query: "Pemangsa ", hits ->1102  1103
	{
		string query = "Pemangsa";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	//Query: "More ", hits ->1003 1201
	{
		string query = "More";
		vector<unsigned> recordIds;
		recordIds.push_back(1003);

		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addItalianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Felix Holt Radicale");
	record->setSearchableAttributeValue("article_sentence",
			"quando è entrato, non era in uno stato d'animo attento, e quando, dopo sedendosi,");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "La difesa di Guenevere");
	record->setSearchableAttributeValue(2,
			"Questa è una riproduzione di un libro pubblicato prima del 1923. Questo libro non può essere spedito in quali pagine mancanti o offuscata");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "il Deerslayer");
	record->setSearchableAttributeValue(2,
			"Questa è una riproduzione di un libro pubblicato prima del 1923. Questo  year libro non può essere spedito in quali pagine mancanti o offuscata,。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Debiti e crediti");
	record->setSearchableAttributeValue("article_sentence",
			"ha due aspetti fondamentali di ogni transazione finanziaria nel sistema di contabilità in partita doppia ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Italian
void testItalian() {
	addItalianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "Questo";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1103);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);
	}

	{
		string query = "finanziaria ";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addKoreanRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, " Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "새벽");
	record->setSearchableAttributeValue("article_sentence",
			"영혼 날개 달린 발이 우리의 스타 족쇄에서 뭔가 무딘; 우리의 본성이 완전 시들다 그러나 영광은 멀리서 우리를 이동");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "ZENDA의 죄수");
	record->setSearchableAttributeValue(2,
			"궁극적 인 현실 도피의 모험 이야기는 ZENDA의 죄수 과거 시대로 청취자를 전송");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "어두운 dark빈");
	record->setSearchableAttributeValue(2,
			"여전히 그의 아내와 딸의 잔인한 slayings에서 원시, 그들의 살인자의 home캡처를 둘러싼 사건。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "농담을 치콧“");
	record->setSearchableAttributeValue("article_sentence",
			"그의 1923 년 이전에 발행 된 도서의 정확한 ​​재현. 이것은 come이상한 문자 OCR'd 책 NOT IS ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Korean
void testKorean() {
	addKoreanRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "를 ";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1102);

		recordIds.push_back(1103);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 3,
						recordIds) == true);
	}

	{
		string query = "come ";
		vector<unsigned> recordIds;
		recordIds.push_back(1001);
		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addLatvianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title",
			"Uz   Constance bākas");
	record->setSearchableAttributeValue("article_sentence",
			"Šis ir viens no visas sirds veltījums kvazi-autobiogrāfiskā plūsmā apziņas romāns");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Vai jūs varat piedot viņai?");
	record->setSearchableAttributeValue(2,
			"Alice Vavasor nevar izlemt, vai precēties viņas vērienīgu");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Camilla");
	record->setSearchableAttributeValue(2,
			"Kamilla nodarbojas ar laulības bažas grupas jaunieši, Camilla");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Brāļi Karamazovi“");
	record->setSearchableAttributeValue("article_sentence",
			"Constance Garnetts tulkošana, bāzes versija angļu valodā ir šajā krievu šedevrs");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Latvian
void testLatvian() {
	addLatvianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "Constance ";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	{
		string query = "valodā";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addLithuanianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Iš ZENDA kalinys");
	record->setSearchableAttributeValue("article_sentence",
			"jis belaisvis ZENDA veža klausytoją į praeities erą");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Tamsiai Tuščiaviduris");
	record->setSearchableAttributeValue(2,
			"Vis dar žalias nuo brutalių slayings jo žmona ir dukra");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Chicot į Jester");
	record->setSearchableAttributeValue(2,
			"Tai tiksli reprodukcija exact knygoje skelbiama iki 1923。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Aklųjų ir kitų  brutalių istorijų Šalis“");
	record->setSearchableAttributeValue("article_sentence",
			"Kaip aš sėdi raštu mano studijų girdžiu mūsų Jane wing nelygumai savo kelią žemyn su ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Lithuanian
void testLithuanian() {
	addLithuanianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "brutalių ";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	{
		string query = "wing";
		vector<unsigned> recordIds;
		recordIds.push_back(1002);

		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addNorwegianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1,  schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_title",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Dorothy Forster");
	record->setSearchableAttributeValue("article_sentence",
			"Gjennomføre så ulastelig, gravitasjon så entall, visdom så bemerkelsesverdig, aldri før har sett i en mann så ung");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Innenriks Manners av amerikanerne");
	record->setSearchableAttributeValue(2,
			"Dette er en reproduksjon av en bok utgitt før 1923. Denne boken kan ha sporadiske småfeil som mangler eller uskarpe sider");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Doctor Thorne så");
	record->setSearchableAttributeValue(2,
			"er Frank Gresham oppsatt på å gifte seg med sin elskede Mary Thorne, til tross Tomorrow for hennes illegitimitet og tilsynelatende fattigdom");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Et mangfold av Creatures“");
	record->setSearchableAttributeValue("article_sentence",
			"semi-nominerte kroppen av noen få poengsum personer, styrer Planet. Transport er Sivilisasjone ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Norwegian
void testNorwegian() {
	addNorwegianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 2;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
				mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1);
	index->bootStrapFromDisk();
	index->createAndStartMergeThreadLoop();
	    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "så";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	{
		string query = "Tomorrow";
		vector<unsigned> recordIds;
		recordIds.push_back(1003);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}



int main(int argc, char **argv) {

	//test simple chinese
	testSimpleChinese();
	cout << "test Chinese passed" << endl;

	testTranditionalChinese();
	cout << "test traditional Chinese passed" << endl;

	// test Zhuyin(注音)
	testSimpleZhuyin();
	cout << "test Zhuyin passed" << endl;

	testPinyinChinese();
	cout << "test pinyin-Chinese passed" << endl;

	testZhuyinChinese();
	cout << "test zhuyin-Chinese passed" << endl;

	testBulgarian();
	cout << "test Bulgarian passed" << endl;

	testCzech();
	cout << "test Czech passed" << endl;

	testGerman();
	cout << "test German passed" << endl;

	testGreek();
	cout << "test Greek passed" << endl;

	testFinnish();
	cout << "test Finnish passed" << endl;

	//test French
	testFrench();
	cout << "test French passed" << endl;

	testCroatian();
	cout << "test Croatian passed" << endl;

	testHungarian();
	cout << "test Hungarian passed" << endl;

	testItalian();
	cout << "test Italian passed" << endl;

	//test Japanese
	testJapanese();
	cout << "test Japanese passed" << endl;

	testKorean();
	cout << "test Korean passed" << endl;

	testDutch();
	cout << "test Dutch passed" << endl;

	testNorwegian();
	cout << "test Norwegian passed" << endl;

	testLithuanian();
	cout << "test Lithuanian passed" << endl;

	testLatvian();
	cout << "test Latvian passed" << endl;

	testEstonian();
	cout << "test Estonian passed" << endl;

	testDanish();
	cout << "test Danish passed" << endl;

	testIndonesia();
	cout << "test Indonesia passed" << endl;

	return 0;
}
