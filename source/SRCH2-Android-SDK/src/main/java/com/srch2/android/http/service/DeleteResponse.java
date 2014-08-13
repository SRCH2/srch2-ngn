package com.srch2.android.http.service;

import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Wraps the RESTful response from the SRCH2 search server upon completion of a deletion task.
 * <br><br>
 * When the method call {@link com.srch2.android.http.service.Indexable#delete(String)}
 * is made, the {@link com.srch2.android.http.service.SRCH2Engine} will forward the delete request to the SRCH2 search server;
 * after the SRCH2 search server finishes executing, it will notify the <code>SRCH2Engine</code>
 * of the status of the completed deletion task. The <code>SRCH2Engine</code> will pass this
 * information through the {@link com.srch2.android.http.service.StateResponseListener} callback method
 * {@link com.srch2.android.http.service.StateResponseListener#onDeleteRequestComplete(String, DeleteResponse)} where the
 * <code>DeleteResponse response</code> is this object.
 * <br><br>
 * If the primary key submitted matches that of an existing record,
 * the deletion task will be successful and {@link #getSuccessCount()} will return one. If no
 * record is found for the submitted primary key, {@link #getFailureCount()} will return zero and
 * <code>getFailureCount()</code> will return one.
 * <br><br>
 * In the event of a failure to delete a record, inspecting the value of {@link #getRESTfulResponseLiteral()}
 * will contain the reason for the failure.
 */
public final class DeleteResponse extends RestfulResponse {

    private static final String TAG = "DeleteResponse";

    private final static String JSON_KEY_PRIMARY_KEY_INDICATOR_TAG = "rid";
    private final static String JSON_KEY_LOG = "log";
    private final static String JSON_KEY_INSERT_TAG = "delete";
    private final static String JSON_VALUE_SUCCESS = "success";

    /**
     * Indicates the InfoResponse was unable to be formed from the JSON response from the
     * SRCH2 server upon performing an info command. Has the <b>constant</b> value <code>-1</code>.
     */
    public static final int INVALID_COUNT = -1;


    /**
     * Utility method for determining if the primary key submitted in the deletion task matched an
     * existing record that was subsequently and successfully deleted.
     * @return the number of successful deletions
     */
    public int getSuccessCount() { return parseSuccess ? 1 : 0; }

    /**
     * Utility method for determining if the deletion task failed such as no record was found with the
     * submitted primary key.
     * @return the number of failed deletions
     */
    public int getFailureCount() { return parseSuccess ? 0 : 1;  }

    private final boolean parseSuccess;

    DeleteResponse(int theHttpResponseCode, String theRestfulResponseLiteral) {
        super(theHttpResponseCode, theRestfulResponseLiteral);

        Cat.d(TAG, "response\n" + theRestfulResponseLiteral + "\nend of response");
        boolean success = false;
        try {
            JSONObject rootNode = new JSONObject(theRestfulResponseLiteral);
            JSONArray logNode = rootNode.getJSONArray(JSON_KEY_LOG);

            int length = logNode.length();
            if (length == 1) {

                JSONObject recordStamp = (JSONObject) logNode.get(0);
                if (recordStamp.has(JSON_KEY_PRIMARY_KEY_INDICATOR_TAG)) {
                    success = recordStamp.getString(JSON_KEY_INSERT_TAG).equals(JSON_VALUE_SUCCESS);
                }
            }
        } catch (JSONException oops) {
            success = false;
        }
        parseSuccess = success;
    }

    @Override
    public String toString() {
        if (parseSuccess) {
            return "Delete task completed with " + getSuccessCount() + " successful deletions " +
                    "and " + getFailureCount() + " failed deletions";
        } else {
            return "Delete task failed to execute. Printing info:\n" +
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
            return "Delete task completed with:\n" +
                    getSuccessCount() + " successful deletions\n" +
                    getFailureCount() + " failed deletions";
        } else {
            return "Delete task failed to execute. Printing info:\n" +
                    "RESTful HTTP status code: " + getRESTfulHTTPStatusCode() + "\n" +
                    "RESTful response: " + getRESTfulResponseLiteral();
        }
    }
}
