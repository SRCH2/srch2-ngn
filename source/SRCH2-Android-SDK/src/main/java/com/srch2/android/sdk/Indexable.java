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
 * is not online such as when {@link com.srch2.android.sdk.SRCH2Engine#initialize()}
 * has been called but {@link com.srch2.android.sdk.SRCH2Engine#onStart(android.content.Context)} has not yet been
 * called).
 */
public abstract class Indexable extends IndexableCore {

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


}
