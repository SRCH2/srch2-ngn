package com.srch2.android.http.service;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.URL;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/**
 * This is the primary point of access for all of the functions of the SRCH2 Android SDK.
 * <br><br>
 * All methods in this class are statically declared so that they may be called without having
 * a reference to any instance. Note that any method that can be called on an instance of an
 * <code>Indexable</code>, can be called statically on this class by using the name of the index
 * that the <code>Indexable</code> represents.
 * <br><br>
 * The search functionality of the SRCH2 Android SDK is powered by the running of the SRCH2
 * HTTP server. This server runs in its own process, and this process is started and stopped
 * by the <code>SRCH2Service</code> remote service to insulate it from low-memory pressure.
 * The <code>SRCH2Engine</code> starts and stops this service, which in turn starts and stops
 * the SRCH2 search server. When <code>SRCH2Engine.onStop(Context context)</code> is called,
 * the SRCH2 search server is not shut down immediately, but only after a delay in order to
 * avoid unnecessary restarting in the event a user navigates away from the application using
 * the SRCH2 Android SDK only to do some short task such as sending a text message.
 * <br><br>
 * Since the SRCH2 search server is running as a RESTful HTTP server, any commands to the
 * SRCH2 search server must be formed as network or RESTful actions. This class and the rest
 * of the API wraps this condition of operation so that users of the SRCH2 Android SDK do
 * not have to manually form their own network tasks but can instead simply make the appropriate
 * calls on either the <code>SRCH2Engine</code> or their <code>Indexable</code> implementations.
 * Similarly, the output of the SRCH2 search server is wrapped in the various subclasses of
 * <code>RestfulResponse</code> so that the RESTful responses from the SRCH2 search server
 * do not have to be parsed: for instance, after inserting a record, the method <code>
 * mStateResponseListener.onInsertRequestComplete(String indexName, InsertResponse response)</code>
 * will be triggered where the <code>InsertResponse response</code> will contain a count of
 * the number of successful inserts. Or, after performing an information request on an index,
 * the method <code>mStateResponseListener.onInfoRequestComplete(String indexName, InfoResponse
 * response)</code> will be triggered where the <code>InfoResponse response</code> contains
 * various method getters for returning state about the index such as its number of records.
 * <br><br>
 * In particular, the <code>Query</code> class enables easy use of the sophisticated search power
 * of the SRCH2 search server.
 */
final public class SRCH2Engine {

    private static final String TAG = "SRCH2Engine";
    static final AtomicReference<IndexQueryPair> lastQuery = new AtomicReference<IndexQueryPair>();
    static final AtomicBoolean isChanged = new AtomicBoolean(false);
    static final AtomicBoolean isReady = new AtomicBoolean(false);
    /**
     * Automatic re-request the lastQuerySting whenever any index is
     * updated. If the Index is null, that means the lastQuery is from the
     * SearchAll
     */

    static boolean isDebugAndTestingMode = false;
    static SRCH2EngineBroadcastReciever incomingIntentReciever;
    static SearchResultsListener searchResultsListener = null;
    static StateResponseListener stateResponseListener = null;
    static boolean isStarted = false;
    static SRCH2Configuration conf = null;
    private static SearchTask allIndexSearchTask = null;

    private SRCH2Engine() { }

    /**
     * Initializes the state of the <code>SRCH2Engine</code> and prepares it for performing searches.
     * <br><br>
     * This method should only be called once per application life-cycle such as when an instance of
     * an activity requiring search is created (thus from the activity's <code>onCreate</code> method).
     * Calling this method will not start the SRCH2 search server and <b>must</b> be called before any
     * call to the method <code>SRCH2Engine.onStart(Context context)</code> is made.
     * <br><br>
     * Callers <b>must</b> pass at least one <code>Indexable</code> to be searched on and can pass in
     * as many as are needed.
     * @param firstIndex an <code>Indexable</code> that must not be null
     * @param additionalIndexes any other <code>Indexable</code>s that are defined
     */
    public static void initialize(Indexable firstIndex, Indexable... additionalIndexes) {
        Log.d("srch2:: " + TAG, "initialize");

        if (firstIndex == null) {
            throw new IllegalArgumentException("The provided Indexable object is null");
        }

        SRCH2Engine.lastQuery.set(null);
        SRCH2Engine.isChanged.set(false);
        SRCH2Engine.isReady.set(false);
        SRCH2Engine.isStarted = false;
        SRCH2Engine.allIndexSearchTask = null;

        SRCH2Engine.conf = new SRCH2Configuration(firstIndex, additionalIndexes);
    }

