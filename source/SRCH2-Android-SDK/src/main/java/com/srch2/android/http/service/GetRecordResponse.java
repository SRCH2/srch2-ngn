package com.srch2.android.http.service;

import java.util.ArrayList;
import java.util.HashMap;
import org.json.JSONObject;

/**
 * Wraps the RESTful response from the SRCH2 search server upon completion of a record
 * retrieval task.
 * <br><br>
 * When the method call to retrieve a record from an index
 * {@link com.srch2.android.http.service.Indexable#getRecordbyID(String)}
 * is made, the {@link com.srch2.android.http.service.SRCH2Engine} will forward the record retrieval
 * request to the SRCH2 search server; after the SRCH2 search server finishes retrieving
 * the record, it will notify the <code>SRCH2Engine</code> of the status of the completed
 * record retrieval task and pass the retrieved record if found. The <code>SRCH2Engine</code>
 * will pass this information through the callback method
 * {@link com.srch2.android.http.service.StateResponseListener#onGetRecordByIDComplete(String, GetRecordResponse)}
 * where the
 * <code>GetRecordResponse response</code> is this object.
 * <br><br>
 * If the primary key submitted matches an existing record, {@link #wasRecordRetrieved()}
 * will return <b>true</b> and {@link #getRetrievedRecord()} will return the
 * <code>JSONObject</code> representing the record as it exists in the index; if the
 * primary key did not match any existing records, {@link #wasRecordRetrieved()}
 * will return <b>false</b> and {@link #getRetrievedRecord()} will return
 * a <i>non-null</i> <code>JSONObject</code> containing <b>no</b> key-pair values.
 */
public final class GetRecordResponse extends RestfulResponse {

    /**
     * Indicates whether the record was able to be retrieved: that is, whether the submitted
     * primary key matched an existing record.
     * @return <b>true</b> if the record was retrieved; <b>false</b> otherwise
     */
    public boolean wasRecordRetrieved() { return isRecordRetrieved; }
    final boolean isRecordRetrieved;

    /**
     * The record retrieved. If no record was able to be retrieved, this <code>JSONObject</code>
     * will not be null but contain no key-pair values.
     * @return the record retrieved if found
     */
    public JSONObject getRetrievedRecord() { return record == null ? new JSONObject() : record; }
    final JSONObject record;
    final String sourceIndexName;

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

        if (!isRetrieved) {
            sourceIndexName = "no index contained requested record";
        } else {
            sourceIndexName = targetCoreName;
        }
    }

    @Override
    public String toString() {
        if (getRESTfulHTTPStatusCode() == -1) {
            return "GetRecordResponse: error! unable to connect. Printing info...\n" +
                    "RESTful HTTP status code: " + getRESTfulHTTPStatusCode() + "\n" +
                    "RESTful response: " + getRESTfulResponseLiteral();
        } else {
            if (isRecordRetrieved) {
                return "Record retrieved from index with name " + sourceIndexName + "\n" +
                        "Printing record: " + getRetrievedRecord().toString();
            } else {
                return "No record was found for given primary key";
            }
        }
    }

    /**
     * Convenience method for outputting to toasts.
     * @return a human readable description of this object that nicely fits
     * into a toast
     */
    public String toToastString() {
        if (getRESTfulHTTPStatusCode() == -1) {
            return "GetRecordResponse: error!\nunable to connect. Printing info...\n" +
                    "RESTful HTTP status code: " + getRESTfulHTTPStatusCode() + "\n" +
                    "RESTful response:\n " + getRESTfulResponseLiteral();
        } else {
            if (isRecordRetrieved) {
                return "Record retrieved!\n" +
                        "Source index: " + sourceIndexName + "\n" +
                        "Printing record:\n"
                        + getRetrievedRecord().toString();
            } else {
                return "No record was found\nfor given primary key";
            }
        }
    }
}
