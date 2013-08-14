
// $Id: StringEncoding_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

#include <instantsearch/Analyzer.h>
#include "operation/IndexerInternal.h"
#include <instantsearch/IndexSearcher.h>
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

void addSimpleRecords()
{
    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);
    schema->setPrimaryKey("article_id"); // integer, not searchable
    schema->setSearchableAttribute("article_id"); // convert id to searchable text
    schema->setSearchableAttribute("article_authors", 2); // searchable text
    schema->setSearchableAttribute("article_title", 7); // searchable text

    Record *record = new Record(schema);

    Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER, "");

    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");

    Indexer *index = Indexer::create(indexMetaData1, analyzer, schema);

    record->setPrimaryKey(1001);
    record->setSearchableAttributeValue("article_authors", "Tom Smith and Jack Lennon");
    record->setSearchableAttributeValue("article_title", "Come Yesterday Once More");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1002);
    record->setSearchableAttributeValue(1, "Jimi Hendrix");
    record->setSearchableAttributeValue(2, "Little wing");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    record->clear();
    record->setPrimaryKey(1003);
    record->setSearchableAttributeValue(1, "Tom Smith and Jack The Ripper");
    record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
    record->setRecordBoost(10);
    index->addRecord(record, 0);

    record->clear();
	record->setPrimaryKey(1100);
	//Finnish text
	record->setSearchableAttributeValue(1, "Eläintieteen ja behaviorismin lyhyt oppimäärä huonekalujen äänellä.Pipalukin rakkaus on yhtä suurta kuin hänen rintansa, hän luottaa vaimoonsa niin, että pelotta ojentaa jopa kyntensä tämän leikattavaksi – tehkääpä perässä! Mutta riittääkö se, jos vaimo hädintuskin uskaltautuu kurkistamaan ulos kaapista? Sivistynyt koiranomistaja voi käyttää mallina koiriaan, kun on kerrottava tiettyjä arkaluontoisia elämäntotuuksia jälkikasvulle: tähän tarjoutuu oiva tilaisuus, kun Kohmosen perheen Wendy-nartun sulhasehdokas tulee vähän tarpeettomankin reilusti esittäytymään. Mutta mikä sivu biologian kirjasta avataan?");
	//Russian text
	record->setSearchableAttributeValue(2, "Vse ljudi roždajutsya svobodnymi i ravnymi v svoem dostoinstve i pravakh. Oni nadeleny razumom i sovest'ju i dolžny postupat' v otnošenii drug druga v dukhe bratstva");
	record->setRecordBoost(90);
	index->addRecord(record, 0);


	record->clear();
	record->setPrimaryKey(1101);
	//Russian text
	record->setSearchableAttributeValue(1, "Vse ljudi roždajutsya svobodnymi i ravnymi v svoem dostoinstve i pravakh. Oni nadeleny razumom i sovest'ju i dolžny postupat' v otnošenii drug druga v dukhe bratstva");
	//German text
	record->setSearchableAttributeValue(2, "Zahlreiche Pressemitteilungen meldeten am 21. Februar 2006, dass Mladić ");
	record->setRecordBoost(90);
	index->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(1102);
	//French text
	record->setSearchableAttributeValue(1, "Ce texte est écritécrit en français et je devrais le voir en anglais.");
	//Spanish text
	record->setSearchableAttributeValue(2, "Este texto está escrito en francés y yo lo veo en Inglés.");
	record->setRecordBoost(90);
	index->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(1103);
	//French text
	record->setSearchableAttributeValue(1, "AVERYUNIQUEWORD Ce texte est écrit en français et je devrais le voir en anglais.");
	//Estonia text
	record->setSearchableAttributeValue(2, " Ратко Младић  Ce Texte est écrit EN français JE да devrais Le Voir EN Anglais.");
	record->setRecordBoost(90);
	index->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(1104);
	//Finnish text
	record->setSearchableAttributeValue(1, "Eläintieteen ja behaviorismin lyhyt oppimäärä huonekalujen äänellä.Pipalukin rakkaus on yhtä suurta kuin hänen rintansa, hän luottaa vaimoonsa niin, että pelotta ojentaa jopa kyntensä tämän leikattavaksi – tehkääpä perässä! Mutta riittääkö se, jos vaimo hädintuskin uskaltautuu kurkistamaan ulos kaapista? Sivistynyt koiranomistaja voi käyttää mallina koiriaan, kun on kerrottava tiettyjä arkaluontoisia elämäntotuuksia jälkikasvulle: tähän tarjoutuu oiva tilaisuus, kun Kohmosen perheen Wendy-nartun sulhasehdokas tulee vähän tarpeettomankin reilusti esittäytymään. Mutta mikä sivu biologian kirjasta avataan?");
	//Russian text
	record->setSearchableAttributeValue(2, "Все люди рождаются свободными и равными в своем достоинстве и правах. Они наделены разумом и совестью и должны поступать в отношении друг друга в духе братства.");
	record->setRecordBoost(90);
	index->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(1105);
	//French text
	record->setSearchableAttributeValue(1, "Ce texte est écrit en français et je devrais le voir en anglais.");
	//Hebrew text
	record->setSearchableAttributeValue(2, "טקסט זה נכתב בצרפתית ואני צריך לראות את זה באנגלית.");
	record->setRecordBoost(90);
	index->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(1106);
	//Chinese text
	record->setSearchableAttributeValue(1, "我爱北京天安门");
	//Chinese text
	record->setSearchableAttributeValue(2, "天安门上太阳升");
	record->setRecordBoost(90);
	index->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(1107);
	//注音 text
	record->setSearchableAttributeValue(1, "ㄓㄨˋ 丨ㄣ ㄈㄨˊ ㄏㄠˋ");
	//注音 text
	record->setSearchableAttributeValue(2, "ㄅㄚ ㄕㄢ ㄕㄤˋ, ㄕㄟˊ ㄉ丨ˋ ㄇㄚˇ");
	record->setRecordBoost(90);
	index->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(1108);
	//注音 text
	record->setSearchableAttributeValue(1, "ㄅㄨˋ　ㄊ｜ㄥ　ㄌㄠˇ　ㄖㄣˊ　｜ㄢˊ　ㄔ　ㄎㄨㄟ　ㄗㄞˋ　｜ㄢˇ　ㄑ｜ㄢˊ");
	//注音 text
	record->setSearchableAttributeValue(2, "ㄅㄨˋ　ㄍㄢ　ㄅㄨˋ　ㄐ｜ㄥˋ　ㄔ　˙ㄌㄜ　ㄇㄟˊ　ㄅ｜ㄥˋ");
	record->setRecordBoost(90);
	index->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(1109);
	//Chinese text
	record->setSearchableAttributeValue(1, "京广铁路联通南北");
	//Chinese text
	record->setSearchableAttributeValue(2, "好好学习，天天向上");
	record->setRecordBoost(90);
	index->addRecord(record, 0);

    index->commit();
    index->save();

    delete schema;
    delete record;
    delete analyzer;
    delete index;
}

