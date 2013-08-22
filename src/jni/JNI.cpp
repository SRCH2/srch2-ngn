#include "SRCH2SDK.h"
#include "util/Logger.h"
#include "util/Evaluate.h"

using namespace srch2::sdk;
using namespace srch2::util;

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void Java_com_srch2_android_lib_SRCH2Index_setLogLevel(JNIEnv* env,
		jobject javaThis, jint logLevel) {
	Logger::setLogLevel((Logger::LogLevel) logLevel);
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jlong Java_com_srch2_android_lib_SRCH2Index_openLoggerFile(
		JNIEnv* env, jobject javaThis, jstring logfile) {
	const char *nativeStringLogFile = env->GetStringUTFChars(logfile, NULL);
	FILE* fpLog = fopen(nativeStringLogFile, "a");
	Logger::setOutputFile(fpLog);
	env->ReleaseStringUTFChars(logfile, nativeStringLogFile);
    long ptr = (long)fpLog;
    return ptr;
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void Java_com_srch2_android_lib_SRCH2Index_closeLoggerFile(
		JNIEnv* env, jobject javaThis, jlong ptr) {
	FILE* fpLog = (FILE*) ptr;
    fclose(fpLog);
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jlong Java_com_srch2_android_lib_SRCH2Index_createIndex(
		JNIEnv* env, jobject javaThis, jstring indexDir, jboolean isGeo) {
	const char *nativeStringIndexPath = env->GetStringUTFChars(indexDir, NULL);

	string strIndexPath(nativeStringIndexPath);
	Logger::console("createIndex at %s", strIndexPath.c_str());

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
JNIEXPORT void Java_com_srch2_android_lib_SRCH2Index_addRecord(JNIEnv* env,
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
JNIEXPORT jlong Java_com_srch2_android_lib_SRCH2Index_createIndexByFile(
		JNIEnv* env, jobject javaThis, jstring testFile, jstring indexDir,
		jint lineLimit, jboolean isGeo) {
	const char *nativeStringDataFile = env->GetStringUTFChars(testFile, NULL);
	const char *nativeStringIndexPath = env->GetStringUTFChars(indexDir, NULL);

	string strIndexPath(nativeStringIndexPath);
	string strTestFile(nativeStringDataFile);
	Logger::console("createIndexByFile");
	Logger::console("Read data from %s", strTestFile.c_str());
	Logger::console("Save index to %s", strIndexPath.c_str());

	Indexer* indexer = createIndex(strTestFile, strIndexPath, lineLimit, isGeo);

	env->ReleaseStringUTFChars(testFile, nativeStringDataFile);
	env->ReleaseStringUTFChars(indexDir, nativeStringIndexPath);
	long ptr = (long) indexer;
	Logger::console("createIndexByFile done");
	return ptr;
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jlong Java_com_srch2_android_lib_SRCH2Index_loadIndex(JNIEnv* env,
		jobject javaThis, jstring indexDir) {
	const char *nativeStringIndexPath = env->GetStringUTFChars(indexDir, NULL);
	string strIndexDir(nativeStringIndexPath);

	Logger::console("Load index from %s", strIndexDir.c_str());
    long ptr = 0;
    try{
        Indexer* indexer = loadIndex(strIndexDir);
        ptr = (long) indexer;
    }catch(std::runtime_error e){ 
        Logger::console("LoadIndex exception:%s", e.what());
        ptr = 0;   
    }
	env->ReleaseStringUTFChars(indexDir, nativeStringIndexPath);
	Logger::console("loadIndex done");
	return ptr;
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void Java_com_srch2_android_lib_SRCH2Index_saveIndex(JNIEnv* env,
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
JNIEXPORT void Java_com_srch2_android_lib_SRCH2Index_commitIndex(JNIEnv* env,
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
JNIEXPORT jobject Java_com_srch2_android_lib_SRCH2Index_query(JNIEnv* env,
		jobject javaThis, jlong indexPtr, jstring queryStr, jint editDistance,
		jboolean isGeo) {

	const char *nativeStringQuery = env->GetStringUTFChars(queryStr, NULL);
	string queryString(nativeStringQuery);
	Logger::console("query:%s", nativeStringQuery);
	Indexer* indexer = (Indexer*) indexPtr;
	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
	const Analyzer *analyzer = indexer->getAnalyzer();

	QueryResults* queryResults = query(analyzer, indexSearcher, queryString,
			editDistance);

	// Find java ArrayList
	jclass clsArrayList = env->FindClass("java/util/ArrayList");
	jmethodID constructor = env->GetMethodID(clsArrayList, "<init>", "(I)V");
	Logger::debug("arrayListConstructor: %d", constructor);
	jmethodID arrayListAdd = env->GetMethodID(clsArrayList, "add",
			"(Ljava/lang/Object;)Z");
	Logger::debug("arrayListAdd: %d", arrayListAdd);

	int count = queryResults->getNumberOfResults();
	jobject objArrayList = env->NewObject(clsArrayList, constructor, count);

	// Find com.srch2.android.lib.Hit
	jclass clsHit = env->FindClass("com/srch2/android/lib/Hit");
	Logger::debug("classHit: %d", clsHit);
	//public Hit(float score, String record, String[] keywords, int[] eds)
	jmethodID constructorHit = env->GetMethodID(clsHit, "<init>",
			"(FLjava/lang/String;[Ljava/lang/String;[I)V");
	Logger::debug("classHit constructor: %d", constructorHit);

	// Foreach queryResult

    vector<string> matchedKeywords;
    vector<unsigned> editDistances;
	for (int i = 0; i < count; i++) {
		float score = queryResults->getResultScore(i);
		string record = queryResults->getInMemoryRecordString(i);
		queryResults->getMatchingKeywords(i, matchedKeywords);
		queryResults->getEditDistances(i, editDistances);

		// create record string
		jstring jstrRecord = env->NewStringUTF(record.c_str());
		jstring jstrEmpty = env->NewStringUTF("");
		// create keywords array
		int size = (matchedKeywords).size();
		jobjectArray jstrArray = (jobjectArray) env->NewObjectArray(size,
				env->FindClass("java/lang/String"), jstrEmpty);
		for (int j = 0; j < size; j++) {
			jstring jstrMatch = env->NewStringUTF((matchedKeywords)[j].c_str());
			env->SetObjectArrayElement(jstrArray, j, jstrMatch);
			env->DeleteLocalRef(jstrMatch);
		}

		// create edit distance array
		jintArray jintArray = env->NewIntArray(size);
		jint* eds = new jint[size];
		for (int j = 0; j < size; j++) {
			eds[j] = (editDistances)[j];
		}
		env->SetIntArrayRegion(jintArray, 0, size, eds);
		delete[] eds;

		jobject hit = env->NewObject(clsHit, constructorHit, score, jstrRecord,
				jstrArray, jintArray);

		bool isAdded = env->CallBooleanMethod(objArrayList, arrayListAdd, hit);
		env->DeleteLocalRef(jstrRecord);
		env->DeleteLocalRef(jstrArray);
		env->DeleteLocalRef(jintArray);
		env->DeleteLocalRef(hit);
		env->DeleteLocalRef(jstrEmpty);
	}

	env->DeleteLocalRef(clsArrayList);
	env->DeleteLocalRef(clsHit);
//	env->DeleteLocalRef(objArrayList);
	Logger::debug("MemUsage:%d", getRAMUsageValue());
	delete queryResults;
	delete indexSearcher;
	env->ReleaseStringUTFChars(queryStr, nativeStringQuery);
	return objArrayList;
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void Java_com_srch2_android_lib_SRCH2Index_destroyIndex(
		JNIEnv* env, jobject javaThis, jlong ptr) {
    Indexer * indexer = (Indexer*)ptr;
    delete indexer;
}
#ifdef __cplusplus
}
#endif

