package com.srch2.android.sdk;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Wraps the RESTful response from the SRCH2 search server upon completion of an insertion task.
 * <br><br>
 * When any of the method calls to insert a record or a set of records into an index
 * ({@link Indexable#insert(org.json.JSONArray)},
 * or {@link Indexable#insert(org.json.JSONObject)})
 * are made, the <code>SRCH2Engine</code> will forward the insert request to the SRCH2 search server;
 * after the SRCH2 search server finishes executing, it will notify the <code>SRCH2Engine</code>
 * of the status of the completed insertion task. The <code>SRCH2Engine</code> will pass this
 * information through the
 * {@link StateResponseListener#onInsertRequestComplete(String, InsertResponse)}
 * where the
 * <code>InsertResponse response</code> is this object.
 * <br><br>
 * If there were no errors in any of the <code>JSONObject</code>(s) representing record(s),
 * the return value of {@link #getSuccessCount()} will match the number of submitted
 * <code>JSONObject</code>s. Any failures to insert (due to duplicate primary keys, for
 * instance) will be reflected in the value of <code>getFailureCount().</code>
 * <br><br>
 * In the event of a failure to insert one or more records, inspecting the value of
 * {@link #getRESTfulResponseLiteral()} will contain the reason for the failure.
 */
public final class InsertResponse extends RestfulResponse {

    private final static String JSON_KEY_PRIMARY_KEY_INDICATOR_TAG = "rid";
    private final static String JSON_KEY_LOG = "log";
    private final static String JSON_KEY_INSERT_TAG = "insert";
    private final static String JSON_VALUE_SUCCESS = "success";

    /**
     * Indicates the InsertResponse was unable to be formed from the JSON response from the
     * SRCH2 server upon performing an insertion. Has the <b>constant</b> value <code>-1</code>.
     */
    public static final int INVALID_COUNT = -1;


    /**
     * Utility method for determining the number of JSONObjects, representing records to insert,
     * that were inserted successfully.
     * @return the number of successful insertions
     */
    public int getSuccessCount() { return recordInsertSuccessCount; }
    private final int recordInsertSuccessCount;

    /**
     * Utility method for determining the number of JSONObjects, representing records to insert,
     * that failed to be inserted.
     * @return the number of failed insertions
     */
    public int getFailureCount() { return recordInsertFailureCount; }
    private final int recordInsertFailureCount;

    private final boolean parseSuccess;

    InsertResponse(int theHttpResponseCode, String theRestfulResponseLiteral) {
        super(theHttpResponseCode, theRestfulResponseLiteral);
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
                    boolean insertSuccess = recordStamp.getString(JSON_KEY_INSERT_TAG).equals(JSON_VALUE_SUCCESS);
                    if (insertSuccess) {
                        ++successCount;
                    } else {
                        ++failureCount;
                    }
                }
            }
        } catch (JSONException oops) {
            oops.printStackTrace();
            successCount = INVALID_COUNT;
            failureCount = INVALID_COUNT;
            success = false;
        }

        recordInsertSuccessCount = successCount;
        recordInsertFailureCount = failureCount;
        parseSuccess = success;
    }

    @Override
    public String toString() {
        if (parseSuccess) {
            return "Insert task completed with " + getSuccessCount() + " successful insertions " +
                    "and " + getFailureCount() + " failed insertions";
        } else {
            return "Insert task failed to execute. Printing info:\n" +
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
            return "Insert task completed with:\n" +
                    getSuccessCount() + " successful insertions\n" +
                    getFailureCount() + " failed insertions";
        } else {
            return "Insert task failed to execute. Printing info:\n" +
                    "RESTful HTTP status code: " + getRESTfulHTTPStatusCode() + "\n" +
                    "RESTful response: " + getRESTfulResponseLiteral();
        }
    }
}