    /**
     * If JUnit or Android automated tests are to be run, calling this method and passing <b>true</b>
     * will cause the <code>SRCH2Engine</code> to immediately stop the SRCH2 search server if it is running
     * anytime <code>SRCH2Engine.onStop(Context context)</code> is called instead of waiting to do so after
     * a delay.
     * <br><br>
     * Developers performing automated testing <i>should</i> call this method before running any tests.
     * @param isTestingMode toggles whether the <code>SRCH2Engine</code> should run in testing mode
     */
    public static void setAutomatedTestingMode(boolean isTestingMode) {
        isDebugAndTestingMode = isTestingMode;
    }

    /**
     * Causes the SRCH2 search server powering the search to start.
     * <br><br>
     * This method should be called anytime the activity requiring search functionality comes to the
     * foreground and is visible--that is, when it can be interacted with by a user who might perform searches.
     * Starting the SRCH2 search server is fast, usually taking under a second, and when it comes online
     * and is ready to handle search requests the callback method <code>onSRCH2ServiceReady()</code> of the
     * <code>StateResponseListener</code> interface, if implemented, will be executed and contain index and
     * engine information. Checking whether the SRCH2 search server is ready can also determined by calling
     * <code>SRCH2Engine.isReady()</code>.
     * <br><br>
     * A context is needed here to start the remote service that hosts the SRCH2 search server process. A
     * reference to this context is not kept.
     * @param context needed to start a remote service, any context will do
     */
    public static void onStart(Context context) {


        incomingIntentReciever = new SRCH2EngineBroadcastReciever();
        context.registerReceiver(incomingIntentReciever, IPCConstants
                .getSRCH2EngineBroadcastRecieverIntentFilter(context));

        Log.d("srch2:: " + TAG, "onStart");
        if (isStarted) {
            return;
        }

        checkConfIsNullThrowIfIs();
        Log.d("srch2:: " + TAG,
                "onStart - conf NOT null ");
        int freePort = detectFreePort();

        conf.setPort(freePort);

        String appHomeDirectory = detectAppHomeDir(context);
        Log.d(TAG, "$$$$$$$$$$$$$$$ app home directory is : " + appHomeDirectory);

        SRCH2Engine.conf.setSRCH2Home(appHomeDirectory
                + File.separator
                + SRCH2Configuration.SRCH2_HOME_FOLDER_DEFAULT_NAME);

        startSRCH2Service(context, SRCH2Configuration.toXML(SRCH2Engine.conf));
        isStarted = true;
    }

    private static void startCheckCoresLoadedTask() {
        Log.d("srch2:: " + TAG, "startCheckCoresLoadedTask");
        HashMap<String, URL> indexUrlMap = new HashMap<String, URL>();
        for (IndexInternal index : conf.indexesMap.values()) {
            indexUrlMap.put(index.getIndexCoreName(),
                    UrlBuilder.getInfoUrl(conf, index.getConf()));
        }

        resetState();
        CheckCoresLoadedTask task = new CheckCoresLoadedTask(indexUrlMap,
                stateResponseListener);
        HttpTask.executeTask(task);
    }

    private static void resetState() {
        Log.d("srch2:: " + TAG, "resetState");
        lastQuery.set(null);
        isChanged.set(false);
        isReady.set(false);
    }

    static SearchResultsListener getSearchResultsListener() {
        return searchResultsListener;
    }

    static IndexInternal getIndexByNameReturnNullIfNotExists(String name){
        return conf.indexesMap.get(name);
    }

    /**
     * Registers the implementation of the interface <code>SearchResultsListener</code> for receiving
     * the results of a search performed by the SRCH2 search server. This can be reset at anytime, and
     * although it is not required to be set, it is the only way to get search results within the API.
     *
     * @param searchResultsListener the implementation of <code>SearchResultsListener</code> that will
     *                              receive search results
     */
    public static void setSearchResultsListener(
            SearchResultsListener searchResultsListener) {
        SRCH2Engine.searchResultsListener = searchResultsListener;
    }

