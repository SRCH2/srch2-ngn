package com.srch2.android.sdk;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/**
 * This is the primary point of access for all of the functions of the SRCH2 Android SDK.
 * <br><br>
 * The search functionality of the SRCH2 Android SDK is powered by the running of the SRCH2
 * HTTP search server. This server runs in its own process, and this process is started and stopped
 * by the {@link com.srch2.android.sdk.SRCH2Service} (a remote service) to insulate it from
 * low-memory pressures.
 * The {@code SRCH2Engine} starts and stops this service, which in turn starts and stops
 * the SRCH2 search server. When {@link com.srch2.android.sdk.SRCH2Engine#onPause(android.content.Context)}
 * is called,
 * the SRCH2 search server is not shut down immediately, but only after a delay in order to
 * avoid unnecessary restarting in the event a user navigates away from the activity performing
 * searches only to do some short task such as sending a text message.
 * <br><br>
 * Since the SRCH2 search server is running as a RESTful HTTP server, any commands to the
 * SRCH2 search server must be formed as network RESTful actions. This class and the rest
 * of the API wraps this condition of operation so that users of the SRCH2 Android SDK do
 * not have to manually form their own network tasks.
 * <br><br>
 * Indexes in the SRCH2 search server are represented by instances of {@link com.srch2.android.sdk.Indexable}
 * or {@link com.srch2.android.sdk.SQLiteIndexable}. These classes are abstract and can be used
 * to access the indexes they represent in the SRCH2 search server: they are derived from the class
 * {@link com.srch2.android.sdk.IndexableCore}. Subclassing {@link com.srch2.android.sdk.Indexable} can
 * be used to represent a default index, including geo-indexes; subclassing {@link com.srch2.android.sdk.SQLiteIndexable}
 * can be used to represent an index that is backed by SQLite database table. By passing the database
 * and table name in the appropriate getters of the implementation of that class, the {@code SRCH2Engine}
 * will configure the SRCH2 search server to automatically observe all data content of that SQLite table
 * and create and update the index that can be searched on.
 * <br><br>
 * The
 * two methods {@link #getIndex(String)} and {@link #getSQLiteIndex(String)} can be used to retrieve
 * the indexes by name (returned from {@link Indexable#getIndexName()} or {@link SQLiteIndexable#getIndexName()}.
 * Thus for default indexes users of the SRCH2 Android SDK can simply make the appropriate
 * calls on their {@code Indexable} implementations: for actions editing the index or
 * retrieving a particular record, the response from the SRCH2 search server will be passed
 * to the corresponding {@code Indexable} method: for example when the SRCH2 search server
 * completes an insertion after the method {@link Indexable#insert(org.json.JSONArray)} is called,
 * the method {@link com.srch2.android.sdk.Indexable#onInsertComplete(int, int, String)} will be
 * executed on that {@code Indexable} indicating the number of successful and failed insertions.
 * These methods should be overridden by the implementation of the {@code Indexable}, although
 * they do not have to be: if they are not they will out to the logcat under the tag 'SRCH2'.
 * <br><br>
 * Note that instances of {@link com.srch2.android.sdk.SQLiteIndexable} do not contain the callbacks
 * {@link com.srch2.android.sdk.Indexable#onInsertComplete(int, int, String)},
 * {@link Indexable#onUpdateComplete(int, int, int, String)},
 * {@link Indexable#onDeleteComplete(int, int, String)},
 * and {@link Indexable#onGetRecordComplete(boolean, org.json.JSONObject, String)} as they are only searchable.
 * <br><br>
 * For search requests, when the SRCH2 search completes the search, the results will be passed
 * through the method {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
 * to the implementation of the {@code SearchResultsListener} set by calling
 * {@link #setSearchResultsListener(SearchResultsListener)}. This callback will be executed off the Ui thread
 * unless the search result listener is registered with {@link #setSearchResultsListener(SearchResultsListener, boolean)}
 * passing <b>true</b> for {@code callbackToUiThread} in which case the callback will be executed on the
 * Ui thread.
 * <br><br>
 * It is also possible to harness the power of the sophisticated search functionality of the SRCH2 search server
 * through the SRCH2 Android SDK and its API by using the {@link Query} class.
 * <br><br>
 * <b>When starting an activity utilizing the {@code SRCH2Engine} the order of calls to initialize should
 * go</b>:
 * <br>
 * &nbsp&nbsp&nbsp&nbsp{@link #setAutomatedTestingMode(boolean)} (<i>optional</i>)
 * &nbsp&nbsp&nbsp&nbsp{@link #setIndexables(Indexable, Indexable...)} (<i>required</i> if there are any Indexables) <br>
 * &nbsp&nbsp&nbsp&nbsp{@link #setSQLiteIndexables(SQLiteIndexable, SQLiteIndexable...)}  (<i>required</i> if there are any SQLiteIndexables) <br>
 * &nbsp&nbsp&nbsp&nbsp{@link #setSearchResultsListener(SearchResultsListener)} (<i>required</i> for search results)
 * &nbsp&nbsp&nbsp&nbsp{@link #onResume(android.content.Context)} (<i>required</i>)
 * <br><br>
 * &nbsp&nbsp&nbsp&nbsp{@link #onPause(android.content.Context)} (<i>required</i>) <br><br>
 * These are grouped by association and should be called in the corresponding {@code Activity} life-cycle callback.
 * Specifically at least one {@code Indexable} or {@code SQLiteIndexable} must be set before the call to
 * {@link #onResume(android.content.Context)} is made, or that method call will do nothing but
 * print an error message to the logcat under the tag 'SRCH2'.
 * <br><br>
 * The {@code SRCH2Engine} can also be accessed from a service if {@link #onResume(android.content.Context)} is called
 * after {@link android.app.Service#onStartCommand(android.content.Intent, int, int)} or
 * {@link android.app.Service#onBind(android.content.Intent)} and {@link #onPause(android.content.Context)}
 * after {@link android.app.Service#onDestroy()}. To avoid leaking memory or context, references to instances of
 * {@code Indexable}, {@code SQLiteIndexable} and
 * {@code SearchResultsListener} are cleared when {@link #onPause(android.content.Context)} is called, so they must be
 * set before each call to {@link #onResume(android.content.Context)}.
 * <br><br>
 * Search on!
 */
