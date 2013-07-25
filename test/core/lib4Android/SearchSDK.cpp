#include "Srch2Android.h"
#include "util/Logger.h"

using namespace srch2::sdk;
using srch2::util::Logger;

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void Java_com_srch2_mobile_ndksearch_Srch2Lib_setLoggerFile(
		JNIEnv* env, jobject javaThis, jstring logfile) {
	const char *nativeStringLogFile = env->GetStringUTFChars(logfile, NULL);
	FILE* fpLog = fopen(nativeStringLogFile, "a");
	Logger::setOutputFile(fpLog);
	env->ReleaseStringUTFChars(logfile, nativeStringLogFile);
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jlong Java_com_srch2_mobile_ndksearch_Srch2Lib_createIndex(
		JNIEnv* env, jobject javaThis, jstring indexDir, jboolean isGeo) {
	Logger::console("createIndex");
	const char *nativeStringIndexPath = env->GetStringUTFChars(indexDir, NULL);

	string strIndexPath(nativeStringIndexPath);

	srch2::instantsearch::TermType termType = TERM_TYPE_PREFIX;

	Logger::console("Save index to %s", strIndexPath.c_str());

	Indexer* indexer = createIndex(strIndexPath, isGeo);

	env->ReleaseStringUTFChars(indexDir, nativeStringIndexPath);
	long ptr = (long) indexer;
	Logger::console("createIndex done");
	return ptr;
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void Java_com_srch2_mobile_ndksearch_Srch2Lib_addRecord(JNIEnv* env,
		jobject javaThis, jlong indexPtr, jstring key, jstring value,
		jboolean keepInMemory) {

	Indexer* indexer = (Indexer*) indexPtr;
	const char *nativeKey = env->GetStringUTFChars(key, NULL);
	const char *nativeVal = env->GetStringUTFChars(value, NULL);

	Logger::console("addRecord %s:%s", nativeKey, nativeVal);

	string cstrkey(nativeKey);
	string cstrval(nativeVal);

	addRecord(indexer, cstrkey, cstrval, keepInMemory);

	env->ReleaseStringUTFChars(key, nativeKey);
	env->ReleaseStringUTFChars(value, nativeVal);
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jlong Java_com_srch2_mobile_ndksearch_Srch2Lib_createIndexByFile(
		JNIEnv* env, jobject javaThis, jstring testFile, jstring indexDir,
		jint lineLimit, jboolean isGeo) {
	Logger::console("createIndex");
	const char *nativeStringDataFile = env->GetStringUTFChars(testFile, NULL);
	const char *nativeStringIndexPath = env->GetStringUTFChars(indexDir, NULL);

	string strIndexPath(nativeStringIndexPath);
	string strTestFile(nativeStringDataFile);

	srch2::instantsearch::TermType termType = TERM_TYPE_PREFIX;

	Logger::console("Read data from %s", strTestFile.c_str());
	Logger::console("Save index to %s", strIndexPath.c_str());

	Indexer* indexer = createIndex(strTestFile, strIndexPath, lineLimit, isGeo);

	env->ReleaseStringUTFChars(testFile, nativeStringDataFile);
	env->ReleaseStringUTFChars(indexDir, nativeStringIndexPath);
	long ptr = (long) indexer;
	Logger::console("createIndex done");
	return ptr;
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jlong Java_com_srch2_mobile_ndksearch_Srch2Lib_loadIndex(JNIEnv* env,
		jobject javaThis, jstring indexDir) {
	Logger::console("loadIndex");
	const char *nativeStringIndexPath = env->GetStringUTFChars(indexDir, NULL);
	string strIndexDir(nativeStringIndexPath);

	Logger::console("Load index from %s", strIndexDir.c_str());
	Indexer* indexer = loadIndex(strIndexDir);
	env->ReleaseStringUTFChars(indexDir, nativeStringIndexPath);
	long ptr = (long) indexer;
	Logger::console("loadIndex done");
	return ptr;
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void Java_com_srch2_mobile_ndksearch_Srch2Lib_saveIndex(JNIEnv* env,
		jobject javaThis, jlong ptr) {
	Logger::console("saveIndex");
	Indexer* index = (Indexer*) ptr;
	saveIndex(index);
	Logger::console("saveIndex done");
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void Java_com_srch2_mobile_ndksearch_Srch2Lib_commitIndex(JNIEnv* env,
		jobject javaThis, jlong ptr) {
	Logger::console("commitIndex");
	Indexer* index = (Indexer*) ptr;
	commitIndex(index);
	Logger::console("commitIndex done");
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jstring Java_com_srch2_mobile_ndksearch_Srch2Lib_queryRaw(JNIEnv* env,
		jobject javaThis, jlong indexPtr, jstring queryStr, jboolean isGeo) {

	const char *nativeStringQuery = env->GetStringUTFChars(queryStr, NULL);
	string queryString(nativeStringQuery);
	Logger::console("query:%s", nativeStringQuery);
	Indexer* indexer = (Indexer*) indexPtr;
	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
	const Analyzer *analyzer = indexer->getAnalyzer();

	QueryResults* queryResults = query(analyzer, indexSearcher, queryString, 2,
			srch2::instantsearch::TERM_TYPE_PREFIX);

	string result = printQueryResult(queryResults, indexer);
	jstring jstr = env->NewStringUTF(result.c_str());
	env->ReleaseStringUTFChars(queryStr, nativeStringQuery);
	return jstr;
}
#ifdef __cplusplus
}
#endif

vector<string> matchedKeywords;
vector<unsigned> editDistances;
#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jobject Java_com_srch2_mobile_ndksearch_Srch2Lib_query(JNIEnv* env,
		jobject javaThis, jlong indexPtr, jstring queryStr, jboolean isGeo) {

	const char *nativeStringQuery = env->GetStringUTFChars(queryStr, NULL);
	string queryString(nativeStringQuery);
	Logger::console("query:%s", nativeStringQuery);
	Indexer* indexer = (Indexer*) indexPtr;
	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
	const Analyzer *analyzer = indexer->getAnalyzer();

	QueryResults* queryResults = query(analyzer, indexSearcher, queryString, 2,
			srch2::instantsearch::TERM_TYPE_PREFIX);

	// Find java ArrayList
	jclass clsArrayList = env->FindClass("java/util/ArrayList");
	jmethodID constructor = env->GetMethodID(clsArrayList, "<init>", "(I)V");
	Logger::console("arrayListConstructor: %d", constructor);
	jmethodID arrayListAdd = env->GetMethodID(clsArrayList, "add",
			"(Ljava/lang/Object;)Z");
	Logger::console("arrayListAdd: %d", arrayListAdd);

	int count = queryResults->getNumberOfResults();
	jobject objArrayList = env->NewObject(clsArrayList, constructor, count);

	// Find com.srch2.mobile.ndksearch.Hit
	jclass clsHit = env->FindClass("com/srch2/mobile/ndksearch/Hit");
	Logger::console("classHit: %d", clsHit);
	//public Hit(float score, String record, String[] keywords, int[] eds)
	jmethodID constructorHit = env->GetMethodID(clsHit, "<init>",
			"(FLjava/lang/String;[Ljava/lang/String;[I)V");
	Logger::console("classHit constructor: %d", constructorHit);

	// Foreach queryResult
	for (int i = 0; i < count; i++) {
		float score = queryResults->getResultScore(i);
		string record = queryResults->getInMemoryRecordString(i);
		queryResults->getMatchingKeywords(i, matchedKeywords);
		queryResults->getEditDistances(i, editDistances);

		// create record string
		jstring jstrRecord = env->NewStringUTF(record.c_str());
		jstring jstrEmpty = env->NewStringUTF("");
		// create keywords array
		int size = matchedKeywords.size();
		jobjectArray jstrArray = (jobjectArray) env->NewObjectArray(size,
				env->FindClass("java/lang/String"), jstrEmpty);
		for (int j = 0; j < size; j++) {
			jstring jstrMatch = env->NewStringUTF(matchedKeywords[j].c_str());
			env->SetObjectArrayElement(jstrArray, j, jstrMatch);
			env->DeleteLocalRef(jstrMatch);
		}

		// create edit distance array
		jintArray jintArray = env->NewIntArray(size);
		jint* eds = new jint[size];
		for (int j = 0; j < size; j++) {
			eds[j] = editDistances[j];
		}
		env->SetIntArrayRegion(jintArray, 0, size, eds);
		delete[] eds;

		jobject hit = env->NewObject(clsHit, constructorHit, score, jstrRecord,
				jstrArray, jintArray);

		bool isAdded = env->CallBooleanMethod(objArrayList, arrayListAdd, hit);
		Logger::console("set %d isAdded:%d", i, isAdded);
		env->DeleteLocalRef(jstrRecord);
		env->DeleteLocalRef(jstrArray);
		env->DeleteLocalRef(jintArray);
		env->DeleteLocalRef(hit);
		env->DeleteLocalRef(jstrEmpty);
	}

	env->DeleteLocalRef(clsArrayList);
	env->DeleteLocalRef(clsHit);
	env->DeleteLocalRef(objArrayList);
	Logger::console("MemUsage:%d", getRAMUsageValue());
	delete queryResults;
	delete indexSearcher;
	env->ReleaseStringUTFChars(queryStr, nativeStringQuery);
	return objArrayList;
}
#ifdef __cplusplus
}
#endif

//TODO: delete the index
