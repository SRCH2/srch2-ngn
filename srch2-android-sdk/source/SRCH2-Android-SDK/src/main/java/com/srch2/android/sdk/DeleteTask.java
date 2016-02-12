
package com.srch2.android.sdk;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;


import java.net.HttpURLConnection;
import java.net.URL;

class DeleteTask extends HttpTask.InsertUpdateDeleteTask {

    private static final String TAG = "DeleteTask";

    DeleteTask(URL insertUrl, String theTargetCoreName) {
        super(insertUrl, theTargetCoreName);
    }

    @Override
    public void run() {
        doDelete();
    }

    private void doDelete() {
        HttpURLConnection connection = null;

        int responseCode = -1;
        String response = null;

        Cat.d(TAG, "targeturl for DELETION is " + targetUrl);

        try {
            connection = (HttpURLConnection) targetUrl.openConnection();
            connection.setRequestMethod("DELETE");
            connection.setUseCaches(false);
            connection.connect();

            responseCode = connection.getResponseCode();
            response = handleStreams(connection, TAG);
        } catch (IOException e) {
            response = handleIOExceptionMessagePassing(e, response, TAG);
            if (response.contains(SRCH2Engine.ExceptionMessages.IO_EXCEPTION_ECONNREFUSED_CONNECTION_REFUSED)) {
                onTaskCrashedSRCH2SearchServer();
            }
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }

        if (response == null) {
            response = prepareIOExceptionMessageForCallback();
        }

        onTaskComplete(responseCode, response);
    }


    @Override
    protected void onTaskComplete(int returnedResponseCode, String returnedResponseLiteral) {
        super.onTaskComplete(returnedResponseCode, returnedResponseLiteral);
        parseJsonResponseAndPushToCallbackThread(returnedResponseLiteral);
    }

    void parseJsonResponseAndPushToCallbackThread(String jsonResponse) {
        boolean success = false;
        try {
            JSONObject rootNode = new JSONObject(jsonResponse);
            JSONArray logNode = rootNode.getJSONArray(RESTfulResponseTags.JSON_KEY_LOG);
            int length = logNode.length();
            if (length == 1) {
                JSONObject recordStamp = (JSONObject) logNode.get(0);
                if (recordStamp.has(RESTfulResponseTags.JSON_KEY_PRIMARY_KEY_INDICATOR)) {
                    success = recordStamp.getString(RESTfulResponseTags.JSON_KEY_DELETE).equals(RESTfulResponseTags.JSON_VALUE_SUCCESS);
                }
            }
        } catch (JSONException oops) {
            success = false;
        }

        int successCount = 1;
        int deleteCount = 1;
        if (success) {
            deleteCount = 0;
        } else {
            successCount = 0;
        }
        onExecutionCompleted(TASK_ID_INSERT_UPDATE_DELETE_GETRECORD);
        HttpTask.executeTask(new DeleteResponse(targetCoreName, successCount, deleteCount, jsonResponse));
    }

}