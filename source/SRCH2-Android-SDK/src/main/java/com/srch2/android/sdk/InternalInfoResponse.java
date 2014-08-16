package com.srch2.android.sdk;

import org.json.JSONObject;

final class InternalInfoResponse {

    static final String INVALID_LAST_MERGE_TIME = "not-set";
    static final int INVALID_COUNT = Indexable.INDEX_RECORD_COUNT_NOT_SET;

    final int numberOfSearchRequests;
    final int numberOfWriteRequests;
    final int numberOfDocumentsInTheIndex;
    final String lastMergeTime;
    final boolean isValidInfoResponse;

    InternalInfoResponse(int theHttpResponseCode, String theRestfulResponseLiteral) {
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
        return "InfoResponse: numberOfSearchRequests[ "
                + numberOfSearchRequests + " ] numberOfWriteRequests[ "
                + numberOfWriteRequests + " ] numberOfDocumentsInTheIndex[ "
                + numberOfDocumentsInTheIndex + " ] lastMergeTime[ "
                + lastMergeTime + " ]";
    }

    String toToastString() {
        return "InfoResponse: valid [ " + isValidInfoResponse  + " ]\n"
                + " numberOfSearchRequests[ "
                + numberOfSearchRequests + " ] numberOfWriteRequests[ "
                + numberOfWriteRequests + " ] numberOfDocumentsInTheIndex[ "
                + numberOfDocumentsInTheIndex + " ] lastMergeTime[ "
                + lastMergeTime + " ]";

    }
}
