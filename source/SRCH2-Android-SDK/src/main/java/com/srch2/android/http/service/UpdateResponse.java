package com.srch2.android.http.service;

import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public final class UpdateResponse extends RestfulResponse {

    private final static String JSON_KEY_MESSAGE = "message";
    private final static String JSON_KEY_PRIMARY_KEY_INDICATOR_TAG = "rid";
    private final static String JSON_KEY_LOG = "log";
    private final static String JSON_KEY_INSERT_TAG = "update";
    private final static String JSON_VALUE_FAIL = "failed";
    private final static String JSON_KEY_FAILURE_REASON = "reason";


    public int getSuccessCount() { return recordUpdateSuccessCount; }
    private final int recordUpdateSuccessCount;
    public int getFailureCount() { return recordUpdateFailureCount; }
    private final int recordUpdateFailureCount;

    UpdateResponse(int theHttpResponseCode, String theRestfulResponseLiteral) {
        super(theHttpResponseCode, theRestfulResponseLiteral);

        Log.d("srch2:: UpdateResponse", "response\n" + theRestfulResponseLiteral + "\nend of response");

        int successCount = 0;
        int failureCount = 0;

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
            successCount = -1;
            failureCount = -1;
        }

        recordUpdateSuccessCount = successCount;
        recordUpdateFailureCount = failureCount;
    }

    @Override
    public String toString() {
        return "UpdateResponse: httpResponseCode[ " + httpResponseCode
                + " ] restfulResponseLiteral[ " + restfulResponseLiteral
                + " ]";
    }

    /**
     * Users of <code>UpdateResponse</code> may call this to get a concise,
     * human readable description of this object that will fit in a toast
     * such that each field member is on its own line.
     *
     * @return a human readable description of this object that nicely fits
     * into a toast
     */
    public String toToastString() {
        return "UpdateResponse:\nhttpResponseCode[ " + httpResponseCode
                + " ]\nrestfulResponseLiteral[ " + restfulResponseLiteral
                + " ]";
    }
}