void addSimpleRecordsToDBLP()
{
	//Build the dblp test index.
	buildIndex(DBLP_INDEX_DIR);

	// Create an index writer
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, DBLP_INDEX_DIR, "", "");
	Indexer *indexer = Indexer::load(indexMetaData1);

	Record *record = new Record(indexer->getSchema());

	// Add more records with different string encoding,i.e (Foreign text)
	record->setPrimaryKey(10001001);
	record->setSearchableAttributeValue(1, "Tom Smith and Jack Lennon");
	record->setSearchableAttributeValue(2, "Come Yesterday Once More");
	record->setSearchableAttributeValue(3, "Come Yesterday Once More");
	record->setSearchableAttributeValue(4, "Come Yesterday Once More");
	record->setRecordBoost(90);
	indexer->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(10001002);
	record->setSearchableAttributeValue(1, "Jimi Hendrix");
	record->setSearchableAttributeValue(2, "Little wing");
	record->setSearchableAttributeValue(3, "Jimi Hendrix");
	record->setSearchableAttributeValue(4, "Little wing");
	record->setRecordBoost(90);
	indexer->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(10001003);
	record->setSearchableAttributeValue(1, "Tom Smith and Jack The Ripper");
	record->setSearchableAttributeValue(2, "Come Tomorrow Two More");
	record->setSearchableAttributeValue(3, "Tom Smith and Jack The Ripper");
	record->setSearchableAttributeValue(4, "Come Tomorrow Two More");
	record->setRecordBoost(10);
	indexer->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(10001100);
	//Finnish text
	record->setSearchableAttributeValue(1, "Eläintieteen ja behaviorismin lyhyt oppimäärä huonekalujen äänellä.Pipalukin rakkaus on yhtä suurta kuin hänen rintansa, hän luottaa vaimoonsa niin, että pelotta ojentaa jopa kyntensä tämän leikattavaksi – tehkääpä perässä! Mutta riittääkö se, jos vaimo hädintuskin uskaltautuu kurkistamaan ulos kaapista? Sivistynyt koiranomistaja voi käyttää mallina koiriaan, kun on kerrottava tiettyjä arkaluontoisia elämäntotuuksia jälkikasvulle: tähän tarjoutuu oiva tilaisuus, kun Kohmosen perheen Wendy-nartun sulhasehdokas tulee vähän tarpeettomankin reilusti esittäytymään. Mutta mikä sivu biologian kirjasta avataan?");
	//Russian text
	record->setSearchableAttributeValue(2, "Vse ljudi roždajutsya svobodnymi i ravnymi v svoem dostoinstve i pravakh. Oni nadeleny razumom i sovest'ju i dolžny postupat' v otnošenii drug druga v dukhe bratstva");
	//Finnish text
	record->setSearchableAttributeValue(3, "Eläintieteen ja behaviorismin lyhyt oppimäärä huonekalujen äänellä.Pipalukin rakkaus on yhtä suurta kuin hänen rintansa, hän luottaa vaimoonsa niin, että pelotta ojentaa jopa kyntensä tämän leikattavaksi – tehkääpä perässä! Mutta riittääkö se, jos vaimo hädintuskin uskaltautuu kurkistamaan ulos kaapista? Sivistynyt koiranomistaja voi käyttää mallina koiriaan, kun on kerrottava tiettyjä arkaluontoisia elämäntotuuksia jälkikasvulle: tähän tarjoutuu oiva tilaisuus, kun Kohmosen perheen Wendy-nartun sulhasehdokas tulee vähän tarpeettomankin reilusti esittäytymään. Mutta mikä sivu biologian kirjasta avataan?");
	//Russian text
	record->setSearchableAttributeValue(4, "Vse ljudi roždajutsya svobodnymi i ravnymi v svoem dostoinstve i pravakh. Oni nadeleny razumom i sovest'ju i dolžny postupat' v otnošenii drug druga v dukhe bratstva");
	record->setRecordBoost(90);
	indexer->addRecord(record, 0);


	record->clear();
	record->setPrimaryKey(10001101);
	//Russian text
	record->setSearchableAttributeValue(1, "Vse ljudi roždajutsya svobodnymi i ravnymi v svoem dostoinstve i pravakh. Oni nadeleny razumom i sovest'ju i dolžny postupat' v otnošenii drug druga v dukhe bratstva");
	//German text
	record->setSearchableAttributeValue(2, "Zahlreiche Pressemitteilungen meldeten am 21. Februar 2006, dass Mladić ");
	//Russian text
	record->setSearchableAttributeValue(3, "Vse ljudi roždajutsya svobodnymi i ravnymi v svoem dostoinstve i pravakh. Oni nadeleny razumom i sovest'ju i dolžny postupat' v otnošenii drug druga v dukhe bratstva");
	//German text
	record->setSearchableAttributeValue(4, "Zahlreiche Pressemitteilungen meldeten am 21. Februar 2006, dass Mladić ");
	record->setRecordBoost(90);
	indexer->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(10001102);
	//French text
	record->setSearchableAttributeValue(1, "Ce texte est écritécrit en français et je devrais le voir en anglais.");
	//Spanish text
	record->setSearchableAttributeValue(2, "Este texto está escrito en francés y yo lo veo en Inglés.");
	//French text
	record->setSearchableAttributeValue(3, "Ce texte est écritécrit en français et je devrais le voir en anglais.");
	//Spanish text
	record->setSearchableAttributeValue(4, "Este texto está escrito en francés y yo lo veo en Inglés.");
	record->setRecordBoost(90);
	indexer->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(10001103);
	//French text
	record->setSearchableAttributeValue(1, "AVERYUNIQUEWORD Ce texte est écritécrit en français et je devrais le voir en anglais.");
	//Estonia text
	record->setSearchableAttributeValue(2, " Ратко Младић  Ce Texte est écritécrit EN français JE да devrais Le Voir EN Anglais.");
	//French text
	record->setSearchableAttributeValue(3, "Ce texte est écritécrit en français et je devrais le voir en anglais.");
	//Estonia text
	record->setSearchableAttributeValue(4, " Ратко Младић  Ce Texte est écritécrit EN français JE да devrais Le Voir EN Anglais.");
	record->setRecordBoost(90);
	indexer->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(10001104);
	//Finnish text
	record->setSearchableAttributeValue(1, "Eläintieteen ja behaviorismin lyhyt oppimäärä huonekalujen äänellä.Pipalukin rakkaus on yhtä suurta kuin hänen rintansa, hän luottaa vaimoonsa niin, että pelotta ojentaa jopa kyntensä tämän leikattavaksi – tehkääpä perässä! Mutta riittääkö se, jos vaimo hädintuskin uskaltautuu kurkistamaan ulos kaapista? Sivistynyt koiranomistaja voi käyttää mallina koiriaan, kun on kerrottava tiettyjä arkaluontoisia elämäntotuuksia jälkikasvulle: tähän tarjoutuu oiva tilaisuus, kun Kohmosen perheen Wendy-nartun sulhasehdokas tulee vähän tarpeettomankin reilusti esittäytymään. Mutta mikä sivu biologian kirjasta avataan?");
	//Russian text
	record->setSearchableAttributeValue(2, "Все люди рождаются свободными и равными в своем достоинстве и правах. Они наделены разумом и совестью и должны поступать в отношении друг друга в духе братства.");
	//Finnish text
	record->setSearchableAttributeValue(3, "Eläintieteen ja behaviorismin lyhyt oppimäärä huonekalujen äänellä.Pipalukin rakkaus on yhtä suurta kuin hänen rintansa, hän luottaa vaimoonsa niin, että pelotta ojentaa jopa kyntensä tämän leikattavaksi – tehkääpä perässä! Mutta riittääkö se, jos vaimo hädintuskin uskaltautuu kurkistamaan ulos kaapista? Sivistynyt koiranomistaja voi käyttää mallina koiriaan, kun on kerrottava tiettyjä arkaluontoisia elämäntotuuksia jälkikasvulle: tähän tarjoutuu oiva tilaisuus, kun Kohmosen perheen Wendy-nartun sulhasehdokas tulee vähän tarpeettomankin reilusti esittäytymään. Mutta mikä sivu biologian kirjasta avataan?");
	//Russian text
	record->setSearchableAttributeValue(4, "Все люди рождаются свободными и равными в своем достоинстве и правах. Они наделены разумом и совестью и должны поступать в отношении друг друга в духе братства.");
	record->setRecordBoost(90);
	indexer->addRecord(record, 0);

	record->clear();
	record->setPrimaryKey(10001105);
	//French text
	record->setSearchableAttributeValue(1, "Ce texte est écritécrit en français et je devrais le voir en anglais.");
	//Hebrew text
	record->setSearchableAttributeValue(2, "טקסט זה נכתב בצרפתית ואני צריך לראות את זה באנגלית.");
	//French text
	record->setSearchableAttributeValue(3, "Ce texte est écritécrit en français et je devrais le voir en anglais.");
	//Hebrew text
	record->setSearchableAttributeValue(4, "טקסט זה נכתב בצרפתית ואני צריך לראות את זה באנגלית.");
	record->setRecordBoost(90);
	indexer->addRecord(record, 0);

	indexer->commit();
	indexer->save();

	delete record;
	delete indexer;
}


