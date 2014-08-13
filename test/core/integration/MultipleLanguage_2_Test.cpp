/*
 * MultipleLanguage_2_Test.cpp
 *
 *  Created on: Oct 3, 2013
 *      Author: srch2
 */
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

string DBLP_INDEX_DIR = getenv("dblp_index_dir");
string INDEX_DIR = getenv("small_index_dir");

//from http://novel.tingroom.com/ translated by google
void addPolishRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
	record->setSearchableAttributeValue("article_title", "dyskursów");
	record->setSearchableAttributeValue("article_sentence",
			"Ze wszystkich kierunków, znajdziesz nie jeden, który jest w stanie kontemplacji się");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Przemówienie zachodniej Sadzenie");
	record->setSearchableAttributeValue(2,
			"on po dyskurs, jeden z najciekawszych i składek cenne dla historii wczesnego wykrywan");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Diana z Crossways");
	record->setSearchableAttributeValue(2,
			"George Meredith, OM (1828-1909), angielski pisarz i poeta. Studiował prawo i został Artykularny jako radca prawny");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Obrona GUENEVERE");
	record->setSearchableAttributeValue("article_sentence",
			"To jest powielanie książki wydanej przed 1923. Ta dyskursów książka może być sporadyczne niedoskonałości, takich jak brakujące lub niewyraźne strony ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Polish
void testPolish() {
	addPolishRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "dyskursów";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
	}

	{
		string query = "takich";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addPortugueseRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
			"As Aventuras de Roderick Aleatório");
	record->setSearchableAttributeValue("article_sentence",
			"aqui não é tão divertido e melhorando universalmente, tal como a que é introduzida, como se fosse, ocasionalmente,");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "As Aventuras de Peregrine Pickle");
	record->setSearchableAttributeValue(2,
			"Em um certo condado da Inglaterra, delimitada de um lado pelo mar");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Terças com Morrie");
	record->setSearchableAttributeValue(2,
			"A história foi adaptada mais tarde por Thomas Rickman em um filme de TV de mesmo nome, dirigido");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Ir Tell It On The Mountain");
	record->setSearchableAttributeValue("article_sentence",
			"O livro analisa o papel da Igreja cristã na vida dos Africano-Americanos");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Portuguese
