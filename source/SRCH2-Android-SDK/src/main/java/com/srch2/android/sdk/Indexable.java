package com.srch2.android.sdk;

import android.util.Log;
import org.json.JSONArray;
import org.json.JSONObject;

/**
 * Represents an index in the SRCH2 search server. For every index to be searched on, users of the
 * SRCH2 Android SDK should implement a separate subclass instance of this class. This class contains
 * methods for performing CRUD actions on the index such as insertion and searching. Note that it is
 * always possible to retrieve a reference to a specific <code>Indexable</code> from the static method
 * {@link com.srch2.android.sdk.SRCH2Engine#getIndex(String)}  where <code>
 * indexName</code> matches the return value of {@link #getIndexName()}.
 * <br><br>
 * <b>For each implementation of this class, it is necessary</b> to override the two methods:
 * {@link #getIndexName()} and {@link #getSchema()} which determine the basic configuration for the index
 * as it resides in the SRCH2 search server.
 * <br><br>
 * In addition, each implementation can optionally chose to override the methods {@link #getTopK()}
 * (which sets the number of search results returned per search) and {@link #getFuzzinessSimilarityThreshold()}
 * (which determines the number of wildcard substitutions that can occur per search input string). If
 * not overridden, these will take the default values {@link #DEFAULT_NUMBER_OF_SEARCH_RESULTS_TO_RETURN_AKA_TOPK} and
 * {@link #DEFAULT_FUZZINESS_SIMILARITY_THRESHOLD} respectively.
 * <br><br>
 * The methods {@link #onInsertComplete(int, int, String)}, {@link #onUpdateComplete(int, int, int, String)},
 * {@link #onDeleteComplete(int, int, String)}, and {@link #onGetRecordComplete(boolean, org.json.JSONObject, String)}
 * can also be overridden. These methods will be executed after the SRCH2 search server completes the corresponding
 * action as caused by the corresponding <code>Indexable</code> method calls (for instance,
 * {@link #insert(org.json.JSONArray)} will trigger {@link #onInsertComplete(int, int, String)} when the
 * SRCH2 search server completes the insertion).
 * <br><br>
 * There is also one method that returns the number of records in the index: {@link #getRecordCount()}. The
 * value this method will return will be updated each time the SRCH2 search server comes online and each time an
 * insert, upsert or delete occurs. Note it can return {@link #INDEX_RECORD_COUNT_NOT_SET} if the SRCH2 search server
 * is not online such as when {@link com.srch2.android.sdk.SRCH2Engine#initialize(Indexable, Indexable...)}
 * has been called but {@link com.srch2.android.sdk.SRCH2Engine#onStart(android.content.Context)} has not yet been
 * called).
 */
public abstract class Indexable {

    /**
     * The JSON key to use to retrieve each original record from each <code>JSONObject</code> in
     * the <code>ArrayList<JSONObject</code> of the callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}.
     * <br><br>
     * Has the <b>constant</b> value '<code>record</code>';
     */
    public static final String SEARCH_RESULT_JSON_KEY_RECORD = "record";

    /**
     * The JSON key to use to retrieve each set of highlighted fields from each record from each <code>JSONObject</code> in
     * the <code>ArrayList<JSONObject</code> of the callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}.
     * <br><br>
     * Has the <b>constant</b> value '<code>highlighted</code>';
     */
    public static final String SEARCH_RESULT_JSON_KEY_HIGHLIGHTED = "highlighted";


    IndexInternal indexInternal;

    /**
     * The value the <code>JSONObject</code> passed in {@link #onInsertComplete(int, int, String)},
     * {@link #onGetRecordComplete(boolean, org.json.JSONObject, String)} will be set to if a connection
     * to the SRCH2 search server could not be established.
     * <br><br>
     * Has the <b>constant</b> value 'Connection failed without known cause';
     */
    public static final String IRRECOVERABLE_NETWORK_ERROR_MESSAGE = "Connection failed without known cause.";

