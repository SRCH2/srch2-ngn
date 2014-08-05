package com.srch2.android.http.service;

import java.util.ArrayList;
import java.util.HashMap;

import org.json.JSONObject;

public final class GetRecordResponse extends RestfulResponse {

    public final boolean isRecordRetrieved;
    public final JSONObject record;

    /**
     * Constructor of the Response
     *
     * @param theHttpResponseCode
     * @param theRestfulResponseLiteral
     * @param targetCoreName
     */
    GetRecordResponse(int theHttpResponseCode,
                      String theRestfulResponseLiteral, String targetCoreName) {
        super(theHttpResponseCode, theRestfulResponseLiteral);

        boolean isRetrieved = false;
        JSONObject tempRecord = new JSONObject();

        if (theRestfulResponseLiteral != null && !theRestfulResponseLiteral.equals(IRRECOVERABLE_NETWORK_ERROR_MESSAGE)
                && targetCoreName != null) {

            HashMap<String, ArrayList<JSONObject>> resultMap = SearchTask
                    .parseResponseForRecordResults(
                            theRestfulResponseLiteral, false,
                            targetCoreName);
            ArrayList<JSONObject> jsonArray = resultMap.get(targetCoreName);

            if (jsonArray.size() > 0) {
                isRetrieved = true;
                tempRecord = jsonArray.get(0);
            }
        }
        isRecordRetrieved = isRetrieved;
        record = tempRecord;


    }

    /**
     * It returns a concise, human readable description of InfoResponse object.
     *
     * @return a human readable description of InfoResponse object
     */
    public String toHumanReadableString() {
        if (httpResponseCode == -1) {
            return "GetRecordResponse: error! unable to connect.";
        }
        return "GetRecordResponse: isRecordRetrieved [ "
                + isRecordRetrieved + " ] record.toString [ "
                + record.toString() + " ]";
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
        return "GetRecordResponse:\nhttpResponseCode[ " + httpResponseCode
                + " ]\nrestfulResponseLiteral[ " + restfulResponseLiteral
                + " ]";
    }
}