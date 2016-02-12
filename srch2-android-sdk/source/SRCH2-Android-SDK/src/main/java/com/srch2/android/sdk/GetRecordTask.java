package com.srch2.android.sdk;

import org.json.JSONObject;

import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;

class GetRecordTask extends SearchTask {

    GetRecordTask(URL url, String nameOfTheSingleCoreToQuery) {
        super(url, nameOfTheSingleCoreToQuery, null);
    }

    @Override
    protected void onTaskComplete(int returnedResponseCode,
                                  String returnedResponseLiteral) {
        parseJsonResponseAndPushToCallbackThread(returnedResponseLiteral);
    }

    void parseJsonResponseAndPushToCallbackThread(String jsonResponse) {

        boolean isRecordRetrieved = false;
        JSONObject record = new JSONObject();
        try {
            HashMap<String, ArrayList<JSONObject>> resultMap = SearchTask
                    .parseResponseForRecordResults(
                            jsonResponse, false,
                            targetCoreName);
            ArrayList<JSONObject> jsonArray = resultMap.get(targetCoreName);

            if (jsonArray != null && jsonArray.size() > 0) {
                isRecordRetrieved = true;
                record = jsonArray.get(0);
            }
        } catch (Exception oops) {
            isRecordRetrieved = false;
        }

        if (record == null) {
            isRecordRetrieved = false;
        }
        onExecutionCompleted(TASK_ID_INSERT_UPDATE_DELETE_GETRECORD);
        HttpTask.executeTask(new GetRecordResponse(targetCoreName, isRecordRetrieved, record, jsonResponse));
    }
}