    /**
     * Implementing this method sets the name of the index this <code>Indexable</code> represents.
     * @return the name of the index this <code>Indexable</code> represents
     */
    abstract public String getIndexName();

    /**
     * Implementing this method sets the schema of the index this <code>Indexable</code> represents. The schema
     * defines the data fields of the index, much like the table structure of an SQLite database table. See
     * {@link Schema} for more information.
     * @return the schema to define the index structure this <code>Indexable</code> represents
     */
    abstract public Schema getSchema();

    /**
     * If returned from {@link #getRecordCount()} indicates this value has not yet been set.
     * <br><br>
     * Has the <b>constant</b> value of <code>-1</code>.
     */
    public static final int INDEX_RECORD_COUNT_NOT_SET = -1;

    private int numberOfDocumentsInTheIndex = INDEX_RECORD_COUNT_NOT_SET;
    /**
     * Returns the number of records that are currently in the index that this
     * <code>Indexable</code> represents.
     * @return the number of records in the index
     */
    public final int getRecordCount() {
        return numberOfDocumentsInTheIndex;
    }

    final void setRecordCount(int recordCount) {
        numberOfDocumentsInTheIndex = recordCount;
    }

    /**
     * The default value for the numbers of search results to return per search request.
     * <br><br>
     * Has the <b>constant</b> value of <code>10</code>.
     */
    public static final int DEFAULT_NUMBER_OF_SEARCH_RESULTS_TO_RETURN_AKA_TOPK = 10;

    /**
     * The default value for the fuzziness similarity threshold. Approximately one character
     * per every three characters will be allowed to act as a wildcard during a search.
     * <br><br>
     * Has the <b>constant</b> value of <code>0.65</code>.
     */
    public static final float DEFAULT_FUZZINESS_SIMILARITY_THRESHOLD = 0.65f;

    /**
     * Override this method to set the number of search results to be returned per query or search task.
     * <br><br>
     * The default value of this method, if not overridden, is ten.
     * <br><br>
     * This method will cause an <code>IllegalArgumentException</code> to be thrown when calling
     * {@link SRCH2Engine#initialize(Indexable, Indexable...)} and passing this
     * <code>Indexable</code> if the returned value
     *  is less than one.
     * @return the number of results to return per search
     */
    public int getTopK() {
        return DEFAULT_NUMBER_OF_SEARCH_RESULTS_TO_RETURN_AKA_TOPK;
    }

    /**
     * Set the default fuzziness similarity threshold. This will determine how many character
     * substitutions the original search input will match search results for: if set to 0.5,
     * the search performed will include results as if half of the characters of the original
     * search input were replaced by wild card characters.
     * <br><br>
     * <b>Note:</b> In the the formation of a {@link Query}, each <code>Term</code> can
     * have its own fuzziness similarity threshold value set by calling the method
     * {@link SearchableTerm#enableFuzzyMatching(float)}; by default it is disabled for terms.
     * <br><br>
     * The default value of this method, if not overridden, is 0.65.
     * <br><br>
     * This method will cause an <code>IllegalArgumentException</code> to be thrown when calling
     * {@link SRCH2Engine#initialize(Indexable, Indexable...)} and passing this <code>Indexable</code> if the
     * value returned is less than zero or greater than one.
     * @return the fuzziness similarity ratio to match against search keywords against
     */
    public float getFuzzinessSimilarityThreshold() { return DEFAULT_FUZZINESS_SIMILARITY_THRESHOLD; }

    /**
     * Inserts the specified <code>JSONObject record</code> into the index that this
     * <code>Indexable</code> represents. This <code>JSONObject</code> should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema; the values of these keys should match the type defined in the index's
     * schema as well.
     * <br><br>
     * After the SRCH2 search server completes the insertion task, the callback method of the
     * {@link #onInsertComplete(int, int, String)}
     * will be triggered containing the status of the
     * completed insertion task.
     * @param record the <code>JSONObject</code> representing the record to insert
     */
    public final void insert(JSONObject record) {
        if (indexInternal != null) {
            indexInternal.insert(record);
        }
    }

