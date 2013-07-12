//$Id: Scalability_Test.cpp 3480 2013-06-19 08:00:34Z jiaying $

#include <jni.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <android/log.h>

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"
#include "MapSearchTestHelper.h"
#include "analyzer/StandardAnalyzer.h"

#define APPNAME "MyApp"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;

using namespace std;

const unsigned kMaxQueryNumber      = 5000;

const string kStrPrimaryKey ("primaryKey");
const string kStrAttribute ("description");

void setupSchema(Schema * schema){
    schema->setPrimaryKey(kStrPrimaryKey);
    schema->setSearchableAttribute(kStrAttribute, 2);
}

Indexer * createIndex(Analyzer* analyzer, Schema* schema, const string& indexPath){
    const unsigned kMergeEveryNSeconds  = 3000;
    const unsigned kMergeEveryMWrites   = 5000;
    IndexMetaData indexMetaData(new Cache(), kMergeEveryNSeconds, kMergeEveryMWrites, 
            indexPath, "");
    return Indexer::create( &indexMetaData, analyzer, schema);
}

int loadFile(const char* inputFile, Indexer * index, Schema* schema){
    Record * record = new Record(schema);
    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "load file");

    int lineCount = 0;
    ifstream fsDoc(inputFile);
    string line;
    while (getline( fsDoc, line)){
        stringstream lineStream (line);
        int icell = 0;
        string cell;
        while (getline( lineStream, cell, '^')){
            if(icell == 0){
                record->setPrimaryKey( cell.c_str());
            }else if( icell == 1){
                record->setSearchableAttributeValue(0, cell);
            }else{
                icell = 0;
                break;
            }
            icell++;
        }
        index->addRecord(record, 0);
        lineCount++;
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "line: %d", lineCount);
        record->clear();
    }
    delete record;
    index->commit();
    index->save();
    return lineCount;
}

int buildIndex(const char* inputFile, const char* indexPath) {
    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "buildindex");
	Schema *schema = Schema::create(srch2is::DefaultIndex);
    setupSchema( schema);
    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "setup schema");

    AnalyzerInternal *analyzer = new StandardAnalyzer( srch2::instantsearch::NO_STEMMER_NORMALIZER, "");

    Indexer *indexer = createIndex( analyzer, schema, string(indexPath));
    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "create index");

    int iDocs = loadFile(inputFile, indexer, schema);

    delete indexer;
    delete analyzer;
	delete schema;
    return iDocs;
}

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jstring Java_com_srch2_mobile_ndksearch_Srch2Lib_giveMe1024(JNIEnv* env,
		jobject javaThis){
	return env->NewStringUTF("I'm a string");
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jint Java_com_srch2_mobile_ndksearch_Srch2Lib_buildIndex(JNIEnv* env,
		jobject javaThis, jstring dataFile, jstring indexPath, jboolean isGeo) {
    const char *nativeStringDataFile = env->GetStringUTFChars( dataFile, NULL);
    const char *nativeStringIndexPath = env->GetStringUTFChars( indexPath, NULL);
	return buildIndex(nativeStringDataFile, nativeStringIndexPath);
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jstring Java_com_srch2_mobile_ndksearch_Srch2Lib_query(JNIEnv* env,
		jobject javaThis, jstring queryFile, jstring indexPath,
		jboolean isGeo) {
	int num = 1024;
	char str[100];
	sprintf(str, "Hello from query code! = %d", num);
	return env->NewStringUTF(str);
}
#ifdef __cplusplus
}
#endif

