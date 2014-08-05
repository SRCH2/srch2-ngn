package com.srch2.android.http.service;

import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public final class DeleteResponse extends RestfulResponse {

    private final static String JSON_KEY_MESSAGE = "message";
    private final static String JSON_KEY_PRIMARY_KEY_INDICATOR_TAG = "rid";
    private final static String JSON_KEY_LOG = "log";
    private final static String JSON_KEY_INSERT_TAG = "delete";
    private final static String JSON_VALUE_SUCCESS = "success";
    private final static String JSON_KEY_FAILURE_REASON = "reason";

    public int getSuccessCount() { return success ? 1 : 0; }
    public int getFailureCount() { return success ? 0 : 1;  }
    private final boolean success;

    DeleteResponse(int theHttpResponseCode, String theRestfulResponseLiteral) {
        super(theHttpResponseCode, theRestfulResponseLiteral);

        Log.d("srch2:: DeleteResponse", "response\n" + theRestfulResponseLiteral + "\nend of response");
        boolean deleteSuccess = false;
        try {
            JSONObject rootNode = new JSONObject(theRestfulResponseLiteral);
            JSONArray logNode = rootNode.getJSONArray(JSON_KEY_LOG);

            int length = logNode.length();
            if (length == 1) {

                JSONObject recordStamp = (JSONObject) logNode.get(0);
                if (recordStamp.has(JSON_KEY_PRIMARY_KEY_INDICATOR_TAG)) {
                    deleteSuccess = recordStamp.getString(JSON_KEY_INSERT_TAG).equals(JSON_VALUE_SUCCESS);
                }
            }
        } catch (JSONException oops) {
            deleteSuccess = false;
        }
        success = deleteSuccess;
    }

    @Override
    public String toString() {
        return "DeleteResponse: httpResponseCode[ " + httpResponseCode
                + " ] restfulResponseLiteral[ " + restfulResponseLiteral
                + " ]";
    }

    /**
     * Users of <code>DeleteResponse</code> may call this to get a concise,
     * human readable description of this object that will fit in a toast
     * such that each field member is on its own line.
     *
     * @return a human readable description of this object that nicely fits
     * into a toast
     */
    public String toToastString() {
        return "DeleteResponse:\nhttpResponseCode[ " + httpResponseCode
                + " ]\nrestfulResponseLiteral[ " + restfulResponseLiteral
                + " ]";
    }
}