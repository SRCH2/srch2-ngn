package com.srch2.android.http.service;

import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
/**
 * Wraps the RESTful response from the SRCH2 search server upon completion of an update task.
 * <br><br>
 * When any of the method calls to update a record or a set of records into an index (
 * <code>mIndexable.update(JSONObject recordToInsert)</code>,
 * <code>Indexable.update(JSONArray recordsToInsert)</code> or
 * the corresponding static calls in the <code>SRCH2Engine</code>)
 * are made, the <code>SRCH2Engine</code> will forward the update request to the SRCH2 search server;
 * after the SRCH2 search server finishes executing, it will notify the <code>SRCH2Engine</code>
 * of the status of the completed update task. The <code>SRCH2Engine</code> will pass this
 * information through the <code>StateResponseListener</code> callback method
 * <code>onUpdateRequestComplete(String indexName, UpdateResponse response)</code> where the
 * <code>UpdateResponse response</code> is this object.
 * <br><br>
 * If there were no errors in any of the <code>JSONObject</code>(s) representing record(s),
 * the return value of <code>getSuccessCount()</code> will match the number of submitted
 * <code>JSONObject</code>s. Any failures to update (missing or incorrect type for
 * one of the defined schema fields, for instance) will be reflected in the value of
 * <code>getFailureCount().</code>
 * <br><br>
 * In the event of a failure to update one or more records, inspecting the value of
 * <code>getRESTfulResponseLiteral()</code> will contain the reason for the failure.
 */
public final class UpdateResponse extends RestfulResponse {

    private final static String JSON_KEY_PRIMARY_KEY_INDICATOR_TAG = "rid";
    private final static String JSON_KEY_LOG = "log";
    private final static String JSON_KEY_INSERT_TAG = "update";
    private final static String JSON_VALUE_FAIL = "failed";


    /**
     * Indicates the InfoResponse was unable to be formed from the JSON response from the
     * SRCH2 server upon performing an info command. Has the <b>constant</b> value <code>-1</code>.
     */
    public static final int INVALID_COUNT = -1;



    /**
     * Utility method for determining the number of JSONObjects, representing records to update,
     * that were updated successfully.
     * @return the number of successful updates
     */
    public int getSuccessCount() { return recordUpdateSuccessCount; }
    private final int recordUpdateSuccessCount;
    /**
     * Utility method for determining the number of JSONObjects, representing records to update,
     * that failed to be updated.
     * @return the number of failed updates
     */
    public int getFailureCount() { return recordUpdateFailureCount; }
    private final int recordUpdateFailureCount;

    private final boolean parseSuccess;

    UpdateResponse(int theHttpResponseCode, String theRestfulResponseLiteral) {
        super(theHttpResponseCode, theRestfulResponseLiteral);

        Log.d("srch2:: UpdateResponse", "response\n" + theRestfulResponseLiteral + "\nend of response");

        int successCount = 0;
        int failureCount = 0;

        boolean success = true;
        try {
            JSONObject rootNode = new JSONObject(theRestfulResponseLiteral);
            JSONArray logNode = rootNode.getJSONArray(JSON_KEY_LOG);

            int length = logNode.length();
            for (int i = 0; i < length; ++i) {
                JSONObject recordStamp = (JSONObject) logNode.get(i);
                if (recordStamp.has(JSON_KEY_PRIMARY_KEY_INDICATOR_TAG)) {
                    boolean updateFail = recordStamp.getString(JSON_KEY_INSERT_TAG).equals(JSON_VALUE_FAIL);
                    if (updateFail) {
                        ++failureCount;
                    } else {
                        ++successCount;
                    }
                }
            }
        } catch (JSONException oops) {
            successCount = INVALID_COUNT;
            failureCount = INVALID_COUNT;
            success = false;
        }
        parseSuccess = success;
        recordUpdateSuccessCount = successCount;
        recordUpdateFailureCount = failureCount;
    }

    @Override
    public String toString() {
        if (parseSuccess) {
            return "Update task completed with " + getSuccessCount() + " successful updates " +
                    "and " + getFailureCount() + " failed updates";
        } else {
            return "Update task failed to execute. Printing info:\n" +
                    "RESTful HTTP status code: " + getRESTfulHTTPStatusCode() + "\n" +
                    "RESTful response: " + getRESTfulResponseLiteral();
        }
    }

    /**
     * Convenience method for outputting to toasts.
     * @return a human readable description of this object that nicely fits
     * into a toast
     */
    public String toToastString() {
        if (parseSuccess) {
            return "Update task completed with:\n" +
                    getSuccessCount() + " successful updates\n" +
                    getFailureCount() + " failed updates";
        } else {
            return "Update task failed to execute. Printing info:\n" +
                    "RESTful HTTP status code: " + getRESTfulHTTPStatusCode() + "\n" +
                    "RESTful response: " + getRESTfulResponseLiteral();
        }
    }
}