void testPortuguese() {
	addPortugueseRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "Aventuras";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1102);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
	}

	{
		string query = "cristã";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addRomanianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
	record->setSearchableAttributeValue("article_title", "Cei trei muschetari");
	record->setSearchableAttributeValue("article_sentence",
			"ecounts aventurile unui tânăr pe nume d'Artagnan după ce pleacă");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Bogat Tata tata sarac");
	record->setSearchableAttributeValue(2,
			"Acesta susține independența financiară prin investiții, imobiliare,");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Sea Wolf imobiliare");
	record->setSearchableAttributeValue(2,
			"care intra sub dominația Wolf Larsen, puternic și amoral căpitan care de salvare");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Povestea vieții mele");
	record->setSearchableAttributeValue("article_sentence",
			"ȚII de ea au fost adaptate de William Gibson pentru un 1957 căpitan");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Romanian
void testRomanian() {
	addRomanianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "imobiliare";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	{
		string query = "căpitan";
		vector<unsigned> recordIds;
		recordIds.push_back(1103);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addRussianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
	record->setSearchableAttributeValue("article_title", "Побег из Шоушенка");
	record->setSearchableAttributeValue("article_sentence",
			"это адаптация повести Стивена Кинга Рита Хейворт");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "На дороге");
	record->setSearchableAttributeValue(2,
			"Это в значительной степени автобиографическая работа, которая была основана на спонтанной");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Как закалялась сталь Steel");
	record->setSearchableAttributeValue(2,
			"Дверь балкона была открыта, а под занавес Tomorrow перемешивают на ветру, заполнив。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"И восходит солнце sun“");
	record->setSearchableAttributeValue("article_sentence",
			"Он восходит солнце стоит как,talk пожалуй, самым впечатляющим первый роман novel ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Russian
void testRussian() {
	addRussianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "Tomorrow ";
		vector<unsigned> recordIds;
		recordIds.push_back(1003);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	{
		string query = "спонтанной ";
		vector<unsigned> recordIds;

		recordIds.push_back(1101);
		recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 3,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addSerbianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
	record->setSearchableAttributeValue("article_title", "Фрозен Дееп");
	record->setSearchableAttributeValue("article_sentence",
			"Време је ноћ. А посао тренутка плеше");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Од мора до мора");
	record->setSearchableAttributeValue(2, "Мотив и шема која ће доћи у ништа");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Фрамлеи Two Парсонаге first");
	record->setSearchableAttributeValue(2,
			"Када је млади Марк Робартс напуштања Tomorrow колеџа, његов отац можда и изјављујем да су сви људи。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Цветање Вилдернес");
	record->setSearchableAttributeValue("article_sentence",
			" Први су су се упознали More на венчању Флеур Little и Мајкл Монт је и искра ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Serbian
void testSerbian() {
	addSerbianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "Tomorrow";
		vector<unsigned> recordIds;
		recordIds.push_back(1003);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	{
		string query = "Вилдернес";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addSlovakRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
			"Záhradná slávnosť a iné príbehy");
	record->setSearchableAttributeValue("article_sentence",
			"Lawrence a niečo ako súper Virginie Woolfovej. Mansfield tvorivé rokov");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Záhradná prežitie");
	record->setSearchableAttributeValue(2,
			"IT prekvapí a zároveň možno pobaví, aby ste vedeli,");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Zo Zeme na earth Mesiac moon");
	record->setSearchableAttributeValue(2,
			"Počas vojny povstania, bol nový poor a vplyvný Yesterday klub založený v meste Baltimore v štáte Maryland");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Franchise Affair“");
	record->setSearchableAttributeValue("article_sentence",
			"Robert Blair chystal ukradnúť z pomalej deň v jeho advokátskej kancelárii, keď zazvonil telefó ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Slovak
void testSlovak() {
	addSlovakRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "Záhradná";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1102);
		//recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	{
		string query = "Yesterday ";
		vector<unsigned> recordIds;
		recordIds.push_back(1001);

		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addSlovenianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
	record->setSearchableAttributeValue(2,
			"Come Tomorrow Two življenja More first");
	record->setRecordBoost(10);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Lisica ženska");
	record->setSearchableAttributeValue("article_sentence",
			"Nekateri dušo tišine, starodavno in potrpežljiv kot korake, brooded nad njimi.");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Flappersu in Filozofi");
	record->setSearchableAttributeValue(2,
			"To verjetno zgodba se začne na morju, ki je bil modro sanje, kot barvita kot modro-svilene nogavice");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Mož, ki se je  man bal");
	record->setSearchableAttributeValue(2,
			"out od najtemnejših globin življenja, kjer je podpredsednik in kriminal in bedo bahajo");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Določenem obdobju“");
	record->setSearchableAttributeValue("article_sentence",
			"Je del utopija, del distopija, del temno satira, s pridihom sodobnega steampunk ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Slovenian
void testSlovenian() {
	addSlovenianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "življenja ";
		vector<unsigned> recordIds;
		recordIds.push_back(1003);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	{
		string query = "Določenem";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addSpanishRecords() {
	///Create Schema Spanish
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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

	// pure Spanish characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "Lucas");
	record->setSearchableAttributeValue("article_sentence",
			"Teófilo, que, para muchas personas tomar una pluma para el libro que cuenta la historia entre nosotros lo han hecho");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Mateo");
	record->setSearchableAttributeValue(2,
			"Los descendientes de Abraham, hijo de David, la genealogía de Jesucristo");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Spanish and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Malaquías ");
	record->setSearchableAttributeValue(2,
			"SEÑOR a Israel por medio Tomorrow de Malaquías miss implicaba 。");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Zechariah Zacarías“");
	record->setSearchableAttributeValue("article_sentence",
			"El segundo año del rey Little Darío agosto Jehová á wing ddo Berequías, hijo del profeta Zacarías ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Spanish
void testSpanish() {
	addSpanishRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "genealogía ";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
	}

	{
		string query = "Little ";
		vector<unsigned> recordIds;
		recordIds.push_back(1002);
		recordIds.push_back(1201);

		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addSwedishRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
			"Första och sista saker");
	record->setSearchableAttributeValue("article_sentence",
			"Efter att jag hade studerat naturvetenskap och särskilt biologisk vetenskap för några år");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "finansiären");
	record->setSearchableAttributeValue(2,
			"På grund av sin ålder, kan den innehålla brister såsom Markeringar, noteringar, marginaliaen");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Felix Holt Radical");
	record->setSearchableAttributeValue(2,
			"Felix Holt, när han kom in, var inte i en observant humör, och när,");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"En dröm av såsom John Ball“");
	record->setSearchableAttributeValue("article_sentence",
			"Engelska författaren William Morris om engelska böndernas revolt av 1381 och rebellen John Ball. ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Swedish
void testSwedish() {
	addSwedishRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "såsom ";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	{
		string query = "böndernas ";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addThaiRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
			"ลงและออกในกรุงปารีสและลอนดอน");
	record->setSearchableAttributeValue("article_sentence",
			"นี้ละครที่ผิดปกติในอัตชีวประวัติส่วนดี narrates โดยไม่ต้องสงสารตัวเอง");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "ฟอสเตอร์โดโรธี");
	record->setSearchableAttributeValue(2,
			"ดำเนินแรงโน้มถ่วงไม่มีที่ติดังนั้นภูมิปัญญาเอกพจน์โดดเด่นเพื่อ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1,
			"มารยาทในประ manners เทศของชาวอเมริกัน");
	record->setSearchableAttributeValue(2,
			"นี่คือการทำสำเนาห Tomorrow นังสือที่ตีพิมพ์ก่อนที่ 1923");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Thorne แพทย์");
	record->setSearchableAttributeValue("article_sentence",
			"แฟรงค์ Gresham เป็นความตั้งใจที่จะแต่งงานกับสุดที่รักของเขาแมรี่ Thorne ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Thai
void testThai() {
	addThaiRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "Tomorrow ";
		vector<unsigned> recordIds;
		recordIds.push_back(1003);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	{
		string query = "บสุดที่ ";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addTurkishRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
			"Yaratıklar bir Çeşitlilik");
	record->setSearchableAttributeValue("article_sentence",
			"sloganımız çalışır. Teorik olarak biz ne biz savunma");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Söylemler");
	record->setSearchableAttributeValue(2,
			"Ne kadar gramatik sanat tefekkür gücüne sahip mi?");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Batı Dikim Bir Söylemler");
	record->setSearchableAttributeValue(2,
			"Yeni Dünya erken keşif home Tarihi için en lost meraklı ve değerli katkılarından biri,");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Crossways Diana“");
	record->setSearchableAttributeValue("article_sentence",
			". O hukuk okumak ve bir avukat olarak sözleşmeli edildi ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Turkish
void testTurkish() {
	addTurkishRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "Söylemler";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	{
		string query = "sözleşmeli ";
		vector<unsigned> recordIds;

		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addUkrainianRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
	record->setSearchableAttributeValue("article_title", "Синя книга Фея");
	record->setSearchableAttributeValue("article_sentence",
			"З людьми, що посієш, те й пожнеш,");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "сліпа любов");
	record->setSearchableAttributeValue(2,
			"нарочним порушених упокій Денніс Howmore");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "холодний будинок");
	record->setSearchableAttributeValue(2,
			"Місцевих найстаріших сімей, які year опинилися  him в  that незручне становище");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title", "Чорна him мантія “");
	record->setSearchableAttributeValue("article_sentence",
			"Цілком можливо, що жінки не мають позитивні оцінки того, що ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Ukrainian
void testUkrainian() {
	addUkrainianRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "him";
		vector<unsigned> recordIds;
		recordIds.push_back(1103);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	{
		string query = "любов";
		vector<unsigned> recordIds;

		recordIds.push_back(1102);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addVietnameseRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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
			"Quốc phòng của Guenevere");
	record->setSearchableAttributeValue("article_sentence",
			"Đây là một bản sao của một cuốn sách xuất bản trước năm 1923");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "các Deerslayer");
	record->setSearchableAttributeValue(2,
			"Crack chết người của một súng trường và tiếng kêu xuyên của Ấn Độ trên các warpat");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "Ghi nợ và Tín");
	record->setSearchableAttributeValue(2,
			"Thẻ ghi nợ và tín dụng là hai khía cạnh cơ bản của tất cả các giao dịch tài chính");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"Deerslayer bình minh");
	record->setSearchableAttributeValue("article_sentence",
			"Bản chất của chúng ta suy yếu không đầy đủ; Một cái gì đó trong tù này ngôi sao của chúng tôi ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Vietnamese
void testVietnamese() {
	addVietnameseRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "Deerslayer";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1201);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	{
		string query = "trường ";
		vector<unsigned> recordIds;

		recordIds.push_back(1102);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addFarsiRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_tittle", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_tittle",
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

	// pure Tra_Chinese characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_tittle", "Barnaby چین Rudge");
	record->setSearchableAttributeValue("article_sentence",
			"اظهار نظر او که کلاغ به تدریج منقرض شدن");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "واحد ضد چین");
	record->setSearchableAttributeValue(2,
			"مواد منفجره، وکیل مدافع جوزف آنتونلی مورد از دست دادن هرگز - و یا پشت سر هم از وجدان");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Farsi
void testFarsi() {
	addFarsiRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();
	{
		string query = "چین ";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}
	//Test a record that includes both French and English

	{
		string query = "اظهار";
		vector<unsigned> recordIds;

		recordIds.push_back(1101);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 1,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addArabicRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_title", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

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

	// pure Arabic characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_title", "الرجل مانيرينغ");
	record->setSearchableAttributeValue("article_sentence",
			"جعلت رواية أو الرومانسية من يفرلي طريقها إلى الجمهور ببطء، وبطبيعة الحال");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "حزب حديقة وقصص أخرى");
	record->setSearchableAttributeValue(2,
			"مانسفيلد هو الكاتب الأكثر شهرة في نيوزيلندا.");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "والفراء الدولة");
	record->setSearchableAttributeValue(2,
			"يجب أن القراء ليس دفعة واحدة تخيل الترفيه الكبرى، مثل الكرة المحكمة، أو بسهرة موسيقية");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1104);
	record->setSearchableAttributeValue(1, "ومجمدة");
	record->setSearchableAttributeValue(2,
			"رئيس بلدية ومؤسسة من المدينة وإعطاء الكرة الكبرى");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1105);
	record->setSearchableAttributeValue(1, "من الأرض إلى القمر");
	record->setSearchableAttributeValue(2,
			"خلال الحرب من تمرد، تم إنشاء ناد جديد ومؤثر في مدينة بالتيمور في ولاية ميريلاند");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1106);
	record->setSearchableAttributeValue(1, "من البحر إلى البحر");
	record->setSearchableAttributeValue(2,
			"الحرية وضرورة استخدام لها. الحافز وخطة من شأنها أن تأتي إلى");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1107);
	record->setSearchableAttributeValue(1, "فرانكشتاين فرانكشتاين");
	record->setSearchableAttributeValue(2,
			"العلماء فرانكشتاين استبدال دور الخالق في محاولة لخلق الحياة في أيديهم");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1108);
	record->setSearchableAttributeValue(1, "الحياة قضية الفرنشايز هو");
	record->setSearchableAttributeValue(2,
			"وكان روبرت بلير على وشك ضرب قبالة من يوم بطيئة في مكتب محاماة له عندما رن جرس ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1109);
	record->setSearchableAttributeValue(1, "المرأة الثعلب");
	record->setSearchableAttributeValue(2,
			"الجرح الخطوات القديمة حتى من جانب الجبل من خلال أشجار الصنوبر طويل القامة، والصبر في عمق مداس عليها من قبل أقدام من عشرين قرنا");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1110);
	record->setSearchableAttributeValue(1, "الزعانف والفلاسفة");
	record->setSearchableAttributeValue(2,
			"تبدأ هذه القصة من غير المرجح على البحر الذي كان حلما الأزرق، ملونة مثل جوارب زرقاء الحرير،");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Arabic and english
	record->clear();
	record->setPrimaryKey(1201);
	record->setSearchableAttributeValue("article_title",
			"bookالحياة بيت القسيس");
	record->setSearchableAttributeValue("article_sentence",
			"عند الشباب Robarts مارك كان يغادر collegeالكلية،أيضاwas والدهleaving قد أعلن   ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//Test a record that includes both Arabic and English
	record->clear();
	record->setPrimaryKey(1202);
	record->setSearchableAttributeValue(1, "Miss الرجل الذي كان يخاف");
	record->setSearchableAttributeValue(2,
			"من أحلك أعماق الحياة، viceحيث الرذيلة والجريمة centuryوالبؤس وتكثر، ويأتي بايرون من lifeالقرن ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1203);
	record->setSearchableAttributeValue(1, "");
	record->setSearchableAttributeValue(2,
			"Missمن أحلك أعماق الحياة يخاف، viceحيث الرذيلة والجريمة centuryوالبؤس وتكثر، ويأتي بايرون من lifeالقرن ");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Arabic
void testArabic() {
	addArabicRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();
	//We use popular English novel and translate into Arabic,using the content and their titles to test if the engine can support Chinese characters. The data was obtained from www.baidu.com search "中国 诗词名句"
	//Query: "مثل", hits -> 1103, 1110
	{
		string query = "مثل";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1103);
		recordIds.push_back(1105);
		recordIds.push_back(1109);
		recordIds.push_back(1110);
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 7, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 7, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 7, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 7, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
	}

	//search English text
	//Query: "book", hits -> 1001, 1201
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1001);
		recordIds.push_back(1201);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, "book", 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, "book", 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, "book", 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, "book", 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
	}

	//Test a record that includes both Arabic and English
	//Query: "Missيخاف", hits -> 1003, 1201
	{
		string query = "Missيخاف";
		vector<unsigned> recordIds;
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
	}

	//search long English and Arabic text
	//Query: "viceحيث الرذيلة والجريمة century", hits -> 1003, 1201
	{
		string query = "viceحيث الرذيلة والجريمة century";
		vector<unsigned> recordIds;
		recordIds.push_back(1202);
		recordIds.push_back(1203);
		ASSERT(
				pingExactPrefix(analyzer, queryEvaluator, query, 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingFuzzyPrefix(analyzer, queryEvaluator, query, 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
		ASSERT(
				pingFuzzyComplete(analyzer, queryEvaluator, query, 2, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND)
						== true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addHebrewRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_tittle", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_tittle",
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

	// pure Tra_Chinese characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_tittle", "הספר של סנובים");
	record->setSearchableAttributeValue("article_sentence",
			"יום זה כ יצירות פילוסופיות חשובות, בשמו שלו שנים מאוחר יותר פילוסופיה בוגרת יותר");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "חרקים ארבה בסיס");
	record->setSearchableAttributeValue(2,
			"ספר זה עוקב אחרי בחור צעיר בשם טוד האקט רואה את עצמו כצייר ואמן");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	//simple Tra_Chinese and english
	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "יחידת סין הוכחה");
	record->setSearchableAttributeValue(2,
			" חומרי נפץ,test הסנגור ג'וזף אנטונלי מעולם לא Tomorrow  מאבדים את מקרה - או פרץ של מצפון צורב בוא");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Hebrew
void testHebrew() {
	addHebrewRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();
	{
		string query = "Tomorrow";
		vector<unsigned> recordIds;
		recordIds.push_back(1003);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);

	}
	//Test a record that includes both French and English

	{
		string query = "סנובים";
		vector<unsigned> recordIds;

		recordIds.push_back(1101);
		recordIds.push_back(1103);

		ASSERT(
				pingExactComplete(analyzer, queryEvaluator, query, 2,
						recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
	}

	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//from http://novel.tingroom.com/ translated by google
void addKazakhRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_tittle", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_tittle",
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

	// pure Tra_Chinese characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_tittle", "тазалап тастапты ");
	record->setSearchableAttributeValue("article_sentence",
			"тастағанға ұқсайды");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "Жеп home отырмын");
	record->setSearchableAttributeValue(2,
			"тастапты ұқсайды тастағанға");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);



	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

// test Kazakh
void testKazakh()
{
	addKazakhRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites,
			updateHistogramEveryPMerges, updateHistogramEveryQWrites,
			INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "тастағанға ";
		vector<unsigned> recordIds;
                recordIds.push_back(1101);
		recordIds.push_back(1102);

		ASSERT(pingExactComplete(analyzer, queryEvaluator, query, 2,recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
	}


	delete analyzer;
	delete queryEvaluator;
	delete index;
}

// from http://my.wikipedia.org/wiki/Wikipedia:Font
void addBurmeseRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_sentence", 2); // searchable text
	schema->setSearchableAttribute("article_translate", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_sentence",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_translate",
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

	// pure Tra_Chinese characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_sentence", "ပြည်ထောင်စု သမ္မတ မြန်မာနိုင်ငံတော်");
	record->setSearchableAttributeValue("article_translate",
			"Republic of the Union of Myanmar");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "ဤစာမျက်နှာသည် ရည်ရွယ်ချက်ရှိလို့ အင်္ဂလိပ်စာနှင့်ရေးထားသည်။ ဘာသာမပြန်ပါနှင့်။");
	record->setSearchableAttributeValue(2,
			"This article is written in English for a specific purpose. Please do not translate it.");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);



	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

//test Burmese
void testBurmese()
{
	addBurmeseRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites,
			updateHistogramEveryPMerges, updateHistogramEveryQWrites,
			INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "ဘာသာမပြန်ပါ";
		vector<unsigned> recordIds;
		recordIds.push_back(1102);

		ASSERT(pingExactComplete(analyzer, queryEvaluator, query, 1,recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
	}


	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//Portuguese - Brazil The main differece from Portuguese is the voice and some write, the character is same. From http://en.wikipedia.org/wiki/Brazilian_Portuguese
void addPortugueseBrazilRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_sentence", 2); // searchable text
	schema->setSearchableAttribute("article_translate", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_sentence",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_translate",
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

	// pure Tra_Chinese characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_sentence", "carteira de habilitação, carteira de motorista, carta");
	record->setSearchableAttributeValue("article_translate",
			"driver's license (US), driving licence (UK)");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "trem, composição ferroviária");
	record->setSearchableAttributeValue(2,
			"train");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);



	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

//test Portuguese - Brazil
void testPortugueseBrazil()
{
	addPortugueseBrazilRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "habilitação";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);

		ASSERT(pingExactComplete(analyzer, queryEvaluator, query, 1,recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
	}


	delete analyzer;
	delete queryEvaluator;
	delete index;
}

//Spanish - Latin America The main differece from Spanish is the voice and some write, the character is same. From http://novel.tingroom.com/ translated by http://www.spanishdict.com/translate/Latin%20America
void addSpanishLatinRecords() {
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_sentence", 2); // searchable text
	schema->setSearchableAttribute("article_translate", 7); // searchable text

	Record *record = new Record(schema);

	Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_sentence",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_translate",
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

	// pure Tra_Chinese characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_sentence", "La novela o el Romance de Waverley hizo su manera al público");
	record->setSearchableAttributeValue("article_translate",
			"The Novel or Romance of Waverley made its way to the public slowly");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1102);
	record->setSearchableAttributeValue(1, "pero luego con tal popularidad acumulando que fomenten al autor a un segundo intento");
	record->setSearchableAttributeValue(2,
			"but afterwards with such accumulating popularity as to encourage the Author to a second attempt");
	record->setRecordBoost(90);
	index->addRecord(record, analyzer);



	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}

//test Spanish-Latin
void testSpanishLatin()
{
	addSpanishLatinRecords();
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites,
			updateHistogramEveryPMerges, updateHistogramEveryQWrites,
			INDEX_DIR);

	Indexer *index = Indexer::load(indexMetaData1);
    QueryEvaluatorRuntimeParametersContainer runtimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(index, &runtimeParameters);
	Analyzer *analyzer = getAnalyzer();

	{
		string query = "público";
		vector<unsigned> recordIds;
		recordIds.push_back(1101);

		ASSERT(pingExactComplete(analyzer, queryEvaluator, query, 1, recordIds, vector<unsigned>(), ATTRIBUTES_OP_AND) == true);
	}


	delete analyzer;
	delete queryEvaluator;
	delete index;
}

void addFarsiRecordsWithNonSearchableAttribute(){
	///Create Schema
	Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
	schema->setPrimaryKey("article_id"); // integer, not searchable
	schema->setSearchableAttribute("article_id"); // convert id to searchable text
	schema->setSearchableAttribute("article_tittle", 2); // searchable text
	schema->setSearchableAttribute("article_sentence", 7); // searchable text

	schema->setRefiningAttribute("price" , ATTRIBUTE_TYPE_UNSIGNED , "0" );
	schema->setRefiningAttribute("class" , ATTRIBUTE_TYPE_TEXT , "الف" );


	Record *record = new Record(schema);
        Analyzer *analyzer = new Analyzer(NULL, NULL, NULL, NULL, "");

	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData(new CacheManager(),
			mergeEveryNSeconds, mergeEveryMWrites, updateHistogramEveryPMerges, updateHistogramEveryQWrites, INDEX_DIR);

	Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

	//pure english
	record->setPrimaryKey(1001);
	record->setSearchableAttributeValue("article_tittle",
			"book Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue("article_sentence",
			"Come Yesterday Once More");
	record->setRefiningAttributeValue("price" , "1001");
	record->setRefiningAttributeValue("class" , "الف");

	record->setRecordBoost(90);

	index->addRecord(record, analyzer);

	record->clear();
	record->setPrimaryKey(1002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");

	record->setRefiningAttributeValue("price" , "1002");
	record->setRefiningAttributeValue("class" , "ب");

	record->setRecordBoost(90);

	index->addRecord(record,analyzer);

	record->clear();
	record->setPrimaryKey(1003);
	record->setSearchableAttributeValue(1, "Mr Smith and Miss Smith");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More first");

	record->setRefiningAttributeValue("price" , "1003");
	record->setRefiningAttributeValue("class" , "C");

	record->setRecordBoost(10);
	index->addRecord(record,analyzer);

	// pure Tra_Chinese characters
	record->clear();
	record->setPrimaryKey(1101);
	record->setSearchableAttributeValue("article_tittle", "Barnaby چین Rudge");
	record->setSearchableAttributeValue("article_sentence",
			"اظهار نظر او که کلاغ به تدریج منقرض شدن");

	record->setRefiningAttributeValue("price" , "1101");
	record->setRefiningAttributeValue("class" , "ج");

	record->setRecordBoost(90);
	index->addRecord(record,analyzer);

	record->clear();
	record->setPrimaryKey(1103);
	record->setSearchableAttributeValue(1, "واحد ضد چین");
	record->setSearchableAttributeValue(2,
			"مواد منفجره، وکیل مدافع جوزف آنتونلی مورد از دست دادن هرگز - و یا پشت سر هم از وجدان");

	record->setRefiningAttributeValue("price" , "1103");
	record->setRefiningAttributeValue("class" , "د");

	record->setRecordBoost(90);
	index->addRecord(record,analyzer);

	index->commit();
	index->save();

	delete schema;
	delete record;
	delete analyzer;
	delete index;
}



int main(){
	testPolish();
	cout << "test Polish passed" << endl;

	testPortuguese();
	cout << "test Portuguese passed" << endl;

	testRomanian();
	cout << "test Romanian passed" << endl;

	testRussian();
	cout << "test Russian passed" << endl;

	testSlovak();
	cout << "test Slovak passed" << endl;

	testSlovenian();
	cout << "test Slovenian passed" << endl;

	testSerbian();
	cout << "test Serbian passed" << endl;

	testSwedish();
	cout << "test Swedish passed" << endl;

	testThai();
	cout << "test Thai passed" << endl;

	testTurkish();
	cout << "test Turkish passed" << endl;

	testVietnamese();
	cout << "test Vietnamese passed" << endl;

	testHebrew();
	cout << "reading from right to left  test Hebrew passed" << endl;

	testUkrainian();
	cout << "test Ukrainian passed" << endl;

	testFarsi();
	cout << "reading from right to left  test Farsi passed" << endl;

	testKazakh();
	cout << "test Kazakh passed" << endl;

	// test Burmese
	testBurmese();
	cout << "test Chinese passed" << endl;

	// test PortugueseBrazil
	testPortugueseBrazil();
	cout << "test Portuguese-Brazil passed" << endl;

	// test Spanish-Latin
	testSpanishLatin();
	cout << "test Spanish-Latin passed" << endl;

	testSpanish();
	cout << "test Spanish passed" << endl;

	// test Arabic
	testArabic();
	cout << "reading from right to left  test Arabic passed" << endl;

	return 0;

}