    static StateResponseListener getControlResponseListener() {
        return stateResponseListener;
    }

    /**
     * Registers the implementation of the interface <code>StateResponseListener</code> for receiving
     * the state and status information about indexes and the SRCH2 search server.  This can be reset
     * at anytime, and although it is not required to be set, the callbacks of the
     * <code>StateResponseListener</code> are useful for knowing whether inserts, updates or deletes
     * were performed correctly, or, more importantly, when the SRCH2 search server comes online
     * and is ready for search.
     * @param stateResponseListener the implementation of <code>StateResponseListener</code> that will
     *                              receive index and engine status and state information
     */
    public static void setStateResponseListener(
            StateResponseListener stateResponseListener) {
        SRCH2Engine.stateResponseListener = stateResponseListener;
    }

    private static void searchAllRawString(String rawQueryString) {
        lastQuery.set(new IndexQueryPair(null, rawQueryString));
        if (isReady()) {
            if (allIndexSearchTask != null) {
                allIndexSearchTask.cancel();
            }
            allIndexSearchTask = new SearchTask(UrlBuilder.getSearchUrl(conf, null,
                    rawQueryString), searchResultsListener);
            HttpTask.executeTask(allIndexSearchTask);
        }
    }

    /**
     * Searches all <code>Indexable</code>s that were initialized when included in the argument set
     * of the method call <code>SRCH2Engine.initialize(...)</code>; that is, this method makes searching
     * all indexes at once easy.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * <code>onNewSearchResultsAvailable(int HTTPResponseCode, String jsonResultsLiteral,
     * HashMap<String, ArrayList<JSONObject>> resultRecordMap)</code> will be triggered. The
     * <code>resultRecordMap</code> will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw exceptions if the <code>searchInput</code> is null or if
     * <code>SRCH2Engine.initialize(...)</code> has not been called; of if the value of
     * <code>searchInput</code> is null or has a length less than one.
     * @param searchInput the textual input to search on
     */
    public static void searchAllIndexes(String searchInput) {
        Log.d("srch2:: " + TAG, "searchAllIndexes");
        checkConfIsNullThrowIfIs();
        String rawString= IndexInternal.formatDefaultQueryURL(searchInput);
        searchAllRawString(rawString);
    }