//Create Index "A". Deserialise "A". Update Index "A". Search "A". Serialize "A"
void test1()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");

    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Query: "tom", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 2 , recordIds) == true);
    }
    //Query: "jimi", hit -> 1002
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 1 , recordIds) == true);
    }
    //Query: "北京", hit -> 1106 1109
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1106);
		recordIds.push_back(1109);
		ASSERT ( ping(analyzer, indexSearcher, "北京" , 2 , recordIds) == true);
	}

	//Query: "天上", hit -> 1106 1109
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1106);
		recordIds.push_back(1109);
		ASSERT ( ping(analyzer, indexSearcher, "天上" , 2 , recordIds) == true);
	}

	//Query: "roždajutsya svobodnymi" hit -> 1100 1101
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1100);
		recordIds.push_back(1101);
		ASSERT ( ping(analyzer, indexSearcher, "roždajutsya svobodnymi" , 2 , recordIds) == true);
	}
	//Query: "ㄈㄨˊ ㄏㄠˋ" hit -> 1107
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1107);
		ASSERT ( ping(analyzer, indexSearcher, "ㄈㄨˊ ㄏㄠˋ" , 1 , recordIds) == true);
	}

    //Update Index
    Record *record = new Record(index->getSchema());
    record->setPrimaryKey(1999);
    record->setSearchableAttributeValue(1, "steve jobs tom");
    record->setSearchableAttributeValue(2, "digital magician");
    record->setRecordBoost(90);
    index->addRecord(record, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "smith", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "smith" , 2 , recordIds) == true);
    }

    //Query: "jobs", hit -> 1999
    {
    	vector<unsigned> recordIds;
		recordIds.push_back(1999);
		recordIds.push_back(1100);
		recordIds.push_back(1104);
		ASSERT ( ping(analyzer, indexSearcher, "jobs" , 3 , recordIds) == true);
    }

    //indexer->print_index();

    //Query: "tom", hits -> 1999, 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 2 , recordIds) == false);
    }

    index->commit();
    index->save(INDEX_DIR);

    //Query: "tom", hits -> 1999, 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    (void)analyzer;
    delete indexSearcher;
    delete index;
}

