package com.srch2.android.sdk;

import android.util.Log;
import org.json.JSONArray;
import org.json.JSONObject;

/**
 * Represents an index in the SRCH2 search server. For every index to be searched on, users of the
 * SRCH2 Android SDK should implement a separate subclass instance of this class. This class contains
 * methods for performing CRUD actions on the index such as insertion and searching. Note that it is
 * always possible to retrieve a reference to a specific {@code Indexable} from the static method
 * {@link com.srch2.android.sdk.SRCH2Engine#getIndex(String)}  where {@code 
 * indexName} matches the return value of {@link #getIndexName()}.
 * <br><br>
 * Each {@code Indexable } instance should be registered prior to calling
 * {@link com.srch2.android.sdk.SRCH2Engine#onResume(android.content.Context)} by calling
 * {@link com.srch2.android.sdk.SRCH2Engine#setIndexables(Indexable, Indexable...)} and passing in all
 * {@code Indexable } instances.
 * <br><br>
 * For each implementation of this class, it is necessary to override the two methods:<br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getIndexName()} (index's handle) <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getSchema()} (configuration of the index)
 * <br><br>
 * In addition, each implementation can customize search results from the index this {@code Indexable} represents by
 * optionally overriding the methods:
 * <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getTopK()} (number of search results) <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getFuzzinessSimilarityThreshold()} (number of typos per search input) <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getHighlighter()} (formatting of search results)
 * <br><br>
 * If the following callback methods are overridden, they will be executed after their corresponding CRUD action:
 * <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #onInsertComplete(int, int, String)}<br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #onUpdateComplete(int, int, int, String)}<br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #onDeleteComplete(int, int, String)}<br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #onGetRecordComplete(boolean, org.json.JSONObject, String)}<br><br>
 * For example, after calling {@link #insert(org.json.JSONArray)}, once the SRCH2 search server is finished processing
 * the insert, the callback method {@link #onInsertComplete(int, int, String)} will be executed passing the number
 * of successful and failed inserts. Each of these methods includes as its last parameter the exact JSON response
 * as it was returned by the SRCH2 search server detailing the completion status of the operation.
 * <br><br>
 * When the SRCH2 search server comes online, after {@link SRCH2Engine#onResume(android.content.Context)} is called,
 * it will load the indexes into memory. When the particular index the {@code Indexable } represents is initialized
 * and ready for access, the following callback method will be executed:<br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #onIndexReady()}<br><br>
 * There is also one method to return the number of records in the index:<br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getRecordCount()}.<br>
 * <br>
 * Note that if the index is to be based on an SQLite database, the {@link com.srch2.android.sdk.SQLiteIndexable} class
 * should be used instead.
 */
public abstract class Indexable extends IndexableCore {
    /**
     * If returned from {@link #getRecordCount()} indicates this value has not yet been set.
     * <br><br>
     * Constant Value: {@value}
     */
    public static final int INDEX_RECORD_COUNT_NOT_SET = -1;

    /**
     * The JSON key to use to retrieve each original record from each {@code JSONObject } in
     * the {@code ArrayList<JSONObject>} of the callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}.
     * <br><br>
     * Constant Value: "{@value}"
     */
    public static final String SEARCH_RESULT_JSON_KEY_RECORD = "record";

    /**
     * The JSON key to use to retrieve each set of highlighted fields from each record from each {@code JSONObject} in
     * the {@code ArrayList<JSONObject>} of the callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}.
     * <br><br>
     * Constant Value: "{@value}"
     */
    public static final String SEARCH_RESULT_JSON_KEY_HIGHLIGHTED = "highlighted";

    /**
     * The default value for the numbers of search results to return per search request.
     * <br><br>
     * Constant Value: {@value}
     */
    public static final int DEFAULT_NUMBER_OF_SEARCH_RESULTS_TO_RETURN_AKA_TOPK = 10;

    /**
     * The default value for the fuzziness similarity threshold. Approximately one character
     * per every three characters will be allowed to act as a wildcard during a search.
     * <br><br>
     * Constant Value: {@value}
     */
    public static final float DEFAULT_FUZZINESS_SIMILARITY_THRESHOLD = 0.65f;


    /**
     * Sets the schema of the index this {@code Indexable}
     * represents. The schema
     * defines the data fields of the index, much like the table structure of an SQLite database table. See
     * {@link Schema} for more information.
     * @return the schema to define the index structure this {@code Indexable} represents
     */
    abstract public Schema getSchema();