    /**
     * Searches the index with the specified <code>indexName</code> (as set by the <code>Indexable</code> implementation
     * of the method <code>getIndexName()</code>) on the specified <code>searchInput</code>.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * <code>onNewSearchResultsAvailable(int HTTPResponseCode, String jsonResultsLiteral,
     * HashMap<String, ArrayList<JSONObject>> resultRecordMap)</code> will be triggered. The
     * <code>resultRecordMap</code> will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw exceptions if the <code>indexName</code> is null or does not match any of
     * the index names as defined by the corresponding <code>Indexable</code> implementation of
     * of <code>getIndexName()</code>;
     * or if the <code>searchInput</code> is null or has a length less than one; or if <code>SRCH2Engine.initialize(...)</code> has
     * not been called.
     * @param indexName the name of the index as defined by the <code>Indexable</code> implementation of <code>getIndexName()</code>
     * @param searchInput the textual input to search on
     */
    public static void searchIndex(String indexName, String searchInput) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).search(searchInput);
    }

    /**
     * Performs an advanced searches on all <code>Indexable</code>s that were initialized when included
     * in the argument set of the method call <code>SRCH2Engine.initialize(...)</code>; that is, this method
     * makes performing an advanced search on all indexes at once easy.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * <code>onNewSearchResultsAvailable(int HTTPResponseCode, String jsonResultsLiteral,
     * HashMap<String, ArrayList<JSONObject>> resultRecordMap)</code> will be triggered. The
     * <code>resultRecordMap</code> will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw exceptions if the <code>query</code> forming the advanced search is null;
     * or if <code>SRCH2Engine.initialize(...)</code> has not been called; or if the value  of
     * <code>query</code> is null;
     * @param query the formation of the advanced search
     */
    public static void advancedSearchOnAllIndexes(Query query) {
        Log.d("srch2:: " + TAG, "searchAllIndexes");
        checkConfIsNullThrowIfIs();
        String rawString = query.toString();
        searchAllRawString(rawString);
    }

    /**
     * Performs an advanced search on the index with the specified <code>indexName</code> (as set by the
     * <code>Indexable</code> implementation
     * of the method <code>getIndexName()</code>) using the specified <code>query</code>.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * <code>onNewSearchResultsAvailable(int HTTPResponseCode, String jsonResultsLiteral,
     * HashMap<String, ArrayList<JSONObject>> resultRecordMap)</code> will be triggered. The
     * <code>resultRecordMap</code> will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw exceptions if the <code>indexName</code> is null or does not match any of
     * the index names as defined by the corresponding <code>Indexable</code> implementation of
     * of <code>getIndexName()</code>;
     * or if the <code>query</code> forming the advanced search is null; or if
     * <code>SRCH2Engine.initialize(...)</code> has not been called; or if the value  of
     * <code>query</code> is null;
     * @param indexName the name of the index as defined by the <code>Indexable</code>
     *                  implementation of <code>getIndexName()</code>
     * @param query the formation of the advanced search
     */
    public static void advancedSearchIndex(String indexName, Query query) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).advancedSearch(query);
    }

    /**
     * Inserts the specified <code>recordToInsert</code> into the index with the matching name of
     * <code>indexName</code> (as set by the <code>Indexable</code> implementation  of the method
     * <code>getIndexName()</code>). This <code>JSONObject</code> should be properly formed and contain
     * only keys matching the schema fields defined for the corresponding <code>Indexable</code>.
     * <br><br>
     * When the SRCH2 search server completes the insert task, the method
     * <code>onInsertRequestComplete(String indexName, InsertResponse response)</code>
     * of the <code>StateResponseListener</code> will be triggered. Whether the SRCH2 search server
     * was successful in inserting the record is signified by the value of the
     * <code>InsertReponse response</code>'s method <code>getSuccessCount()</code>.
     * <br><br>
     * This method will throw exceptions if the <code>indexName</code> is null or does not match any of
     * the index names as defined by the corresponding <code>Indexable</code> implementation of
     * of <code>getIndexName()</code>;
     * or if <code>SRCH2Engine.initialize(...)</code> has not been called.
     * @param indexName  the name of the index as defined by the <code>Indexable</code> implementation of <code>getIndexName()</code>
     * @param recordToInsert the <code>JSONObject</code> with keys matching the fields of the
     *                       schema as defined in the corresponding <code>Indexable</code> to insert
     */
    public static void insertIntoIndex(String indexName, JSONObject recordToInsert) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).insert(recordToInsert);
    }

    /**
     * Inserts the specified <code>recordsToInsert</code> into the index with the matching name of
     * <code>indexName</code> (as set by the <code>Indexable</code> implementation  of the method
     * <code>getIndexName()</code>). This <code>JSONArray</code> should be properly formed and contain only
     * the set of properly formed <code>JSONObject</code>s (that contain only keys matching the schema
     * fields as defined for the corresponding <code>Indexable</code>).
     * <br><br>
     * When the SRCH2 search server completes the insert task, the method
     * <code>onInsertRequestComplete(String indexName, InsertResponse response)</code>
     * of the <code>StateResponseListener</code> will be triggered. Whether the SRCH2 search server
     * was successful in inserting the records is signified by the value of the
     * <code>InsertReponse response</code>'s method <code>getSuccessCount()</code>.
     * <br><br>
     * This method will throw exceptions if the <code>indexName</code> is null or does not match any of
     * the index names as defined by the corresponding <code>Indexable</code> implementation of
     * of <code>getIndexName()</code>;
     * or if <code>SRCH2Engine.initialize(...)</code> has not been called.
     * @param indexName  the name of the index as defined by the <code>Indexable</code> implementation of <code>getIndexName()</code>
     * @param recordsToInsert the <code>JSONArray</code> containing the set of <code>JSONObject</code>s
     *                        with keys matching the fields of the schema as defined in the corresponding
     *                        <code>Indexable</code> to insert
     */
    public static void insertIntoIndex(String indexName, JSONArray recordsToInsert) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).insert(recordsToInsert);
    }

    /**
     * Updates the specified <code>recordToUpdate</code> in the index with the matching name of
     * <code>indexName</code> (as set by the <code>Indexable</code> implementation  of the method
     * <code>getIndexName()</code>). This <code>JSONObject</code> should be properly formed and contain
     * only keys matching the schema fields defined for the corresponding <code>Indexable</code>.
     * If the primary key of this record does not already exist in the index, it will be inserted;
     * otherwise, the existing record will have its fields updated with the new values in the
     * <code>recordToUpdate.</code>
     * <br><br>
     * When the SRCH2 search server completes the update task, the method
     * <code>onUpdateRequestComplete(String indexName, UpdateResponse response)</code>
     * of the <code>StateResponseListener</code> will be triggered. Whether the SRCH2 search server
     * was successful in updating the record is signified by the value of the
     * <code>UpdateResponse response</code>'s method <code>getSuccessCount()</code>.
     * <br><br>
     * This method will throw exceptions if the <code>indexName</code> is null or does not match any of
     * the index names as defined by the corresponding <code>Indexable</code> implementation of
     * of <code>getIndexName()</code>;
     * or if <code>SRCH2Engine.initialize(...)</code> has not been called.
     * @param indexName  the name of the index as defined by the <code>Indexable</code> implementation of <code>getIndexName()</code>
     * @param recordToUpdate the <code>JSONObject</code> with keys matching the fields of the
     *                       schema as defined in the corresponding <code>Indexable</code> to update
     */
    public static void updateIndex(String indexName, JSONObject recordToUpdate) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).update(recordToUpdate);
    }

    /**
     * Updates the set of records contained in the <code>recordsToUpdate</code> in the index with the matching name of
     * <code>indexName</code> (as set by the <code>Indexable</code> implementation  of the method
     * <code>getIndexName()</code>). This <code>JSONArray</code> should be properly formed and contain only
     * the set of properly formed <code>JSONObject</code>s (that contain only keys matching the schema
     * fields as defined for the corresponding <code>Indexable</code>). For each <code>JSONObject</code>
     * in the set, if the primary key does not already exist in the index, the record will be inserted;
     * otherwise, the existing record will have its fields updated with the new values.
     * <br><br>
     * When the SRCH2 search server completes the update task, the method
     * <code>onUpdateRequestComplete(String indexName, UpdateResponse response)</code>
     * of the <code>StateResponseListener</code> will be triggered. Whether the SRCH2 search server
     * was successful in updating the records is signified by the value of the
     * <code>UpdateResponse response</code>'s method <code>getSuccessCount()</code>.
     * <br><br>
     * This method will throw exceptions if the <code>indexName</code> is null or does not match any of
     * the index names as defined by the corresponding <code>Indexable</code> implementation of
     * of <code>getIndexName()</code>;
     * or if <code>SRCH2Engine.initialize(...)</code> has not been called.
     * @param indexName  the name of the index as defined by the <code>Indexable</code> implementation of <code>getIndexName()</code>
     * @param recordsToUpdate the <code>JSONArray</code> containing the set of <code>JSONObject</code>s
     *                        with keys matching the fields of the schema as defined in the corresponding
     *                        <code>Indexable</code> to update
     */
    public static void updateIndex(String indexName, JSONArray recordsToUpdate) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).update(recordsToUpdate);
    }

    /**
     * Deletes the record with the matching <code>primaryKeyToDelete</code> into the index with the matching name of
     * <code>indexName</code> (as set by the <code>Indexable</code> implementation  of the method
     * <code>getIndexName()</code>).
     * <br><br>
     * When the SRCH2 search server completes the deletion task, the method
     * <code>onDeleteRequestComplete(String indexName, DeleteResponse response)</code>
     * of the <code>StateResponseListener</code> will be triggered. Whether the SRCH2 search server
     * was successful in deleting the record is signified by the value of the
     * <code>DeleteResponse response</code>'s method <code>getSuccessCount()</code>.
     * <br><br>
     * This method will throw exceptions if the <code>indexName</code> is null or does not match any of
     * the index names as defined by the corresponding <code>Indexable</code> implementation of
     * of <code>getIndexName()</code>;
     * or if <code>SRCH2Engine.initialize(...)</code> has not been called.
     * @param indexName  the name of the index as defined by the <code>Indexable</code> implementation of <code>getIndexName()</code>
     * @param primaryKeyOfRecordToDelete the primary key of the record to delete
     */
    public static void deleteFromIndex(String indexName, String primaryKeyOfRecordToDelete) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).delete(primaryKeyOfRecordToDelete);
    }

    /**
     * Returns the record with the specified <code>primaryKeyOfRecordToRetrieve</code> if found in the index with the matching name of
     * <code>indexName</code> (as set by the <code>Indexable</code> implementation  of the method
     * <code>getIndexName()</code>). When the SRCH2
     * search server completes the retrieval task, the method <code>onGetRecordByIDComplete(String
     * indexName, GetRecordResponse response)</code> of the <code>StateResponseListener</code> will
     * be triggered. The record retrieved will be in the form of a <code>JSONObject</code> inside the
     * <code>response GetRecordResponse</code>.
     * <br><br>
     * This method will throw exceptions if the <code>indexName</code> is null or does not match any of
     * the index names as defined by the corresponding <code>Indexable</code> implementation of
     * of <code>getIndexName()</code>;
     * or if <code>SRCH2Engine.initialize(...)</code> has not been called; or if the value of
     * <code>primaryKeyOfRecordToRetrieve</code> is null or has a length less than one.
     * @param indexName the name of the index as defined by the <code>Indexable</code> implementation of <code>getIndexName()</code>
     * @param primaryKeyOfRecordToRetrieve the primary key of the record to retrieve
     */
    public static void getRecordByIdFromIndex(String indexName, String primaryKeyOfRecordToRetrieve) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).getRecordbyID(primaryKeyOfRecordToRetrieve);
    }

    /**
     * Performs an information task on the specified index for the <code>Indexable</code> with the
     * corresponding <code>indexName</code> as defined in the <code>getIndexName()</code> of that
     * <code>Indexable</code>. When the SRCH2 search server completes the information task, the
     * method <code>void onInfoRequestComplete(String indexName, InfoResponse response)</code>
     * will be triggered. The <code>InfoResponse response</code> will contain information about the
     * index such as the number of records it contains or the time stamp of the last time the index
     * was updated to reflect any pending changes.
     * <br><br>
     * This method will throw exceptions if the <code>indexName</code> is null or does not match any of
     * the index names as defined by the corresponding <code>Indexable</code> implementation of
     * of <code>getIndexName()</code>;
     * or if <code>SRCH2Engine.initialize(...)</code> has not been called.
     * @param indexName the name of the index as defined by the <code>Indexable</code> implementation of <code>getIndexName()</code>
     */
    public static void getIndexInfo(String indexName) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).info();
    }

    /**
     * Tells the <code>SRCH2Engine</code> to bring the SRCH2 search server to a stop. The SRCH2
     * search server will not halt immediately, but instead wait a short duration in case the user
     * navigates back to the activity requiring search. In that case, the command to stop is
     * cancelled. Since the SRCH2 search server is hosted by a remote service, a context is needed
     * to stop this service. After calling this method, <code>SRCH2Engine.isReady()</code> will
     * return false and no subsequent actions can be performed on the <code>SRCH2Engine</code> until
     * <code>SRCH2Engine.onStart(Context context)</code> is called again. Pending tasks however,
     * such as batch inserts, will not be interrupted and be allowed to finish.
     * @param context needed to stop a remote service, any context will do
     */
    public static void onStop(Context context) {
        Log.d("srch2:: " + TAG, "onStop");
        stopExecutable(context);
        resetState();
        isStarted = false;
        try {
            context.unregisterReceiver(incomingIntentReciever);
        } catch (IllegalArgumentException ignore) {
        }

    }

    /**
     * The engine need some time to load the index. During these windows the
     * engine will not response for any write operations. For the search query,
     * the engine keeps updating the last query and will automatically request
     * the
     *
     * @return if the SRCH2 engine is ready or not.
     */
    public static boolean isReady() {
        return isReady.get();
    }

    static SRCH2Configuration getConfig() {
        return conf;
    }

    private static void startSRCH2Service(final Context context,
                                          String xmlConfigLiteral) {
        Log.d("srch2:: " + TAG, "startSRCH2Service METHOD being called");
        Log.d("srch2:: " + TAG, "startSRCH2Service METHOD being called XML CONFIG IS \n"+xmlConfigLiteral);

        Intent i = new Intent(context, SRCH2Service.class);
        i.putExtra(IPCConstants.INTENT_KEY_XML_CONFIGURATION_FILE_LITERAL,
                xmlConfigLiteral);
        i.putExtra(IPCConstants.INTENT_KEY_SHUTDOWN_URL, UrlBuilder
                .getShutDownUrl(getConfig()).toString());
        i.putExtra(IPCConstants.INTENT_KEY_PORT_NUMBER, conf.getPort());
        i.putExtra(IPCConstants.INTENT_KEY_OAUTH, conf.getAuthorizationKey());
        i.putExtra(IPCConstants.INTENT_KEY_IS_DEBUG_AND_TESTING_MODE, isDebugAndTestingMode);
        context.startService(i);
        HttpTask.onStart();
    }

    private static void stopExecutable(final Context context) {
        Log.d("srch2:: " + TAG, "stopExecutable");
        Intent i = new Intent(
                IPCConstants
                        .getSRCH2ServiceBroadcastRecieverIntentAction(context));
        i.putExtra(IPCConstants.INTENT_KEY_START_AWAITING_SHUTDOWN,
                IPCConstants.INTENT_KEY_START_AWAITING_SHUTDOWN);
        context.sendBroadcast(i);
        HttpTask.onStop();
    }

    static String detectAppHomeDir(Context context) {
        Log.d("srch2:: " + TAG, "detectAppHomeDir");
        return context.getApplicationContext().getFilesDir().getAbsolutePath();
    }

    /**
     * search the task again
     */
    static void reQueryLastOne() {
        Log.d("srch2:: " + TAG, "reQueryLastOne");
        IndexQueryPair pair = lastQuery.get();
        if (pair == null || pair.query == null) {
            return;
        }
        if (pair.index == null) {
            SRCH2Engine.searchAllRawString(pair.query);
        } else {
            pair.index.searchRawString(pair.query);
        }
    }

    static int detectFreePort() {
        Log.d("srch2:: " + TAG, "detectFreePort");
        int port = 49152;
        for (; port < 65535; ++port) {
            try {
                new ServerSocket(port).close();
                break;
            } catch (IOException ex) {
            }
        }
        return port;
    }

    private static void checkConfIsNullThrowIfIs() {
        if (conf == null) {
            throw new NullPointerException(
                    "Cannot start SRCH2Engine without configuration being set.");
        }
    }

    /**
     * Sets the authorization key that is required for the SRCH2 search server to perform any command or task.
     * <br><br>
     * If this key is specified, each valid HTTP request needs to provide the following key-value pair in order to get the authorization.
     * OAuth=foobar
     * Example: curl -i "HTTP://localhost:8081/search?q=terminator&OAuth=foobar"
     * <br><br>
     * If this key is not specified, it will be automatically generated by the <code>SRCH2Engine</code>.
     * <br><br>
     * This method will throw an exception if <code>SRCH2Engine.initialize(...)</code> has not been called.
     * @param authorizationKey the key that any request on the SRCH2 search server will have to supply in order for the
     *                         SRCH2 search server to carry out the command or task
     */
    public static void setAuthorizationKey(String authorizationKey) {
        checkConfIsNullThrowIfIs();
        conf.setAuthorizationKey(authorizationKey);
    }

    static private class SRCH2EngineBroadcastReciever extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d("srch2:: " + TAG, "onReceive");
            int actualPortExecutableStartedWith = intent.getIntExtra(
                    IPCConstants.INTENT_KEY_PORT_NUMBER, 0);
            String actualOAuthExecutableStartedWith = intent.getStringExtra(IPCConstants.INTENT_KEY_OAUTH);
            if ((SRCH2Engine.conf.getPort() != actualPortExecutableStartedWith
                    && actualPortExecutableStartedWith != 0) && actualOAuthExecutableStartedWith != null) {
                Log.d("srch2:: " + TAG, "onReceive - resting port to "
                        + actualPortExecutableStartedWith);
                SRCH2Engine.conf.setPort(actualPortExecutableStartedWith);
                SRCH2Engine.conf.setAuthorizationKey(actualOAuthExecutableStartedWith);
            }
            SRCH2Engine.startCheckCoresLoadedTask();
        }
    }

    static class IndexQueryPair {
        public IndexInternal index = null;
        public String query = null;

        public IndexQueryPair(IndexInternal index, String query) {
            Log.d("srch2:: " + TAG, "IndexQueryPair()");
            this.index = index;
            this.query = query;
        }
    }
}
