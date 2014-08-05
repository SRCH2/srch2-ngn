package com.srch2.android.http.service;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.ServerSocket;
import java.net.URL;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

final public class SRCH2Engine {

    static final String TAG = "SRCH2Engine";
    static final AtomicReference<IndexQueryPair> lastQuery = new AtomicReference<IndexQueryPair>();
    static final AtomicBoolean isChanged = new AtomicBoolean(false);
    static final AtomicBoolean isReady = new AtomicBoolean(false);
    /**
     * Automatic re-request the lastQuerySting whenever any index is
     * updated. If the Index is null, that means the lastQuery is from the
     * SearchAll
     */
//    static final ArrayList<IndexInternal> indexes = new ArrayList<IndexInternal>();

    static boolean isDebugAndTestingMode = false;
    static SRCH2EngineBroadcastReciever incomingIntentReciever;
    static SearchResultsListener searchResultsListener = null;
    static StateResponseListener controlResultsListener = null;
    static boolean isStarted = false;
    static SRCH2Configuration conf = null;
    static SearchTask allIndexSearchTask = null;

    public static void initialize(Indexable index1, Indexable... restIndexes) {
        Log.d("srch2:: " + TAG, "initialize");

        if (index1 == null) {
            throw new IllegalArgumentException("The provided Indexable object is null");
        }

        SRCH2Engine.lastQuery.set(null);
        SRCH2Engine.isChanged.set(false);
        SRCH2Engine.isReady.set(false);
        SRCH2Engine.isStarted = false;
        SRCH2Engine.allIndexSearchTask = null;

        SRCH2Engine.conf = new SRCH2Configuration(index1, restIndexes);
    }

    /**
     * This method is to setup the Debug mode for the purpose of testing. The developer should set it in there test cases
     *
     * @param isDebug open or close the deubg mode
     */
    public static void setTestAndDebugMode(boolean isDebug) {
        isDebugAndTestingMode = isDebug;
    }

    /**
     * To start the SRCH2 Engine. Client's APP must call this function to start
     * the service. When the Engine is ready, the
     * <code>controlResultsListener</code> will be called. User can check the
     * <code>onSRCH2ServiceReady</code> to get all the engine information.
     *
     * @param context The Android Context
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

    static void startCheckCoresLoadedTask() {
        Log.d("srch2:: " + TAG, "startCheckCoresLoadedTask");
        HashMap<String, URL> indexUrlMap = new HashMap<String, URL>();
        for (IndexInternal index : conf.indexesMap.values()) {
            indexUrlMap.put(index.getIndexCoreName(),
                    UrlBuilder.getInfoUrl(conf, index.getConf()));
        }

        resetState();
        CheckCoresLoadedTask task = new CheckCoresLoadedTask(indexUrlMap,
                controlResultsListener);
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

    /**
     * Set the IndexSearchResultListener. This listener will receive the search
     * results from all the indexes.
     *
     * @param indexSearchResultListener
     */
    public static void setSearchResultsListener(
            SearchResultsListener indexSearchResultListener) {
        searchResultsListener = indexSearchResultListener;
    }

    static StateResponseListener getControlResponseListener() {
        return controlResultsListener;
    }

    /**
     * Set the controlResultsListener. This listener will receive the
     * insert/update/delete/info response from the engine
     *
     * @param listener
     */
    public static void setControlResponseListener(
            StateResponseListener listener) {
        controlResultsListener = listener;
    }

    static void searchAllRawString(String rawQueryString) {
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
     * To search all the indexes. The search result will pass to the
     * <code> indexSearchResultListener</code>. To search one specific
     * <code>Index</code> please use <code>Index.search</code>
     *
     * @param searchInput
     */
    public static void searchAllIndexes(String searchInput) {
        Log.d("srch2:: " + TAG, "searchAllIndexes");
        checkConfIsNullThrowIfIs();
        String rawString = null;
        rawString = IndexInternal.formatDefaultQueryURL(searchInput);
        searchAllRawString(rawString);
    }

    public static void searchIndex(String indexName, String searchInput) throws UnsupportedEncodingException {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).search(searchInput);
    }

    public static void advancedSearchOnAllIndexes(Query query) {
        Log.d("srch2:: " + TAG, "searchAllIndexes");
        checkConfIsNullThrowIfIs();
        String rawString = query.toString();
        searchAllRawString(rawString);
    }

    public static void advancedSearchIndex(String indexName, Query query) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).advancedSearch(query);
    }

    public static void insertIntoIndex(String indexName, JSONObject recordToUpdate) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).insert(recordToUpdate);
    }

    public static void insertIntoIndex(String indexName, JSONArray recordsToUpdate) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).insert(recordsToUpdate);
    }

    public static void updateIndex(String indexName, JSONObject recordToUpdate) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).update(recordToUpdate);
    }

    public static void updateIndex(String indexName, JSONArray recordsToUpdate) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).update(recordsToUpdate);
    }

    public static void deleteFromIndex(String indexName, String firstIdToDelete) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).delete(firstIdToDelete);
    }

    public static void getRecordByIdFromIndex(String indexName, String idOfRecordToRetrieve) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).getRecordbyID(idOfRecordToRetrieve);
    }

    public static void getIndexInfo(String indexName) {
        checkConfIsNullThrowIfIs();
        conf.getIndexAndThrowIfNotThere(indexName).info();
    }

    /**
     * The stop the SRCH2 Engine. Client's APP must call this function to stop
     * the service.
     *
     * @param context
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
     * @return
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

    private static String detectAppHomeDir(Context context) {
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

    static void checkConfIsNullThrowIfIs() {
        if (conf == null) {
            throw new NullPointerException(
                    "Cannot start SRCH2Engine without configuration being set.");
        }
    }

    /**
     * This parameter specifies an authorization key that is required in each HTTP request to the engine.
     * If this key is specified, each valid HTTP request needs to provide the following key-value pair in order to get the authorization.
     * OAuth=foobar
     * Example: curl -i "http://localhost:8081/search?q=terminator&OAuth=foobar"
     *
     * @param authorizationKey
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