    /**
     * Inserts the set of records represented as <code>JSONObject</code>s contained in the
     * specified <code>JSONArray records</code>. Each <code>JSONObject</code> should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema; the values of these keys should match the type defined in the index's
     * schema as well. The <code>JSONArray records</code> should also be properly formed
     * and contain only the set of <code>JSONObect</code>s that represent the records to be
     * inserted.
     * <br><br>
     * After the SRCH2 search server completes the insertion task, the callback method of the
     * {@link #onInsertComplete(int, int, String)}
     * will be triggered containing the status of the
     * completed insertion task.
     * @param records the <code>JSONArray</code> containing the set of <code>JSONObject</code>s
     *                representing the records to insert
     */
    public final void insert(JSONArray records) {
        if (indexInternal != null) {
            indexInternal.insert(records);
        }
    }

    /**
     * Updates the specified <code>JSONObject record</code> into the index that this
     * <code>Indexable</code> represents. This <code>JSONObject</code> should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema; the values of these keys should match the type defined in the index's
     * schema as well. If the primary key supplied in the <code>record</code> does not
     * match the primary key of any existing record, the <code>record</code> will be
     * inserted.
     * <br><br>
     * After the SRCH2 search server completes the update task, the callback method of the
     * {@link #onUpdateComplete(int, int, int, String)}
     * will be triggered containing the status of the
     * completed update task.
     * @param record the <code>JSONObject</code> representing the record to upsert
     */
    public final void update(JSONObject record) {
        if (indexInternal != null) {
            indexInternal.update(record);
        }

    }

    /**
     * Updates the set of records represented as <code>JSONObject</code>s contained in the
     * specified <code>JSONArray records</code>. Each <code>JSONObject</code> should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema; the values of these keys should match the type defined in the index's
     * schema as well. The <code>JSONArray records</code> should also be properly formed
     * and contain only the set of <code>JSONObect</code>s that represent the records to be
     * updates. If the primary key of any of the supplied records in the <code>JSONArray</code>
     * does not match the primary key of any existing record, that record will be
     * inserted.
     * <br><br>
     * After the SRCH2 search server completes the update task, the callback method of the
     * {@link #onUpdateComplete(int, int, int, String)}
     *  will be triggered containing the status of the
     * completed update task.
     * @param records the <code>JSONArray</code> containing the set of <code>JSONObject</code>s
     *                representing the records to upsert
     */
    public final void update(JSONArray records) {
        if (indexInternal != null) {
            indexInternal.update(records);
        }
    }

    /**
     * Deletes from the index this record with a primary
     * key matching the value of the <code>primaryKeyOfRecordToDelete</code>. If no record with
     * a matching primary key is found, the index will remain as it is.
     * <br><br>
     * After the SRCH2 search server completes the deletion task, the callback method of the
     * {@link #onDeleteComplete(int, int, String)}
     * will be triggered containing the status of the
     * completed deletion task.
     * @param primaryKeyOfRecordToDelete the primary key of the record to delete
     */
    public final void delete(String primaryKeyOfRecordToDelete) {
        if (indexInternal != null) {
            indexInternal.delete(primaryKeyOfRecordToDelete);
        }
    }

    /**
     * Does a basic search on the index that this <code>Indexable</code> represents. A basic
     * search means that all distinct keywords (delimited by white space) of the
     * <code>searchInput</code> are treated as fuzzy, and the last keyword will
     * be treated as fuzzy and prefix.
     * <br><br>
     * For more configurable searches, use the
     * {@link Query} class in conjunction with the {@link #advancedSearch(Query)}
     * method.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * {@link SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     *  will be triggered. The
     * <code>HashMap</code> argument will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw an exception is the value of <code>searchInput</code> is null
     * or has a length less than one.
     * @param searchInput the textual input to search on
     */
    public final void search(String searchInput) {
        if (indexInternal != null) {
            indexInternal.search(searchInput);
        }
    }

