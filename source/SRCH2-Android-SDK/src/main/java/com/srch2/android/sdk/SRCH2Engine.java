package com.srch2.android.sdk;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;

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
 * a reference to any instance. In particular, note that reference to a specific
 * {@link com.srch2.android.sdk.Indexable}
 * subclass instance can be obtained by calling the static method
 * {@link com.srch2.android.sdk.SRCH2Engine#getIndex(String)}
 * and supplying a value of <code>indexName</code> that matches the return value of the
 * {@link Indexable#getIndexName()} for the particular <code>Indexable</code> to retrieve.
 * Thus any of the
 * <code>Indexable</code> methods can be accessed from everywhere in the code.
 * <br><br>
 * The search functionality of the SRCH2 Android SDK is powered by the running of the SRCH2
 * HTTP server. This server runs in its own process, and this process is started and stopped
 * by the {@link com.srch2.android.sdk.SRCH2Service} (a remote service) to insulate it from
 * low-memory pressures.
 * The <code>SRCH2Engine</code> starts and stops this service, which in turn starts and stops
 * the SRCH2 search server. When {@link com.srch2.android.sdk.SRCH2Engine#onStop(android.content.Context)}
 * is called,
 * the SRCH2 search server is not shut down immediately, but only after a delay in order to
 * avoid unnecessary restarting in the event a user navigates away from the activity performing
 * searches only to do some short task such as sending a text message.
 * <br><br>
 * Since the SRCH2 search server is running as a RESTful HTTP server, any commands to the
 * SRCH2 search server must be formed as network RESTful actions. This class and the rest
 * of the API wraps this condition of operation so that users of the SRCH2 Android SDK do
 * not have to manually form their own network tasks but can instead simply make the appropriate
 * calls on their <code>Indexable</code> implementations. For actions editing the index or
 * retrieving a particular record, the response from the SRCH2 search server will be passed
 * to the corresponding <code>Indexable</code> method: for example when the SRCH2 search server
 * completes an insertion after the method {@link Indexable#insert(org.json.JSONArray)} is called,
 * the method {@link com.srch2.android.sdk.Indexable#onInsertComplete(int, int, String)} will be
 * executed on that <code>Indexable</code> indicating the number of successful and failed insertions.
 * These methods should be overridden by the implementation of the <code>Indexable</code>, although
 * they do not have to be: if they are not they will out to the logcat under the tag 'SRCH2'.
 * For a search action, when the SRCH2 search completes the search, the results will be passed
 * through the method {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
 * to the implementation of the <code>SearchResultsListener</code> set by calling
 * {@link #setSearchResultsListener(SearchResultsListener)}.
 * <br><br>
 * It is also possible to harness the power of the sophisticated search functionality of the SRCH2 search server
 * through the SRCH2 Android SDK and its API by using the {@link Query} class.
 */
final public class SRCH2Engine {

    private static final String TAG = "SRCH2Engine";
    static final AtomicReference<IndexQueryPair> lastQuery = new AtomicReference<IndexQueryPair>();
    static final AtomicBoolean isChanged = new AtomicBoolean(false);
    static final AtomicBoolean isReady = new AtomicBoolean(false);

    static boolean isDebugAndTestingMode = false;
    static SRCH2EngineBroadcastReciever incomingIntentReciever;
    static SearchResultsListener searchResultsObserver = null;
    static boolean isStarted = false;
    static SRCH2Configuration conf = null;
    private static SearchTask allIndexSearchTask = null;
    static boolean searchResultsPublishedToUiThread = false;
    private static Handler searchResultsUiCallbackHandler;
    static Handler getSearchResultsUiCallbackHandler() {
        if (isReady.get()) {
            if (searchResultsUiCallbackHandler == null) {
                searchResultsUiCallbackHandler = new Handler(Looper.getMainLooper());
            }
            return searchResultsUiCallbackHandler;
        } else {
            return null;
        }
    }

    private SRCH2Engine() { }

    /**
     * Initializes the state of the <code>SRCH2Engine</code> and prepares it for performing searches.
     * <br><br>
     * This method should only be called once per application life-cycle such as when an instance of
     * an activity requiring search is created (thus from the activity's <code>onCreate</code> method).
     * Calling this method will not start the SRCH2 search server and <b>must</b> be called before any
     * call to the method {@link #onStart(android.content.Context)} is made.
     * <br><br>
     * Callers <b>must</b> pass at least one <code>Indexable</code> to be searched on and can pass in
     * as many as are needed.
     * @param firstIndex an <code>Indexable</code> that must not be null
     * @param additionalIndexes any other <code>Indexable</code>s that are defined
     */
    public static void initialize(Indexable firstIndex, Indexable... additionalIndexes) {
        Cat.d(TAG, "initialize");

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
     * anytime {@link #onStop(android.content.Context)} is called instead of waiting to do so after
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
     * and is ready to handle search requests the callback method {@link Indexable#onIndexReady()} will
     * be called for each index as it becomes loaded and ready for access. Checking
     * whether the SRCH2 search server is ready can also determined by calling
     * {@link #isReady()}.
     * <br><br>
     * An <code>context</code> instance is needed here to start the remote service that hosts the SRCH2 search
     * server process. A
     * reference to this context is not kept.
     * @param context needed to start a remote service, any context will do
     */
    public static void onStart(Context context) {


        incomingIntentReciever = new SRCH2EngineBroadcastReciever();
        context.registerReceiver(incomingIntentReciever, IPCConstants
                .getSRCH2EngineBroadcastRecieverIntentFilter(context));

        Cat.d(TAG, "onStart");
        if (isStarted) {
            return;
        }

        checkConfIsNullThrowIfIs();
        Cat.d(TAG,
                "onStart - conf UNARY_NOT null ");
        int freePort = detectFreePort();

        conf.setPort(freePort);

        String appHomeDirectory = detectAppHomeDir(context);
        Cat.d(TAG, "app home directory is : " + appHomeDirectory);

        SRCH2Engine.conf.setSRCH2Home(appHomeDirectory
                + File.separator
                + SRCH2Configuration.SRCH2_HOME_FOLDER_DEFAULT_NAME);

        startSRCH2Service(context, SRCH2Configuration.toXML(SRCH2Engine.conf));
        isStarted = true;
    }

    private static void startCheckCoresLoadedTask() {
        Cat.d(TAG, "startCheckCoresLoadedTask");
        HashMap<String, URL> indexUrlMap = new HashMap<String, URL>();
        for (Indexable index : conf.indexableMap.values()) {
            indexUrlMap.put(index.indexInternal.getIndexCoreName(),
                    UrlBuilder.getInfoUrl(conf, index.indexInternal.getConf()));
        }

        resetState();
        CheckCoresLoadedTask task = new CheckCoresLoadedTask(indexUrlMap);
        HttpTask.executeTask(task);
    }

    private static void resetState() {
        Cat.d(TAG, "resetState");
        isReady.set(false);
        lastQuery.set(null);
        isChanged.set(false);
        searchResultsPublishedToUiThread = false;
        if (searchResultsUiCallbackHandler != null) {
            searchResultsUiCallbackHandler = null;
        }
    }

    static SearchResultsListener getSearchResultsObserver() {
        return searchResultsObserver;
    }

    /**
     * Registers the implementation of the interface <code>SearchResultsListener</code> for receiving
     * the results of a search performed by the SRCH2 search server. This can be reset at anytime, and
     * although it is not required to be set, it is the only way to get search results within the API.
     * <br><br>
     * The callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * can be executed on a background thread, enabling post-processing operations, or can be executed
     * on the Ui thread. If it is not executed on the Ui thread, the search results will have to be
     * pushed the Ui thread before altering any of the Ui (such as when invalidating the adapter of
     * the <code>ListView</code> showing the search results).
     * @param searchResultsListener the implementation of <code>SearchResultsListener</code> that will
     *                              receive search results
     * @param callbackToUiThread whether to push the search results to the Ui thread
     */
    public static void setSearchResultsListener(
            SearchResultsListener searchResultsListener, boolean callbackToUiThread) {
        searchResultsObserver = searchResultsListener;
        searchResultsPublishedToUiThread = callbackToUiThread;
    }

    /**
     * Registers the implementation of the interface <code>SearchResultsListener</code> for receiving
     * the results of a search performed by the SRCH2 search server. This can be reset at anytime, and
     * although it is not required to be set, it is the only way to get search results within the API.
     * <br><br>
     * The callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * can be executed on a background thread, enabling post-processing operations, or can be executed
     * on the Ui thread. If it is not executed on the Ui thread, the search results will have to be
     * pushed the Ui thread before altering any of the Ui (such as when invalidating the adapter of
     * the <code>ListView</code> showing the search results). By default, this callback will not be
     * called on the Ui thread. Use {@link #setSearchResultsListener(SearchResultsListener, boolean)}
     * to enable pushing search results to the Ui thread.
     * @param searchResultsListener the implementation of <code>SearchResultsListener</code> that will
     *                              receive search results
     */
    public static void setSearchResultsListener(
            SearchResultsListener searchResultsListener) {
        searchResultsObserver = searchResultsListener;
    }

    private static void searchAllRawString(String rawQueryString) {
        lastQuery.set(new IndexQueryPair(null, rawQueryString));
        if (isReady()) {
            if (allIndexSearchTask != null) {
                allIndexSearchTask.cancel();
            }
            allIndexSearchTask = new SearchTask(UrlBuilder.getSearchUrl(conf, null,
                    rawQueryString), searchResultsObserver);
            HttpTask.executeTask(allIndexSearchTask);
        }
    }

    /**
     * Searches all <code>Indexable</code>s that were initialized when included in the argument set
     * of the method call {@link #initialize(Indexable, Indexable...)}; that is, this method makes searching
     * all indexes at once easy.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * {@link SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * will be triggered. The
     * <code>HashMap resultMap</code> will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw exceptions if the <code>searchInput</code> is null or if
     * {@link #initialize(Indexable, Indexable...)} has not been called; or if the value of
     * <code>searchInput</code> is null or has a length less than one.
     * @param searchInput the textual input to search on
     */
    public static void searchAllIndexes(String searchInput) {
        Cat.d(TAG, "searchAllIndexes");
        checkConfIsNullThrowIfIs();
        String rawString= IndexInternal.formatDefaultQueryURL(searchInput);
        searchAllRawString(rawString);
    }

    /**
     * Performs an advanced searches on all <code>Indexable</code>s that were initialized when included
     * in the argument set of the method call {@link #initialize(Indexable, Indexable...)} ; that is, this method
     * makes performing an advanced search on all indexes at once easy.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * {@link SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * will be triggered. The
     * <code>resultMap</code> will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw exceptions if the <code>query</code> forming the advanced search is null;
     * or if {@link #initialize(Indexable, Indexable...)} has not been called; or if the value  of
     * <code>query</code> is null;
     * @param query the formation of the advanced search
     */
    public static void advancedSearchOnAllIndexes(Query query) {
        Cat.d(TAG, "searchAllIndexes");
        checkConfIsNullThrowIfIs();
        String rawString = query.toString();
        searchAllRawString(rawString);
    }

    /**
     * Returns the <code>Indexable</code> representing the index that matches the value of
     * <code>indexName</code> as it was defined in the <code>Indexable</code> implementation
     * of <code>getIndexName()</code>. This method is a convenience getter, offering static access
     * to the <code>Indexable</code> references as they were supplied to
     * {@link SRCH2Engine#initialize(Indexable, Indexable...)}:
     * by obtaining the <code>Indexable</code>, its methods can be called to search, insert, update or
     * perform any other <code>Indexable</code> function.
     * @param indexName the name of the index as defined by the <code>Indexable</code> implementation of <code>getIndexName()</code>
     * @return the <code>Indexable</code> representing the index with the specified <code>indexName</code>
     */
    public static Indexable getIndex(String indexName) {
        checkConfIsNullThrowIfIs();
        return conf.getIndexableAndThrowIfNotThere(indexName);
    }

    /**
     * Tells the <code>SRCH2Engine</code> to bring the SRCH2 search server to a stop. The SRCH2
     * search server will not halt immediately, but instead wait a short duration in case the user
     * navigates back to the activity requiring search. In that case, the command to stop is
     * cancelled as long as {@link #onStart(android.content.Context)} is called again from, for example,
     * {@link android.app.Activity#onResume()}. Since the SRCH2 search server is hosted by a remote service, a context is needed
     * to stop this service. After calling this method, {@link #isReady()} will
     * return false and no subsequent actions can be performed on the <code>SRCH2Engine</code> until
     * {@link #onStart(android.content.Context)} is called again. Pending tasks however,
     * such as batch inserts, will not be interrupted and be allowed to finish.
     * @param context needed to stop a remote service, any context will do
     */
    public static void onStop(Context context) {
        Cat.d(TAG, "onStop");
        HeartBeatPing.stop();
        stopExecutable(context);
        resetState();
        isStarted = false;
        try {
            context.unregisterReceiver(incomingIntentReciever);
        } catch (IllegalArgumentException ignore) {
        }

    }

    /**
     * Determines if any calls to the <code>Indexable</code> such as insert, search or retrieve record
     * will be executed. This method should return <b>true</b> a short time after {@link #onStart(android.content.Context)}
     * is called for the first time in an app's life-cycle (it takes a moment or two for the SRCH2 engine to come online)
     * and remain <b>true</b> until
     * {@link #onStop(android.content.Context)} is called. Search requests made before the SRCH2 search server
     * comes online are cached, and the latest search input will be sent as a search request as soon it comes
     * online.
     *
     * @return if the <code>SRCH2Engine</code> is ready or not.
     */
    public static boolean isReady() {
        return isReady.get();
    }

    static SRCH2Configuration getConfig() {
        return conf;
    }

    private static void startSRCH2Service(final Context context,
                                          String xmlConfigLiteral) {
        Cat.d(TAG, "startSRCH2Service METHOD being called");
        Cat.d(TAG, "startSRCH2Service METHOD being called XML CONFIG IS \n"+xmlConfigLiteral);

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
        Cat.d(TAG, "stopExecutable");
        Intent i = new Intent(
                IPCConstants
                        .getSRCH2ServiceBroadcastRecieverIntentAction(context));
        i.putExtra(IPCConstants.INTENT_KEY_START_AWAITING_SHUTDOWN,
                IPCConstants.INTENT_KEY_START_AWAITING_SHUTDOWN);
        context.sendBroadcast(i);
        HttpTask.onStop();
    }

    static String detectAppHomeDir(Context context) {
        Cat.d(TAG, "detectAppHomeDir");
        return context.getApplicationContext().getFilesDir().getAbsolutePath();
    }

    static void reQueryLastOne() {
        Cat.d(TAG, "reQueryLastOne");
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

    static class ExceptionMessages {
        /** Will be message of IOException when port already in use. */
        static final String IO_EXCEPTION_EADDRINUSE_ADDRESS_ALREADY_IN_USE = "EADDRINUSE (Address already in use)";
        /** Will be message of IOException when Internet Permission missing from manifest. */
        static final String IO_EXCEPTION_EACCES_PERMISSION_DENIED = "EACCES (Permission Denied)";
        /** Will be message of IOException when server crashes. */
        static final String IO_EXCEPTION_ECONNREFUSED_CONNECTION_REFUSED = "ECONNREFUSED (Connection Refused)";
    }

    static int detectFreePort() {
        Cat.d(TAG, "detectFreePort");
        int port = 49152;
        for (; port < 65535; ++port) {
            try {
                new ServerSocket(port).close();
                break;
            } catch (IOException ex) {
                String message = ex.getMessage();
                if (message != null) {
                    Cat.d(TAG, "message: [" + message + "]");

                    if (message.contains(ExceptionMessages.IO_EXCEPTION_EADDRINUSE_ADDRESS_ALREADY_IN_USE)) {
                        continue;
                    } else if (message.contains(ExceptionMessages.IO_EXCEPTION_EACCES_PERMISSION_DENIED)) {
                        port = -1;
                        break;
                    }
                }
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
     * This method will throw an exception if {@link #initialize(Indexable, Indexable...)} has not been called.
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
            Cat.d(TAG, "onReceive");
            int actualPortExecutableStartedWith = intent.getIntExtra(
                    IPCConstants.INTENT_KEY_PORT_NUMBER, 0);
            String actualOAuthExecutableStartedWith = intent.getStringExtra(IPCConstants.INTENT_KEY_OAUTH);
            if ((SRCH2Engine.conf.getPort() != actualPortExecutableStartedWith
                    && actualPortExecutableStartedWith != 0) && actualOAuthExecutableStartedWith != null) {
                Cat.d(TAG, "onReceive - resting port to "
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
            Cat.d(TAG, "IndexQueryPair()");
            this.index = index;
            this.query = query;
        }
    }

    /**
     * Used to determine whether the user is subject to canopy immersion.
     * @return whether the user is an anteater and in a tree
     */
    public static boolean isUserAnAntEaterInATree() {
        return true && true;
    }
}