    /**
     * Returns the number of records that are currently in the index that this
     * {@code Indexable} represents.
     * @return the number of records in the index
     */
    public final int getRecordCount() {
        return numberOfDocumentsInTheIndex;
    }

    /**
     * Inserts the specified {@code JSONObject record} into the index that this
     * {@code Indexable} represents. This {@code JSONObject} should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema. The values of these keys should match the type defined in the index's
     * schema; numeric types will be converted to text if the field type is
     * {@link com.srch2.android.sdk.Field.Type#TEXT}.
     * <br><br>
     * After the SRCH2 search server completes the insertion task, the callback method of the
     * {@link #onInsertComplete(int, int, String)}
     * will be executed containing the status of the
     * completed insertion task.
     * @param record the record to insert
     */
    public final void insert(JSONObject record) {
        if (indexInternal != null) {
            indexInternal.insert(record);
        }
    }

    /**
     * Inserts the set of records represented as {@code JSONObject}s contained in the
     * specified {@code JSONArray records}. This {@code JSONArray} should be properly formed
     * and contain only the set of records, each represented as a {@code JSONObject},
     * to be inserted.
     * <br><br>
     * Each {@code JSONObject} in the set should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema. The values of these keys should match the type defined in the index's
     * schema; numeric types will be converted to text if the field type is
     * {@link com.srch2.android.sdk.Field.Type#TEXT}.
     * <br><br>
     * After the SRCH2 search server completes the insertion task, the callback method of the
     * {@link #onInsertComplete(int, int, String)}
     * will be executed containing the status of the
     * completed insertion task.
     * @param records the set of records to insert
     */
    public final void insert(JSONArray records) {
        if (indexInternal != null) {
            indexInternal.insert(records);
        }
    }

    /**
     * Updates the specified {@code JSONObject record} into the index that this
     * {@code Indexable} represents. This {@code JSONObject} should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema. The values of these keys should match the type defined in the index's
     * schema; numeric types will be converted to text if the field type is
     * {@link com.srch2.android.sdk.Field.Type#TEXT}.
     * <br><br>
     * If the primary key supplied in the {@code record} does not
     * match the primary key of any existing record, the {@code record} will be
     * inserted (an 'upsert').
     * <br><br>
     * Records to be updated must contain the complete set of fields: if any field data
     * is missing, it will not retrieved from the existing record.
     * <br><br>
     * After the SRCH2 search server completes the update task, the callback method of the
     * {@link #onUpdateComplete(int, int, int, String)}
     * will be executed containing the status of the
     * completed update task.
     * @param record the record to upsert
     */
    public final void update(JSONObject record) {
        if (indexInternal != null) {
            indexInternal.update(record);
        }
    }

    /**
     * Updates the set of records, each represented as a {@code JSONObject}, contained in the
     * specified {@code JSONArray records}. This {@code JSONArray} should be properly formed
     * and contain only the set of records, each represented as a {@code JSONObject},
     * to be updated.
     * <br><br>
     * Each {@code JSONObject} in the set should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema. The values of these keys should match the type defined in the index's
     * schema; numeric types will be converted to text if the field type is
     * {@link com.srch2.android.sdk.Field.Type#TEXT}.
     * If the primary key of any of the supplied records in the {@code JSONArray}
     * does not match the primary key of any existing record, that record will be
     * inserted (an 'upsert').
     * <br><br>
     * Records to be updated must contain the complete set of fields: if any field data
     * is missing, it will not retrieved from the existing record.
     * <br><br>
     * After the SRCH2 search server completes the update task, the callback method of the
     * {@link #onUpdateComplete(int, int, int, String)}
     *  will be executed containing the status of the
     * completed update task.
     * @param records the set of records to upsert
     */
    public final void update(JSONArray records) {
        if (indexInternal != null) {
            indexInternal.update(records);
        }
    }

    /**
     * Deletes from the index the record with a primary
     * key matching the value of the {@code primaryKeyOfRecordToDelete}. If no record with
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
     * Retrieves a record from the index this {@code Indexable} represents if the primary
     * key of any record in the index matches the value of {@code primaryKeyOfRecordToRetrieve}.
     * When the SRCH2 search server completes the retrieval task, the method
     * {@link #onGetRecordComplete(boolean, org.json.JSONObject, String)}
     * will be executed containing the record represented as a {@code JSONObject} if found.
     * <br><br>
     * This method will than an {@code IllegalArgumentException}if the value of
     * {@code primaryKeyOfRecordToRetrieve} is null or has a length less than one.
     * @param primaryKeyOfRecordToRetrieve the primary key
     */
    public final void getRecordbyID(String primaryKeyOfRecordToRetrieve) {
        if (indexInternal != null) {
            indexInternal.getRecordbyID(primaryKeyOfRecordToRetrieve);
        }
    }

