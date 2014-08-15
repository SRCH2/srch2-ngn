package com.srch2.android.http.service;

import org.json.JSONObject;

/**
 * Wraps the RESTful response from the SRCH2 search server upon completion of an information task.
 * <br><br>
 * When either of the method calls to retrieve information on an {@link Indexable#info()} ,
 * the <code>SRCH2Engine</code> will forward the information request to the SRCH2 search server;
 * after the SRCH2 search server finishes executing, it will notify the <code>SRCH2Engine</code>
 * of the status of the completed information task and pass the returned index information.
 * The <code>SRCH2Engine</code> will pass this information through the callback method
 * {@link com.srch2.android.http.service.StateResponseListener#onInfoRequestComplete(String, InfoResponse)} where the
 * <code>InfoResponse response</code> is this object.
 * <br><br>
 * When this class is constructed, the RESTful response from the SRCH2 search server is parsed and made
 * available through the several methods of this class. For example, the number of records in the index
 * can be obtained by calling {@link #getNumberOfDocumentsInTheIndex()}.
 * <br><br>
 * Users can verify that the SRCH2 server was able to retrieve information on the specified index by checking
 * the value returned from {@link #isValidInfoResponse}; if there was some error, such as the SRCH2
 * search server was unavailable, this method will return <b>false</b>.
 */
public final class InfoResponse extends RestfulResponse {

    /**
     * Indicates the InfoResponse was unable to be formed from the JSON response from the SRCH2 server upon performing an info command.
     * Has the <bold>constant</bold> value <code>"null"</code>.
     */
    public static final String INVALID_LAST_MERGE_TIME = "null";


    /**
     * Indicates the InfoResponse was unable to be formed from the JSON response from the SRCH2 server upon performing an info command.
     * Has the <bold>constant</bold> value <code>-1</code>.
     */
    public static final int INVALID_COUNT = -1;



    /**
     * Returns the number of search requests that have been made on the specified index.
     * @return the number of search requests
     */
    public int getNumberOfSearchRequests() { return numberOfSearchRequests; }
    private final int numberOfSearchRequests;

    /**
     * Returns the number of write requests (insert, update, and delete) that have been made on the specified index.
     * @return the number of write requests
     */
    public int getNumberOfWriteRequests() { return numberOfWriteRequests; }
    private final int numberOfWriteRequests;

    /**
     * Returns the number of records that are currently in the index.
     * @return the number of records
     */
    public int getNumberOfDocumentsInTheIndex() { return numberOfDocumentsInTheIndex; }
    private final int numberOfDocumentsInTheIndex;

    /**
     * Returns the last time the specified index was merged--that is, the last time
     * the specified index was updated to reflect any pending write
     * requests prior to this time.
     * <br><br>
     * <b>Note</b>: The value of <code>lastMergeTime</code> may be set to "null"
     * if the response from the SRCH2 search server was unable to be parsed correctly.
     * @return a time-stamp of the last time the index was merged
     */
    public String getLastMergeTime() { return lastMergeTime == null ? INVALID_LAST_MERGE_TIME : lastMergeTime; }
    private final String lastMergeTime;

    /**
     * Returns whether this <code>InfoResponse</code> was parsed correctly. If <b>true</b>, the specified index
     * is up, running and searchable; if <b>false</b>, this indicates the index is unavailable and
     * <code>getSearchRequestCount()</code>, <code>getWriteRequestCount()</code>, and
     * <code>getNumberOfDocumentsInTheIndex()</code> will return <code>-1</code> or <code>INVALID_COUNT</code>
     * and <code>getLastMergeTime()</code> will return the literal value "null" or <code>INVALID_LAST_MERGE_TIME</code>.
     * @return whether this <code>InfoResponse</code> was formed properly from the JSON response
     *          indicating whether the index is available
     */
    public boolean isValidInfoResponse() { return isValidInfoResponse; }
    private final boolean isValidInfoResponse;

    InfoResponse(int theHttpResponseCode, String theRestfulResponseLiteral) {
        super(theHttpResponseCode, theRestfulResponseLiteral);
        int tempNumberOfSearchRequests = INVALID_COUNT;
        int tempNumberOfWriteRequests = INVALID_COUNT;
        int tempNumberOfDocumentsInTheIndex = INVALID_COUNT;
        String tempLastMergeTime = INVALID_LAST_MERGE_TIME;
        boolean success = false;
        try {
            if (theHttpResponseCode / 100 == 2) {
                JSONObject rootNode;
                rootNode = new JSONObject(theRestfulResponseLiteral);
                JSONObject engineStatus = rootNode
                        .getJSONObject("engine_status");

                tempNumberOfDocumentsInTheIndex = engineStatus
                        .getInt("docs_in_index");
                tempNumberOfSearchRequests = engineStatus
                        .getInt("search_requests");
                tempNumberOfWriteRequests = engineStatus
                        .getInt("write_requests");

                tempLastMergeTime = engineStatus.getString("last_merge");
                success = true;
            }
        } catch (Exception oops) {
            tempNumberOfDocumentsInTheIndex = INVALID_COUNT;
            tempNumberOfSearchRequests = INVALID_COUNT;
            tempNumberOfWriteRequests = INVALID_COUNT;
            tempLastMergeTime = INVALID_LAST_MERGE_TIME;
            success = false;
        }

        numberOfSearchRequests = tempNumberOfSearchRequests;
        numberOfWriteRequests = tempNumberOfWriteRequests;
        numberOfDocumentsInTheIndex = tempNumberOfDocumentsInTheIndex;
        lastMergeTime = tempLastMergeTime;
        isValidInfoResponse = success;
    }

    @Override
    public String toString() {
        if (!isValidInfoResponse) {
            return "InfoResponse: error! index unavailable. Printing info...\n" +
                    "RESTful HTTP response code: " + getRESTfulHTTPStatusCode() + "\n" +
                    "RESTful response literal: " + getRESTfulResponseLiteral();
        } else {
            return "InfoResponse: numberOfSearchRequests[ "
                    + getNumberOfSearchRequests() + " ] numberOfWriteRequests[ "
                    + getNumberOfWriteRequests() + " ] numberOfDocumentsInTheIndex[ "
                    + getNumberOfDocumentsInTheIndex() + " ] lastMergeTime[ "
                    + getLastMergeTime() + " ]";
        }
    }

    /**
     * Convenience method for outputting to toasts.
     * @return a human readable description of this object that nicely fits
     * into a toast
     */
    public String toToastString() {
        if (!isValidInfoResponse) {
            return "InfoResponse: error! index unavailable.\n Printing info...\n" +
                    "RESTful HTTP response code: " + getRESTfulHTTPStatusCode() + "\n" +
                    "RESTful response literal:\n" + getRESTfulResponseLiteral();
        } else {
            return "InfoResponse:\nnumberOfSearchRequests[ "
                    + getNumberOfSearchRequests() + " ]\nnumberOfWriteRequests[ "
                    + getNumberOfWriteRequests() + " ]\nnumberOfDocumentsInTheIndex[ "
                    + getNumberOfDocumentsInTheIndex() + " ]\nlastMergeTime[ "
                    + getLastMergeTime() + " ]";
        }
    }
}
