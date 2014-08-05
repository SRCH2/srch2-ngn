package com.srch2.android.http.service;

import org.json.JSONObject;

/**
 * A concrete implementation of the superclass <code>RestfulResponse</code> that
 * wraps the String literal result from doing an /info request on the SRCH2 http
 * restful server.
 * <p/>
 * Users of this class can directly access its field members, such as
 * <code>numberOfSearchRequests</code> and <code>lastMergeTime</code>, to avoid
 * having to parse the String value of <code>restfulResponseLiteral</code>
 * themselves.
 */
public final class InfoResponse extends RestfulResponse {

    /**
     * Indicates the InfoResponse was unable to be formed from the JSON response from the SRCH2 server upon performing an info command. Has the <bold>constant</bold> value <code>"null"</code>.
     */
    public static final String INVALID_LAST_MERGE_TIME = "null";



    /**
     * Returns the number of search requests that have been made on the specified index.
     *
     * @return the number of search requests
     */
    public int getNumberOfSearchRequests() { return numberOfSearchRequests; }
    private final int numberOfSearchRequests;

    /**
     * Returns the number of write requests (insert, update, and delete) that have been made on the specified index.
     *
     * @return the number of write requests
     */
    public int getNumberOfWriteRequests() { return numberOfWriteRequests; }
    private final int numberOfWriteRequests;

    /**
     * Returns the number of records that are currently in the index.
     *
     * @return the number of records
     */
    public int getNumberOfDocumentsInTheIndex() { return numberOfDocumentsInTheIndex; }
    private final int numberOfDocumentsInTheIndex;

    /**
     * Returns the last time the specified index was merged--that is, the last time
     * the specified index was updated to reflect any pending write
     * requests prior to this time.
     * <p/>
     * <b>Note</b>: The value of <code>lastMergeTime</code> may be set to "null"
     * if the response from the server was unable to be parsed correctly.
     *
     * @return a time-stamp of the last time the index was merged
     */
    public String getLastMergeTime() { return lastMergeTime == null ? INVALID_LAST_MERGE_TIME : lastMergeTime; }
    private final String lastMergeTime;

    /**
     * Returns whether this <code>InfoResponse</code> was parsed correctly. If <b>true</b>, the specified index is
     * up, running and searchable; if <b>false</b>, this indicates the index is unavailable and <code>getNumberOfSearchRequests()</code>,
     * <code>getNumberOfWriteRequests()</code>, and <code>getNumberOfDocumentsInTheIndex()</code> will return -1 or <code>INVALID_COUNT</code>
     * and <code>getLastMergeTime()</code> will return the literal value "null" or <code>INVALID_LAST_MERGE_TIME</code>.
     *
     * @return whether this <code>InfoResponse</code> was formed properly from the JSON response indicating whether the index is available
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

    /**
     * Returns a concise, human readable description of InfoResponse object.
     *
     * @return a human readable description of InfoResponse object
     */
    public String toHumanReadableString() {
        if (httpResponseCode == -1) {
            return "InfoResponse: error! index unavailable";
        }
        return "InfoResponse: numberOfSearchRequests[ "
                + getNumberOfSearchRequests() + " ] numberOfWriteRequests[ "
                + getNumberOfWriteRequests() + " ] numberOfDocumentsInTheIndex[ "
                + getNumberOfDocumentsInTheIndex() + " ] lastMergeTime[ "
                + getLastMergeTime() + " ]";
    }

    /**
     * Returns a concise, human readable description of InfoResponse object that
     * will fit in a toast such that each field member is on its own line.
     *
     * @return a human readable description of InfoResponse object that nicely fits into
     * a toast
     */
    public String toToastString() {
        if (httpResponseCode == -1) {
            return "InfoResponse: error! index unavailable";
        }
        return "InfoResponse:\nnumberOfSearchRequests[ "
                + getNumberOfSearchRequests() + " ]\nnumberOfWriteRequests[ "
                + getNumberOfWriteRequests() + " ]\nnumberOfDocumentsInTheIndex[ "
                + getNumberOfDocumentsInTheIndex() + " ]\nlastMergeTime[ "
                + getLastMergeTime() + " ]";
    }
}