    /**
     * Does an advanced search by forming search request input as a {@link Query}. This enables
     * searches to use all the advanced features of the SRCH2 search engine, such as per term
     * fuzziness similarity thresholds, limiting the range of results to a refining field, and
     * much more. See the {@link Query} for more details.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * {@link SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * will be triggered. The argument
     * <code>HashMap</code> will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw an exception if the value of <code>query</code> is null.
     * @param query the formation of the query to do the advanced search
     */
    public final void advancedSearch(Query query) {
        if (indexInternal != null) {
            indexInternal.advancedSearch(query);
        }
    }

    /**
     * Retrieves a record from the index this <code>Indexable</code> represents if the primary
     * key of any record in the index matches the value of <code>primaryKeyOfRecordToRetrieve</code>.
     * When the SRCH2 search server completes the retrieval task, the method
     * {@link #onGetRecordComplete(boolean, org.json.JSONObject, String)}
     * will be triggered containing the record represented as a <code>JSONObject</code> if found.
     * <br><br>
     * This method will than an <code>IllegalArgumentException</code>if the value of
     * <code>primaryKeyOfRecordToRetrieve</code> is null or has a length less than one.
     * @param primaryKeyOfRecordToRetrieve the primary key
     */
    public final void getRecordbyID(String primaryKeyOfRecordToRetrieve) {
        if (indexInternal != null) {
            indexInternal.getRecordbyID(primaryKeyOfRecordToRetrieve);
        }
    }

    /**
     * Callback executed upon completion of corresponding insert action by SRCH2 search server. After
     * calling either {@link #insert(org.json.JSONObject)} or {@link #insert(org.json.JSONArray)} the
     * SRCH2 search server will attempt to insert the specified records; upon completion, the JSON
     * response will be parsed and this method will be triggered. Hopefully the value of the parameter
     * <code>success</code> will match the number of records inserted; if not, the parameter
     * <code>JSONResponse</code> contains the raw json response as it was received from the SRCH2
     * search server which will contain the reason for the failure to insert.
     * <br><br>
     * <i>This method does not have to be overridden</i>. If it is not, the values of <code>success</code>
     * and <code>failed</code> will be printed to logcat under the tag 'SRCH2' with the message
     * prefixed by the name of the index this <code>Indexable</code> represents.
     * @param success the number of successful inserts
     * @param failed the number of failed inserts
     * @param JSONResponse the raw JSON response from the SRCH2 search server
     */
    public void onInsertComplete(int success, int failed, String JSONResponse) {
        Log.d("SRCH2", "Index " + getIndexName() + " completed insert action. Successful insertions: " +
                success + ", failed insertions: " + failed);
    }

    /**
     * Callback executed upon completion of corresponding update action by SRCH2 search server. After
     * calling either {@link #update(org.json.JSONObject)} or {@link #update(org.json.JSONArray)} the
     * SRCH2 search server will attempt to update the specified records; upon completion, the JSON
     * response will be parsed and this method will be triggered. Hopefully the sum of the values of
     * the parameters <code>success</code> and <code>upserts</code> will match the total number of records
     * updated (an upsert means a record was inserted since no existing record was found to update);  if not,
     * the parameter
     * <code>JSONResponse</code> contains the raw json response as it was received from the SRCH2
     * search server which will contain the reason for the failure to update.
     * <br><br>
     * <i>This method does not have to be overridden</i>. If it is not, the values of <code>success</code>,
     * <code>upserts</code>, and <code>failed</code> will be printed to logcat under the tag 'SRCH2' with
     * the message prefixed by the name of the index this <code>Indexable</code> represents.
     * @param success the number of existing records updated successfully
     * @param upserts the number of records that were inserted since no existing record of the same primary key
     *                was found in the index to update
     * @param failed the number of failed updates
     * @param JSONResponse the raw JSON response from the SRCH2 search server
     */
    public void onUpdateComplete(int success, int upserts, int failed, String JSONResponse) {
        Log.d("SRCH2", "Index " + getIndexName() + " completed update action. Successful updates: " +
                success + ", successful upserts: " + upserts + ", failed updates: " + failed);
    }