    /**
     * Callback executed upon completion of corresponding insert action by the SRCH2 search server. After
     * calling either {@link #insert(org.json.JSONObject)} or {@link #insert(org.json.JSONArray)} the
     * SRCH2 search server will attempt to insert the specified records; upon completion, the JSON
     * response will be parsed and this method will be triggered. If the value of the parameter
     * {@code success} matches the number of records originally inserted, the insertion task was a complete
     * success; if not, the parameter
     * {@code JSONResponse} contains the raw JSON response as it was received from the SRCH2
     * search server which will contain the reason for the failure to insert.
     * <br><br>
     * <i>This method does not have to be overridden</i>. If it is not, the values of {@code success}
     * and {@code failed} will be printed to logcat under the tag 'SRCH2' with the message
     * prefixed by the name of the index this {@code Indexable} represents.
     * @param success the number of successful inserts
     * @param failed the number of failed inserts
     * @param JSONResponse the raw JSON response from the SRCH2 search server
     */
    public void onInsertComplete(int success, int failed, String JSONResponse) {
        Log.d("SRCH2", "Index " + getIndexName() + " completed insert action. Successful insertions: " +
                success + ", failed insertions: " + failed);
    }

    /**
     * Callback executed upon completion of corresponding update action by the SRCH2 search server. After
     * calling either {@link #update(org.json.JSONObject)} or {@link #update(org.json.JSONArray)} the
     * SRCH2 search server will attempt to update the specified records; upon completion, the JSON
     * response will be parsed and this method will be triggered. If the sum of the values of
     * {@code success} and {@code upserts} matches the total number of records originally
     * updated, the update task was a complete success;  if not,
     * the parameter
     * {@code JSONResponse} contains the raw JSON response as it was received from the SRCH2
     * search server which will contain the reason for the failure to update.
     * <br><br>
     * An upsert occurs when a record to be updated did not exist in the index and was consequently
     * inserted.
     * <br><br>
     * <i>This method does not have to be overridden</i>. If it is not, the values of {@code success},
     * {@code upserts}, and {@code failed} will be printed to logcat under the tag 'SRCH2' with
     * the message prefixed by the name of the index this {@code Indexable} represents.
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
     * Callback executed upon completion of corresponding delete action by the SRCH2 search server. After
     * calling {@link #delete(String)}  the
     * SRCH2 search server will attempt to delete the records with the specified primary keys; upon completion,
     * the JSON
     * response will be parsed and this method will be triggered. If the value of the parameter
     * {@code success} matches the original number of primary keys passed to {@link #delete(String)}, the
     * delete task was a complete success; if not,
     * the parameter
     * {@code JSONResponse} contains the raw JSON response as it was received from the SRCH2
     * search server which will contain the reason for the failure to delete.
     * <br><br>
     * <i>This method does not have to be overridden</i>. If it is not, the values of {@code success}
     * and {@code failed} will be printed to logcat under the tag 'SRCH2' with the message
     * prefixed by the name of the index this {@code Indexable} represents.
     * @param success the number of successful inserts
     * @param failed the number of failed inserts
     * @param JSONResponse the raw JSON response from the SRCH2 search server
     */
    public void onDeleteComplete(int success, int failed, String JSONResponse) {
        Log.d("SRCH2", "Index " + getIndexName() + " completed delete action. Successful deletions: " +
                success + ", failed deletions: " + failed);
    }

    /**
     * Callback executed upon completion of corresponding get record by id action by the SRCH2 search server. After
     * calling either {@link #getRecordbyID(String)}   the
     * SRCH2 search server will attempt to retrieve the record in the index with the specified primary key;
     * upon completion,
     * the JSON
     * response will be parsed and this method will be executed.
     * <br><br>If the record was found, it will be the value of
     * the parameter of {@code record} and the value of {@code success} will be <b>true</b>; if the
     * record was not found, {@code success} will be <b>false</b> and the {@code record} will not
     * be null but contain no values.
     * The value of
     * {@code JSONResponse} contains the raw json response as it was received from the SRCH2 search
     * server.
     * <br><br>
     * <i>This method does not have to be overridden</i>. If it is not, the values of {@code success}
     * and {@code record} will be printed to logcat under the tag 'SRCH2' with the message
     * prefixed by the name of the index this {@code Indexable} represents.
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


}
