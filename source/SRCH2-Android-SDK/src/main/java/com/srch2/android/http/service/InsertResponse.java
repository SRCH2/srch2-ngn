package com.srch2.android.http.service;

import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public final class InsertResponse extends RestfulResponse {

    private final static String JSON_KEY_MESSAGE = "message";
    private final static String JSON_KEY_PRIMARY_KEY_INDICATOR_TAG = "rid";
    private final static String JSON_KEY_LOG = "log";
    private final static String JSON_KEY_INSERT_TAG = "insert";
    private final static String JSON_VALUE_SUCCESS = "success";
    private final static String JSON_KEY_FAILURE_REASON = "reason";


    public int getSuccessCount() { return recordInsertSuccessCount; }
    private final int recordInsertSuccessCount;
    public int getFailureCount() { return recordInsertFailureCount; }
    private final int recordInsertFailureCount;

    InsertResponse(int theHttpResponseCode, String theRestfulResponseLiteral) {
        super(theHttpResponseCode, theRestfulResponseLiteral);

        Log.d("srch2:: InsertResponse", "response\n" + theRestfulResponseLiteral + "\nend of response");

        int successCount = 0;
        int failureCount = 0;

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

            Log.d("srch2:: InsertResponse", "insertSuccess " + successCount + " insertFailure " + failureCount);

        } catch (JSONException oops) {
            Log.d("srch2:: InsertResponse", "JSON EXCEPTION");
            oops.printStackTrace();
            successCount = INVALID_COUNT;
            failureCount = INVALID_COUNT;
        }

        recordInsertSuccessCount = successCount;
        recordInsertFailureCount = failureCount;
    }

    @Override
    public String toString() {
        return "InsertResponse: httpResponseCode[ " + httpResponseCode
                + " ] restfulResponseLiteral[ " + restfulResponseLiteral
                + " ]";
    }

    /**
     * It returns a concise, human readable description of InsertResponse object that
     * will fit in a toast such that each field member is on its own line.
     *
     * @return a human readable description of InsertResponse object that nicely fits
     * into a toast
     */
    public String toToastString() {
        return "InsertResponse:\nhttpResponseCode[ " + httpResponseCode
                + " ]\nrestfulResponseLiteral[ " + restfulResponseLiteral
                + " ]";
    }
}