//Deserialise "A" from test1. Update Index "A". Search "A" and then, save "A"
//Testing Indexer::load(...) and Indexer::save(...)
void test2()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");

    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Query: "smith", hits -> 1001, 1003
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        ASSERT ( ping(analyzer, indexSearcher, "smith" , 2 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    //Query: "jobs", hit -> 1999
    {
    	vector<unsigned> recordIds;
		recordIds.push_back(1999);
		recordIds.push_back(1100);
		recordIds.push_back(1104);
		ASSERT ( ping(analyzer, indexSearcher, "jobs" , 3 , recordIds) == true);
    }

    //Update a Deserialised Index
    Record *record = new Record(index->getSchema());
    record->setPrimaryKey(1998);
    record->setSearchableAttributeValue(1, "jimi hendrix");
    record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    record->setRecordBoost(100);
    index->addRecord(record, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "smith", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 2 , recordIds) == true);
    }

    //Query: "jobs", hits -> 1998 , 1999
    {
    	vector<unsigned> recordIds;
		recordIds.push_back(1999);
		recordIds.push_back(1998);
		recordIds.push_back(1100);
		recordIds.push_back(1104);
		ASSERT ( ping(analyzer, indexSearcher, "jobs" , 4 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    index->commit();
    index->save(INDEX_DIR);

    (void)analyzer;
    delete indexSearcher;
    delete index;

}

//Deserialise Index "A". Add duplicate records. Test. Serialise
void test3()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");

    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Update a Deserialised Index with Duplicate
    Record *record = new Record(index->getSchema());
    record->setPrimaryKey(1998);
    record->setSearchableAttributeValue(1, "jimi hendrix");// THis record must not be added to index
    record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    record->setRecordBoost(100);
    index->addRecord(record, 0);

    //Update a Deserialised Index with Duplicate
    record->clear();
    record->setPrimaryKey(1998);
    record->setSearchableAttributeValue(1, "jimi hendrix");// THis record must not be added to index
    record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    record->setRecordBoost(100);
    index->addRecord(record, 0);

    //Update a Deserialised Index with Duplicate
    record->clear();
    record->setPrimaryKey(1998);
    record->setSearchableAttributeValue(1, "tom tom tom tom"); // THis record must not be added to index
    record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    record->setRecordBoost(100);
    index->addRecord(record, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "smith", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 2 , recordIds) == true);
    }

    //Query: "jobs", hits -> 1998 , 1999
    {
    	vector<unsigned> recordIds;
		recordIds.push_back(1999);
		recordIds.push_back(1998);
		recordIds.push_back(1100);
		recordIds.push_back(1104);
		ASSERT ( ping(analyzer, indexSearcher, "jobs" , 4 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    index->commit();
    index->save(INDEX_DIR);

    delete indexSearcher;
    (void)analyzer;
    delete index;
}


//Deserialise Index "A". Delete a record. Test. Serialise
void test4()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");

    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    std::string recordId = "1998";
    index->deleteRecord(recordId, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "smith", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        //recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 1 , recordIds) == true);
    }

    //Query: "jobs", hits -> 1998 , 1999
    {
    	vector<unsigned> recordIds;
		recordIds.push_back(1999);
		recordIds.push_back(1100);
		recordIds.push_back(1104);
		ASSERT ( ping(analyzer, indexSearcher, "jobs" , 3 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    index->save(INDEX_DIR);

    //delete analyzer;
    delete indexSearcher;
    (void)analyzer;
    delete index;
}

//Deserialise Index "A". Add the deleted record in test3(). Test. Serialise
void test5()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");

    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Update a Deserialised Index with Duplicate
    Record *record = new Record(index->getSchema());
    record->setPrimaryKey(1998);
    record->setSearchableAttributeValue(1, "jimi hendrix");// THis record must not be added to index
    record->setSearchableAttributeValue(2, "steve jobs shot the sheriff");
    record->setRecordBoost(100);
    index->addRecord(record, 0);

    sleep(mergeEveryNSeconds+1);

    delete indexSearcher;
    indexSearcher = IndexSearcher::create(index);

    //Query: "jimi", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jimi" , 2 , recordIds) == true);
    }

    //Query: "jobs", hits -> 1998, 1999
    {
    	vector<unsigned> recordIds;
		recordIds.push_back(1999);
		recordIds.push_back(1998);
		recordIds.push_back(1100);
		recordIds.push_back(1104);
		ASSERT ( ping(analyzer, indexSearcher, "jobs" , 4 , recordIds) == true);
    }

    //Query: "tom", hits -> 1001, 1003 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1001);
        recordIds.push_back(1003);
        recordIds.push_back(1999);
        ASSERT ( ping(analyzer, indexSearcher, "tom" , 3 , recordIds) == true);
    }

    index->commit();
    index->save(INDEX_DIR);

    delete indexSearcher;
    (void)analyzer;
    delete index;
}

//Test Fuzzy Query and Edit Distance
void test6()
{
    // create an index searcher
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");

    Indexer *index = Indexer::load(indexMetaData1);
    IndexSearcher *indexSearcher = IndexSearcher::create(index);
    const Analyzer *analyzer = index->getAnalyzer();

    //Query: "jemi", hits -> 1002, 1998
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1002);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jemi" , 2 , recordIds) == true);
    }

    //Query: "jobsi", hits -> 1998 , 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1999);
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "jobsi" , 2 , recordIds) == true);
    }

    //Query: "shiref", hits -> 1999
    {
        vector<unsigned> recordIds;
        recordIds.push_back(1998);
        ASSERT ( ping(analyzer, indexSearcher, "shiref" , 1 , recordIds) == true);
    }

    //delete analyzer;
    (void)analyzer;
    delete indexSearcher;
    delete index;
}