    /**
     * Callback executed upon completion of corresponding delete action by SRCH2 search server. After
     * calling {@link #delete(String)}  the
     * SRCH2 search server will attempt to delete the records with the specified primary keys; upon completion,
     * the JSON
     * response will be parsed and this method will be triggered. Hopefully the value of the parameter
     * <code>success</code> will match the number of primary keys passed for records to be deleted; if not,
     * the parameter
     * <code>JSONResponse</code> contains the raw json response as it was received from the SRCH2
     * search server which will contain the reason for the failure to delete.
     * <br><br>
     * <i>This method does not have to be overridden</i>. If it is not, the values of <code>success</code>
     * and <code>failed</code> will be printed to logcat under the tag 'SRCH2' with the message
     * prefixed by the name of the index this <code>Indexable</code> represents.
     * @param success the number of successful inserts
     * @param failed the number of failed inserts
     * @param JSONResponse the raw JSON response from the SRCH2 search server
     */
    public void onDeleteComplete(int success, int failed, String JSONResponse) {
        Log.d("SRCH2", "Index " + getIndexName() + " completed delete action. Successful deletions: " +
                success + ", failed deletions: " + failed);
    }

    /**
     * Callback executed upon completion of corresponding get record by id action by SRCH2 search server. After
     * calling either {@link #getRecordbyID(String)}   the
     * SRCH2 search server will attempt to retrieve the record in the index with the specified primary key;
     * upon completion,
     * the JSON
     * response will be parsed and this method will be triggered. If the record was found, it will be the value of
     * the parameter of <code>record</code> and the value of <code>success</code> will be <b>true</b>; if the
     * record was not found, <code>success</code> will be <b>false</b> and the <code>record</code> <i>will not
     * be null but contain no values</i>.
     * The value of
     * <code>JSONResponse</code> contains the raw json response as it was received from the SRCH2 search
     * server.
     * <br><br>
     * <i>This method does not have to be overridden</i>. If it is not, the values of <code>success</code>
     * and <code>record</code> will be printed to logcat under the tag 'SRCH2' with the message
     * prefixed by the name of the index this <code>Indexable</code> represents.
     * @param success whether the record was retrieved
     * @param record the retrieved record, will have no values if the record was not in the index
     * @param JSONResponse the raw JSON response from the SRCH2 search server
     */
    public void onGetRecordComplete(boolean success, JSONObject record, String JSONResponse) {
        if (success) {
            Log.d("SRCH2", "Index " + getIndexName() + " completed get record action. Record string: " +
                    record.toString());
        } else {
            Log.d("SRCH2", "Index " + getIndexName() + " completed get record action but found no record.");
        }
    }

    /**
     * Callback executed very shortly after the call to
     * {@link com.srch2.android.sdk.SRCH2Engine#onStart(android.content.Context)} is made:
     * when the SRCH2 search server is initialized by the <code>SRCH2Engine</code> (by the method just
     * mentioned), it will load each index into memory; this can take anywhere from a couple of milliseconds
     * to three seconds (depending on the number of records, how much data each record contains, and the
     * processing power of the device). When the index this <code>Indexable</code> represents is finished loading,
     * this method is thus triggered. At this point all operations on this index are valid: search, insert, update, et
     * cetera.
     * <br><br>
     * By overriding this method, its implementation can be used to verify the integrity of the index such as if
     * records need to be inserted (by checking {@link #getRecordCount()} for the first time or likewise if the index
     * needs to be updated.
     * <br><br>
     * <i>This method does not have to be overridden</i> (thought it is <b>strongly encouraged</b> to do so). If it is
     * not, the number of records it contains upon being loaded will be printed to logcat
     * under the tag 'SRCH2' with the message prefixed by the name of the index this <code>Indexable</code> represents.
     */
    public void onIndexReady() {
        Log.d("SRCH2", "Index " + getIndexName() + " is ready to be accessed and contains " + getRecordCount() + " records");
    }
}