final public class SRCH2Engine {

    private static final String TAG = "SRCH2Engine";

    static class ExceptionMessages {
        /** Will be message of IOException when port already in use. */
        static final String IO_EXCEPTION_EADDRINUSE_ADDRESS_ALREADY_IN_USE = "EADDRINUSE (Address already in use)";
        /** Will be message of IOException when Internet Permission missing from manifest. */
        static final String IO_EXCEPTION_EACCES_PERMISSION_DENIED = "EACCES (Permission denied)";
        /** Will be message of IOException when server crashes. */
        static final String IO_EXCEPTION_ECONNREFUSED_CONNECTION_REFUSED = "ECONNREFUSED (Connection refused)";
    }


    /** Note: these two lists are only used between calls to {@link #setIndexables(Indexable, Indexable...)}/
     * {@link #setSQLiteIndexables(SQLiteIndexable, SQLiteIndexable...)} and {@link #initialize()} and are
     * dereference-ed afterwards.
     */
    static ArrayList<Indexable> indexablesUserSets;
    static ArrayList<SQLiteIndexable> sqliteIndexablesUserSets;


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
     * If JUnit or Android automated tests are to be run, calling this method and passing <b>true</b>
     * will cause the {@code SRCH2Engine} to immediately stop the SRCH2 search server if it is running
     * anytime {@link #onPause(android.content.Context)} is called instead of waiting to do so after
     * a delay.
     * <br><br>
     * Developers performing automated testing <i>should</i> call this method before running any tests.
     * @param isTestingMode toggles whether the {@code SRCH2Engine} should run in testing mode
     */
    public static void setAutomatedTestingMode(boolean isTestingMode) {
        isDebugAndTestingMode = isTestingMode;
    }

    static SearchResultsListener getSearchResultsObserver() {
        return searchResultsObserver;
    }

    /**
     * Registers the implementation of the interface {@code SearchResultsListener} for receiving
     * the results of a search performed by the SRCH2 search server. This can be reset at anytime, and
     * although it is not required to be set, it is the only way to get search results within the API.
     * <br><br>
     * The callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * can be executed on a background thread, enabling post-processing operations, or can be executed
     * on the Ui thread. If it is not executed on the Ui thread, the search results will have to be
     * pushed the Ui thread before altering any of the Ui (such as when invalidating the adapter of
     * the {@code ListView} showing the search results).
     * @param searchResultsListener the implementation of {@code SearchResultsListener} that will
     *                              receive search results
     * @param callbackToUiThread whether to push the search results to the Ui thread
     */
    public static void setSearchResultsListener(
            SearchResultsListener searchResultsListener, boolean callbackToUiThread) {
        searchResultsObserver = searchResultsListener;
        searchResultsPublishedToUiThread = callbackToUiThread;
    }

    /**
     * Registers the implementation of the interface {@code SearchResultsListener} for receiving
     * the results of a search performed by the SRCH2 search server. This can be reset at anytime, and
     * although it is not required to be set, it is the only way to get search results within the API.
     * <br><br>
     * The callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * can be executed on a background thread, enabling post-processing operations, or can be executed
     * on the Ui thread. If it is not executed on the Ui thread, the search results will have to be
     * pushed the Ui thread before altering any of the Ui (such as when invalidating the adapter of
     * the {@code ListView} showing the search results). By default, this callback will not be
     * called on the Ui thread. Use {@link #setSearchResultsListener(SearchResultsListener, boolean)}
     * to enable pushing search results to the Ui thread.
     * @param searchResultsListener the implementation of {@code SearchResultsListener} that will
     *                              receive search results
     */
    public static void setSearchResultsListener(
            SearchResultsListener searchResultsListener) {
        Cat.d(TAG, "%%%%%%%%%%%%%%%%%%%%adding search result listener!!!");
        searchResultsPublishedToUiThread = false;
        searchResultsObserver = searchResultsListener;
    }

    /**
     * Registers any instances of {@link com.srch2.android.sdk.Indexable} that represent indexes in the SRCH2 search server.
     * <br><br>
     * This method (or {@link #setSQLiteIndexables(SQLiteIndexable, SQLiteIndexable...)})<b> must be called before
     * every call</b> to {@link #onResume(android.content.Context)} and at least one
     * {@code Indexable} or {@code SQLiteIndexable} be passed; this call should be made from
     * {@link android.app.Activity#onResume()} for an {@code Activity} utilizing search.
     * @param firstIndexable at least one {@code Indexable} representing an index in the SRCH2 search server
     * @param additionalIndexables any additional {@code Indexable} instances representing indexes in the SRCH2 search server
     */
    public static void setIndexables(Indexable firstIndexable, Indexable... additionalIndexables) {
        Cat.d(TAG, "settingIndexables");
        indexablesUserSets = new ArrayList<Indexable>();
        if (firstIndexable != null) {
            Cat.d(TAG, "settingIndexables -adding first");
            indexablesUserSets.add(firstIndexable);
        }
        if (additionalIndexables != null) {
            for (Indexable idx : additionalIndexables) {
                if (idx != null) {
                    indexablesUserSets.add(idx);
                }
            }
        }
    }

    /**
     * Registers any instances of {@link com.srch2.android.sdk.SQLiteIndexable} that represent indexes in the SRCH2 search server.
     * <br><br>
     * This method (or {@link #setIndexables(Indexable, Indexable...)}) <b>must be called before
     * every call</b> to {@link #onResume(android.content.Context)} and at least one
     * {@code Indexable} or {@code SQLiteIndexable} be passed; this call should be made from
     * {@link android.app.Activity#onResume()} for an {@code Activity} utilizing search.
     * @param firstSQLiteIndexable at least one {@code SQLiteIndexable} representing an index in the SRCH2 search server
     * @param additionalSQLiteIndexables any additional {@code SQLiteIndexable} instances representing indexes in the SRCH2 search server
     */
    public static void setSQLiteIndexables(SQLiteIndexable firstSQLiteIndexable, SQLiteIndexable... additionalSQLiteIndexables) {
        Cat.d(TAG, "settingSQLiteIndexables");
        sqliteIndexablesUserSets = new ArrayList<SQLiteIndexable>();
        if (firstSQLiteIndexable != null) {
            Cat.d(TAG, "settingSQLiteIndexables -adding first");
            sqliteIndexablesUserSets.add(firstSQLiteIndexable);
        }
        if (additionalSQLiteIndexables != null) {
            for (SQLiteIndexable idx : additionalSQLiteIndexables) {
                if (idx != null) {
                    sqliteIndexablesUserSets.add(idx);
                }
            }
        }
    }

    /**
     * Causes the SRCH2 search server powering the search to start after initializing any
     * {@code Indexable} or {@code SQLiteIndexable} instances registered. This method
     * must be preceded by {@link #setIndexables(Indexable, Indexable...)} and
     * {@link #setSQLiteIndexables(SQLiteIndexable, SQLiteIndexable...)} each time it is called. If no
     * {@code Indexable} or {@code SQLiteIndexable} instances are set, this call will do nothing but
     * print an error message to the logcat under the tag 'SRCH2'.
     * <br><br>
     * This method should be called anytime the activity requiring search functionality comes to the
     * foreground and is visible--that is, when it can be interacted with by a user who might perform searches.
     * Thus it should be typically called from {@link android.app.Activity#onResume()}.
     * <br><br>
     * Starting the SRCH2 search server is fast, usually taking under a second, and when it comes online
     * and is ready to handle search requests the callback method {@link Indexable#onIndexReady()} and
     * {@link SQLiteIndexable#onIndexReady()}
     * will
     * be called for each index as it becomes loaded and ready for access. If the SRCH2 search server was
     * already running, such as when the user leaves the search activity for a moment to check their
     * contacts, this call-back will happen almost immediately.
     * <br><br>
     * Checking
     * whether the SRCH2 search server is ready can also determined by calling
     * {@link #isReady()}.
     * <br><br>
     * An {@code context} instance is needed here to start the remote service that hosts the SRCH2 search
     * server process. A
     * reference to this context is not kept.
     * <br><br>
     * @param context needed to start a remote service, any context will do
     */
    public static void onResume(Context context) {
        Cat.d(TAG, "onResume");
        if (!checkResumeReady()) {
            return;
        }

        registerReciever(context);

        initialize();
        initializeConfiguration(context);
        startSRCH2Service(context, SRCH2Configuration.generateConfigurationFileString(SRCH2Engine.conf));
        isStarted = true;
    }

    /**
     * Causes the SRCH2 search server powering the search to start after initializing any
     * {@code Indexable} or {@code SQLiteIndexable} instances registered. This method
     * must be preceded by {@link #setIndexables(Indexable, Indexable...)} and
     * {@link #setSQLiteIndexables(SQLiteIndexable, SQLiteIndexable...)} each time it is called. If no
     * {@code Indexable} or {@code SQLiteIndexable} instances are set, this call will do nothing but
     * print an error message to the logcat under the tag 'SRCH2'.
     * <br><br>
     * This method should be called anytime the activity requiring search functionality comes to the
     * foreground and is visible--that is, when it can be interacted with by a user who might perform searches.
     * Thus it should be typically called from {@link android.app.Activity#onResume()}.
     * <br><br>
     * Starting the SRCH2 search server is fast, usually taking under a second, and when it comes online
     * and is ready to handle search requests the callback method {@link Indexable#onIndexReady()} and
     * {@link SQLiteIndexable#onIndexReady()}
     * will
     * be called for each index as it becomes loaded and ready for access. If the SRCH2 search server was
     * already running, such as when the user leaves the search activity for a moment to check their
     * contacts, this call-back will happen almost immediately.
     * <br><br>
     * Checking
     * whether the SRCH2 search server is ready can also determined by calling
     * {@link #isReady()}.
     * <br><br>
     * An {@code context} instance is needed here to start the remote service that hosts the SRCH2 search
     * server process. A
     * reference to this context is not kept.
     * <br><br>
     * For convenience, a {code SearchResultsListener} may be registered from this overload.
     * @param context needed to start a remote service, any context will do
     * @param searchResultsListener the implementation of {@code SearchResultsListener} that will
     *                              receive search results
     * @param callbackToUiThread whether to push the search results to the Ui thread
     */
    public static void onResume(Context context, SearchResultsListener searchResultsListener,  boolean callbackToUiThread) {
        if (!checkResumeReady()) {
            return;
        }
        Cat.d(TAG, "onResume");
        registerReciever(context);
        setSearchResultsListener(searchResultsListener, callbackToUiThread);

        initialize();
        initializeConfiguration(context);
        startSRCH2Service(context, SRCH2Configuration.generateConfigurationFileString(SRCH2Engine.conf));
        isStarted = true;
    }

    private static boolean checkResumeReady() {
        boolean resumeReady = true;

        if (sqliteIndexablesUserSets == null) {
            Cat.d(TAG, "sqliteIndexableUserSet NULL");
        }
        if (indexablesUserSets == null) {
            Cat.d(TAG, "indexableUserset NULL");
        }

        if (sqliteIndexablesUserSets == null && indexablesUserSets == null) {
            resumeReady = false;
            Cat.d(TAG, "both sets null");
        } else if ((sqliteIndexablesUserSets != null && sqliteIndexablesUserSets.size() < 1)
                    && (indexablesUserSets != null && indexablesUserSets.size() < 1)) {
            resumeReady = false;
            Cat.d(TAG, "one of the sets not of size");
        }

        if (!resumeReady) {
            Log.d("SRCH2", "SRCH2Engine error - no indexables/sqlitindexables set before call to SRCH2Engine.onResume(). " +
                    "At least one indexable must be set before calling SRCH2Engine.onResume()");
        }
        return resumeReady;
    }

    /**
     * Sets the authorization key that is required for the SRCH2 search server to perform any command or task.
     * <br><br>
     * If this key is specified, each valid HTTP request needs to provide the following key-value pair in order to get the authorization.
     * OAuth=foobar
     * Example: curl -i "HTTP://localhost:8081/search?q=terminator&OAuth=foobar"
     * <br><br>
     * If this key is not specified, it will be automatically generated by the {@code SRCH2Engine}.
     * <br><br>
     * This method should be called after {@link #onResume(android.content.Context)}.
     * @param authorizationKey the key that any request on the SRCH2 search server will have to supply in order for the
     *                         SRCH2 search server to carry out the command or task
     */
    public static void setAuthorizationKey(String authorizationKey) {
        if (getConfig() == null) {
            Log.d("SRCH2", "SRCH2 Engine error - tried to set authorization key before calling SRCH2Engine.onResume()");
            return;
        }
        conf.setAuthorizationKey(authorizationKey);
    }

    /**
     * Determines if the SRCH2 search server is available and accessable. This method should return <b>true</b>
     * a short time after {@link #onResume(android.content.Context)}
     * is called for the first time in an app's life-cycle (it takes a moment or two for the SRCH2 engine to come online)
     * and remain <b>true</b> until
     * {@link #onPause(android.content.Context)} is called. Search requests made before the SRCH2 search server
     * comes online are cached, and the latest search input will be sent as a search request as soon it comes
     * online; requests editing an {@link com.srch2.android.sdk.Indexable} such as an insert will be queued
     * until the SRCH2 search server is ready to process them.
     *
     * @return if the {@code SRCH2Engine} is ready or not.
     */
    public static boolean isReady() {
        return isReady.get();
    }

    /**
     * Tells the {@code SRCH2Engine} to bring the SRCH2 search server to a stop. The SRCH2
     * search server will not halt immediately, but instead wait a short duration in case the user
     * navigates back to the activity requiring search. In that case, the command to stop is
     * cancelled as long as {@link #onResume(android.content.Context)} is called again from, for example,
     * {@link android.app.Activity#onResume()}.
     * <br><br>
     * This method should typically be called from {@link android.app.Activity#onPause()}.
     * <br><br>
     * Since the SRCH2 search server is hosted by a remote service, a context is needed
     * to stop this service. After calling this method, {@link #isReady()} will
     * return false and no subsequent actions can be performed on the {@code SRCH2Engine} until
     * {@link #onResume(android.content.Context)} is called again. Pending tasks however,
     * such as batch inserts, will not be interrupted and be allowed to finish.
     * @param context needed to stop a remote service, any context will do
     */
    public static void onPause(Context context) {
        Cat.d(TAG, "onPause");
        if (!isStarted) {
            return;
        }
        ArrayList<Indexable> dirtyIndexList = new ArrayList<Indexable>();
        for (IndexableCore idx : conf.indexableMap.values()) {
            if (Indexable.class.isAssignableFrom(idx.getClass())) {
                Cat.d(TAG, "indexable " + idx.getIndexName() + " was dirty adding to save task");
                dirtyIndexList.add((Indexable) idx);
            } else {
                Cat.d(TAG, "indexable " + idx.getIndexName() + " was not dirty NOT adding to save task");
            }
        }
        if (dirtyIndexList.size() > 0 && !isDebugAndTestingMode) {
            MultiSaveTask mst = new MultiSaveTask(dirtyIndexList);
            HttpTask.executeTask(mst);
        }
        if (searchResultsUiCallbackHandler != null) {
            searchResultsUiCallbackHandler = null;
        }
        if (searchResultsObserver != null) {
            searchResultsObserver = null;
        }
        if (getConfig().indexableMap != null && getConfig().indexableMap.size() > 0) {
            getConfig().indexableMap.clear();
        }
        lastQuery.set(new IndexQueryPair(null, null));
        stopExecutable(context);
        unregisterReciever(context);
        isReady.set(false);
        isStarted = false;
    }

    static void initialize() {
        SRCH2Engine.isStarted = false;
        SRCH2Engine.isReady.set(false);
        SRCH2Engine.isChanged.set(false);
        SRCH2Engine.allIndexSearchTask = null;

        ArrayList<IndexableCore> indexes = new ArrayList<IndexableCore>();
        if (indexablesUserSets != null) {
            indexes.addAll(indexablesUserSets);
        }
        if (sqliteIndexablesUserSets != null) {
            indexes.addAll(sqliteIndexablesUserSets);
        }
        if (indexes.size() < 1) {
            Log.d("SRCH2", "SRCH2Engine error - no indexables/sqlitindexables set before call to SRCH2Engine.onResume(). " +
                    "At least one indexable must be set before calling SRCH2Engine.onResume()");
        }

        SRCH2Engine.conf = new SRCH2Configuration(indexes);
        indexablesUserSets = null;
        sqliteIndexablesUserSets = null;
    }

    // moved out for testing of remote service
    static void initializeConfiguration(Context c) {
        conf.setPort(detectFreePort());
        String appFilesDirectory = detectAppFilesDirectory(c);
        Cat.d(TAG, "app files directory is : " + appFilesDirectory);
        conf.setSRCH2Home(appFilesDirectory);
    }

    // moved out for testing of remote service
    static void registerReciever(Context c) {
        incomingIntentReciever = new SRCH2EngineBroadcastReciever();
        c.registerReceiver(incomingIntentReciever, IPCConstants
                .getSRCH2EngineBroadcastRecieverIntentFilter(c));
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
        URL pingUrl = null;
        IndexableCore defaultIndexable;
        Iterator<IndexableCore> it = SRCH2Engine.conf.indexableMap.values().iterator();
        if (it.hasNext()) {
            defaultIndexable = it.next();
            pingUrl = UrlBuilder
                    .getInfoUrl(
                            SRCH2Engine.conf,
                            defaultIndexable.indexInternal.indexDescription);
        }
        i.putExtra(IPCConstants.INTENT_KEY_PING_URL,
                pingUrl == null ? UrlBuilder.getShutDownUrl(getConfig()).toString() : pingUrl.toString());
        i.putExtra(IPCConstants.INTENT_KEY_PORT_NUMBER, conf.getPort());
        i.putExtra(IPCConstants.INTENT_KEY_OAUTH, conf.getAuthorizationKey());
        i.putExtra(IPCConstants.INTENT_KEY_IS_DEBUG_AND_TESTING_MODE, isDebugAndTestingMode);
        context.startService(i);
        HttpTask.onStart();
    }

    private static void startCheckCoresLoadedTask(boolean isCheckingAfterCrash) {
        Cat.d(TAG, "startCheckCoresLoadedTask");
        HashMap<String, URL> indexUrlMap = new HashMap<String, URL>();
        for (IndexableCore index : conf.indexableMap.values()) {
            indexUrlMap.put(index.indexInternal.getIndexCoreName(),
                    UrlBuilder.getInfoUrl(conf, index.indexInternal.getConf()));
        }
        CheckCoresLoadedTask task = new CheckCoresLoadedTask(indexUrlMap, isCheckingAfterCrash);
        HttpTask.executeTask(task);
    }

    private static void stopExecutable(final Context context) {
        Cat.d(TAG, "stopExecutable");
        Intent i = new Intent(
                IPCConstants
                        .getSRCH2ServiceBroadcastRecieverIntentAction(context));
        i.putExtra(IPCConstants.INTENT_KEY_BROADCAST_ACTION,
                IPCConstants.INTENT_VALUE_BROADCAST_ACTION_START_AWAITING_SHUTDOWN);
        context.sendBroadcast(i);
        HttpTask.onStop();
    }

    static void unregisterReciever(Context context) {
        try {
            context.unregisterReceiver(incomingIntentReciever);
        } catch (IllegalArgumentException ignore) {
        }
    }

    static void reQueryLastOne() {
        Cat.d(TAG, "reQueryLastOne");
        IndexQueryPair pair = lastQuery.get();
        if (pair != null && pair.query != null && pair.query.length() > 0) {
            if (pair.indexName != null) {
                getConfig().indexableMap.get(pair.indexName).search(pair.query);
            } else {
                searchAllRawString(pair.query);
            }
        }
    }

    static boolean validateSearchInput(String searchInput) {
        if (getConfig() == null) {
            return false;
        }
        if (searchInput == null) {
            return false;
        } else if (searchInput.length() < 1) {
            if (lastQuery != null) {
                lastQuery.get().indexName = null;
                lastQuery.get().query = null;
            }
            return false;
        } else {
            return true;
        }
    }

    private static void searchAllRawString(String rawQueryString) {
        lastQuery.set(new IndexQueryPair(null, rawQueryString));
        if (allIndexSearchTask != null) {
            allIndexSearchTask.cancel();
        }
        allIndexSearchTask = new SearchTask(UrlBuilder.getSearchUrl(conf, null,
                                                rawQueryString), searchResultsObserver);
        HttpTask.addToQueue(allIndexSearchTask);
    }

    /**
     * Searches all {@code Indexable}s and {@code SQLiteIndexable}s there were registered
     * in {@link #setIndexables(Indexable, Indexable...)} and
     * {@link #setSQLiteIndexables(SQLiteIndexable, SQLiteIndexable...)}.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * {@link SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * will be triggered. The
     * {@code HashMap resultMap} will contain the search results in the form of {@code 
     * JSONObject}s as they were originally inserted (and updated).
     * <br><br>
     * This method will do nothing if the value of {@code searchInput} is null, an empty
     * string or {@link #onResume(android.content.Context)} has not been called yet.
     * @param searchInput the textual input to search on
     */
    public static void searchAllIndexes(String searchInput) {
        Cat.d(TAG, "searchAllIndexes");
        if (validateSearchInput(searchInput)) {
            String rawString = IndexInternal.formatDefaultQueryURL(searchInput);
            searchAllRawString(rawString);
        }
    }

    /**
     * Performs an advanced searches on all {@code Indexable}s and {@code SQLiteIndexable}s there were registered
     * in {@link #setIndexables(Indexable, Indexable...)} and
     * {@link #setSQLiteIndexables(SQLiteIndexable, SQLiteIndexable...)}.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * {@link SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * will be triggered. The
     * {@code resultMap} will contain the search results in the form of {@code 
     * JSONObject}s as they were originally inserted (and updated).
     * <br><br>
     * This method will do nothing if the value of {@code searchInput} is null, an empty
     * string or {@link #onResume(android.content.Context)} has not been called yet.
     * @param query the formation of the advanced search
     */
    public static void advancedSearchOnAllIndexes(Query query) {
        if (query == null) {
            return;
        }
        Cat.d(TAG, "searchAllIndexes");
        String rawString = query.toString();
        searchAllRawString(rawString);
    }

    /**
     * Returns the {@code Indexable} representing the index that matches the value of
     * {@code indexName} as it was defined in {@link Indexable#getIndexName()}. This method is a
     * convenience getter, offering static access
     * to the {@code Indexable} references as they were registered with {@link #setIndexables(Indexable, Indexable...)}.
     * By obtaining the {@code Indexable}, its methods can be called to search, insert, update or
     * perform any other {@code Indexable} function.
     * @param indexName the name of the index as defined by the {@code Indexable} implementation of {@code getIndexName()}
     * @return the {@code Indexable} representing the index with the specified {@code indexName}
     */
    public static Indexable getIndex(String indexName) {
        checkConfIsNullThrowIfIs();
        return conf.getIndexableAndThrowIfNotThere(indexName);
    }

    /**
     * Returns the {@code SQLiteIndexable} representing the index that matches the value of
     * {@code indexName} as it was defined in the {@link SQLiteIndexable#getIndexName()}.
     * This method is a convenience getter, offering static access
     * to the {@code SQLiteIndexable} references as they were registered with
     * {@link #setSQLiteIndexables(SQLiteIndexable, SQLiteIndexable...)}.
     * @param indexName the name of the index as defined by the {@code SQLiteIndexable} implementation of {@code getIndexName()}
     * @return the {@code SQLiteIndexable} representing the index with the specified {@code indexName}
     */
    public static SQLiteIndexable getSQLiteIndex(String indexName) {
        checkConfIsNullThrowIfIs();
        return conf.getSqliteIndexableAndThrowIfNotThere(indexName);
    }

    static SRCH2Configuration getConfig() {
        return conf;
    }

    static String detectAppFilesDirectory(Context context) {
        return context.getApplicationContext().getFilesDir().getAbsolutePath();
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

    static void checkConfIsNullThrowIfIs() {
        if (conf == null) {
            throw new NullPointerException(
                    "Cannot start SRCH2Engine without configuration being set.");
        }
    }

    static private class SRCH2EngineBroadcastReciever extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Cat.d(TAG, " SRCH2EngineBroadcastReciever onReceive");
            String broadCastCommand = intent.getStringExtra(IPCConstants.INTENT_KEY_BROADCAST_ACTION);
            if (broadCastCommand != null) {
                if (broadCastCommand.equals(IPCConstants.INTENT_VALUE_BROADCAST_ACTION_VALIDATE_SRCH2ENGINE_ALIVE)) {
                    Cat.d(TAG, " SRCH2EngineBroadcastReciever INTENT_VALUE_BROADCAST_ACTION_VALIDATE_SRCH2ENGINE_ALIVE");
                    Intent i = new Intent(
                            IPCConstants
                                    .getSRCH2ServiceBroadcastRecieverIntentAction(context));
                    i.putExtra(IPCConstants.INTENT_KEY_BROADCAST_ACTION,
                            IPCConstants.INTENT_VALUE_BROADCAST_ACTION_VALIDATE_SRCH2ENGINE_ALIVE);
                    context.sendBroadcast(i);
                } else if (broadCastCommand.equals(IPCConstants.INTENT_VALUE_BROADCAST_ACTION_VALIDATE_SRCH2ENGINE_ALIVE_FOR_PING)) {
                    Cat.d(TAG, " SRCH2EngineBroadcastReciever INTENT_VALUE_BROADCAST_ACTION_VALIDATE_SRCH2ENGINE_ALIVE_FOR_PING");
                    Intent i = new Intent(
                            IPCConstants
                                    .getSRCH2ServiceBroadcastRecieverIntentAction(context));
                    i.putExtra(IPCConstants.INTENT_KEY_BROADCAST_ACTION,
                            IPCConstants.INTENT_VALUE_BROADCAST_ACTION_VALIDATE_SRCH2ENGINE_ALIVE_FOR_PING);
                    context.sendBroadcast(i);
                } else if (broadCastCommand.equals(IPCConstants.INTENT_VALUE_BROADCAST_ACTION_ENGINE_CRASHED_BUT_CAN_RESUME)
                             || broadCastCommand.equals(IPCConstants.INTENT_VALUE_BROADCAST_ACTION_ENGINE_STARTED_PROCEED)) {
                    Cat.d(TAG, " SRCH2EngineBroadcastReciever - proceed or resume");
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
                    boolean isResumingAfterCrash = false;
                    if (broadCastCommand.equals(IPCConstants.INTENT_VALUE_BROADCAST_ACTION_ENGINE_CRASHED_BUT_CAN_RESUME)) {
                        Cat.d(TAG, "SRCH2EngineBroadcastReciever onReceive - resume after crash");
                        isResumingAfterCrash = true;
                    } else if (broadCastCommand.equals(IPCConstants.INTENT_VALUE_BROADCAST_ACTION_ENGINE_STARTED_PROCEED)) {
                        Cat.d(TAG, "SRCH2EngineBroadcastReciever onReceive - proceed");
                        isResumingAfterCrash = false;
                    }
                    SRCH2Engine.startCheckCoresLoadedTask(isResumingAfterCrash);
                }
            }
        }
    }

    static class IndexQueryPair {
        public String indexName = null;
        public String query = null;

        public IndexQueryPair(String indexName, String query) {
            Cat.d(TAG, "IndexQueryPair()");
            this.indexName = indexName;
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