//Test API 2 Queries - AttributeFilter Queries
void test7()
{
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");

	Indexer *index = Indexer::load(indexMetaData1);
	IndexSearcher *indexSearcher = IndexSearcher::create(index);
	const Analyzer *analyzer = index->getAnalyzer();
	// STRING ENCODING TESTS
	//Query: "écrit", hits -> 1105, 1102, 1103
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1102);
		recordIds.push_back(1103);
		recordIds.push_back(1105);
		ASSERT ( ping(analyzer, indexSearcher, "écrit" , 3 , recordIds) == true);
	}

	//Query: "écrit", hits -> 1104
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1104);
		ASSERT ( ping(analyzer, indexSearcher, "рождаются" , 1 , recordIds) == true);
	}

	//Query: "באנגלית", hits -> 1105
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1105);
		ASSERT ( ping(analyzer, indexSearcher, "  אנגלית a" , 1,  recordIds) == true);
	}

	//Query: "äänellä", hits -> 1104
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1100);
		recordIds.push_back(1104);
		ASSERT ( ping(analyzer, indexSearcher, "äänellä" , 2 , recordIds) == true);
	}

	//Query: "äänellä", hits -> 1104
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1100);
		recordIds.push_back(1104);
		ASSERT ( ping(analyzer, indexSearcher, "äanellä" , 2 , recordIds) == true);
	}

	//Query: "AVERYUNIQUEWORD", hits -> 1102
	{
		vector<unsigned> recordIds;
		recordIds.push_back(1103);
		ASSERT ( ping(analyzer, indexSearcher, "AVERYUNIQUEWORD" , 1 , recordIds) == true);
	}

	//delete analyzer;
	(void)analyzer;
	delete indexSearcher;
	delete index;
}

void test7_DBLP(string INDEX_DIR)
{
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, INDEX_DIR, "", "");

	Indexer *index = Indexer::load(indexMetaData1);
	IndexSearcher *indexSearcher = IndexSearcher::create(index);
	const Analyzer *analyzer = index->getAnalyzer();

	cout << "STRING ENCODING TESTS" << INDEX_DIR << endl;
	// STRING ENCODING TESTS
	//Query: "écritécrit", hits -> 1105, 1102, 1103
	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001102);
		recordIds.push_back(10001105);
		recordIds.push_back(10001103);
		ASSERT ( ping(analyzer, indexSearcher, "écritécrit" , 3 , recordIds) == true);
	}

	//Query: "рождаются", hits -> 1104
	/*{
		vector<unsigned> recordIds;
		recordIds.push_back(10001104);
		ASSERT ( ping(analyzer, indexSearcher, "рождаются" , 1 , recordIds) == true);
	}*/

	//Query: "באנגלית", hits -> 1105
	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001105);
		ASSERT ( ping(analyzer, indexSearcher, "  אנגלית+a" , 1,  recordIds) == true);
	}

	//Query: "äänellä", hits -> 1104
	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001100);
		recordIds.push_back(10001104);
		ASSERT ( ping(analyzer, indexSearcher, "äänellä" , 2 , recordIds) == true);
	}

	//Query: "AVERYUNIQUEWORD", hits -> 1102
	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001103);
		ASSERT ( ping(analyzer, indexSearcher, "AVERYUNIQUEWORD" , 1 , recordIds) == true);
	}

	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001002);
		ASSERT ( ping(analyzer, indexSearcher, "Jimi+Hendrix" , 1 , recordIds) == true);
	}

	// DBLP contains many records with keyword "efficient". S0, we test for negation.
	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001103);
		ASSERT ( ping(analyzer, indexSearcher, "efficient" , 0 , recordIds) == false);
	}

	//delete analyzer;
	(void)analyzer;
	delete indexSearcher;
	delete index;
}

void testForSmallDatasetWithDifferentEncoding()
{
	cout << INDEX_DIR << endl;
	addSimpleRecords();
	//Create Index "A". Deserialise "A". Update Index "A". Search "A". Serialize "A"
	// Testing IndexerInternal constructor
	test1();
	cout << "test1 passed" << endl;

	//Deserialise "A" from test1. Update Index "A". Search "A" and then, save "A"
	//Testing Indexer::load(...) and Indexer::save(...)
	test2();
	cout << "test2 passed" << endl;

	//Deserialise Index "A". Add duplicate records. Test. Serialise
	test3();
	cout << "test3 passed" << endl;

	//Deserialise Index "A". Delete a record. Test. Serialise
	test4();
	cout << "test4 passed" << endl;

	//Deserialise Index "A". Add the deleted record in test3(). Test. Serialise
	test5();
	cout << "test5 passed" << endl;

	//Test Fuzzy Query and Edit Distance
	test6();
	cout << "test6 passed" << endl;

	//Test API 2 Queries - AttributeFilter Queries
	test7();
	cout << "test7 passed" << endl;

	cout<<"String Encoding tests Succesful!!"<<endl;
}

void test7_DBLP()
{
	// create an index searcher
	// create an index searcher
	unsigned mergeEveryNSeconds = 3;
	unsigned mergeEveryMWrites = 5;
	IndexMetaData *indexMetaData1 = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, DBLP_INDEX_DIR, "", "");

	Indexer *index = Indexer::load(indexMetaData1);
	IndexSearcher *indexSearcher = IndexSearcher::create(index);
	const Analyzer *analyzer = index->getAnalyzer();

	cout << "STRING ENCODING TESTS" << INDEX_DIR << endl;
	// STRING ENCODING TESTS
	//Query: "écritécrit", hits -> 1105, 1102, 1103
	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001102);
		recordIds.push_back(10001103);
		recordIds.push_back(10001105);
		ASSERT ( ping(analyzer, indexSearcher, "écritécrit" , 3 , recordIds) == true);
	}

	//Query: "рождаются", hits -> 1104
	/*{
		vector<unsigned> recordIds;
		recordIds.push_back(10001104);
		ASSERT ( ping(analyzer, indexSearcher, "рождаются" , 1 , recordIds) == true);
	}*/

	//Query: "באנגלית", hits -> 1105
	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001105);
		ASSERT ( ping(analyzer, indexSearcher, "  אנגלית+a" , 1,  recordIds) == true);
	}

	//Query: "äänellä", hits -> 1104
	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001100);
		recordIds.push_back(10001104);
		ASSERT ( ping(analyzer, indexSearcher, "äänellä" , 2 , recordIds) == true);
	}

	//Query: "AVERYUNIQUEWORD", hits -> 1102
	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001103);
		ASSERT ( ping(analyzer, indexSearcher, "AVERYUNIQUEWORD" , 1 , recordIds) == true);
	}

	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001002);
		ASSERT ( ping(analyzer, indexSearcher, "Jimi+Hendrix" , 1 , recordIds) == true);
	}

	// DBLP contains many records with keyword "efficient". S0, we test for negation.
	{
		vector<unsigned> recordIds;
		recordIds.push_back(10001103);
		ASSERT ( ping(analyzer, indexSearcher, "efficient" , 0 , recordIds) == false);
	}

	(void)analyzer;
	//delete analyzer;
	delete index;
	delete indexSearcher;
}

void testForDBLPDatasetWithDifferentEncoding()
{
	addSimpleRecordsToDBLP();

	// Testing IndexerInternal constructor
	//test1(DBLP_INDEX_DIR);//BUG - Due to Cache Hit

	//Test String Encoding
	test7_DBLP();

}


int main(int argc, char **argv)
{

	testForSmallDatasetWithDifferentEncoding();
	testForDBLPDatasetWithDifferentEncoding();

    return 0;
}
