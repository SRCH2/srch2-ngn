package com.srch2.android.sdk;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;

class InsertTask extends HttpTask.InsertUpdateDeleteTask {
    private static final String TAG = "srch2:: InsertTask";

    private JSONArray recordsToInsert;
    private JSONObject recordToInsert;
    private boolean isSingleInsertion = false;

    public InsertTask(URL theTargetUrl, String theTargetCoreName, JSONArray recordsToBeInserted) {
        super(theTargetUrl, theTargetCoreName);
        recordsToInsert = recordsToBeInserted;
    }

    public InsertTask(URL theTargetUrl, String theTargetCoreName, JSONObject recordToBeInserted) {
        super(theTargetUrl, theTargetCoreName);
        isSingleInsertion = true;
        recordToInsert = recordToBeInserted;
    }

    @Override
    public void run() {
        if (isSingleInsertion) {
            doSingleInsert();
        } else {
            doBatchInsert();
        }
    }

    private void doSingleInsert() {
        HttpURLConnection connection = null;
        OutputStream outStream = null;
        BufferedWriter writer = null;

        int responseCode = -1;
        String response = null;

        try {
            connection = (HttpURLConnection) targetUrl.openConnection();
            connection.setDoOutput(true);
            connection.setRequestMethod("PUT");
            connection.setUseCaches(false);
            connection.connect();

            outStream = new BufferedOutputStream(connection.getOutputStream());
            writer = new BufferedWriter(new OutputStreamWriter(outStream, "UTF-8"));
            writer.write(recordToInsert.toString());
            writer.flush();

            responseCode = connection.getResponseCode();
            response = handleStreams(connection, TAG);
        } catch (IOException e) {
            response = handleIOExceptionMessagePassing(e, response, TAG);
        } finally {
            if (outStream != null) {
                try {
                    outStream.close();
                } catch (IOException ignore) {
                }
            }
            if (writer != null) {
                try {
                    writer.close();
                } catch (IOException ignore) {
                }
            }
            if (connection != null) {
                connection.disconnect();
            }
        }

        if (response == null) {
            response = Indexable.IRRECOVERABLE_NETWORK_ERROR_MESSAGE;
        }
        onTaskComplete(responseCode, response);
    }

    private void doBatchInsert() {
        HttpURLConnection connection = null;
        OutputStream outStream = null;
        BufferedWriter writer = null;

        int responseCode = -1;
        String response = null;

        try {
            connection = (HttpURLConnection) targetUrl.openConnection();
            connection.setDoOutput(true);
            connection.setRequestMethod("PUT");
            connection.setUseCaches(false);
            connection.connect();

            outStream = new BufferedOutputStream(connection.getOutputStream());
            writer = new BufferedWriter(new OutputStreamWriter(outStream, "UTF-8"));
            writer.write(recordsToInsert.toString());
            writer.flush();

            responseCode = connection.getResponseCode();
            response = handleStreams(connection, TAG);
        } catch (IOException e) {
            response = handleIOExceptionMessagePassing(e, response, TAG);
        } finally {
            if (outStream != null) {
                try {
                    outStream.close();
                } catch (IOException ignore) {
                }
            }
            if (writer != null) {
                try {
                    writer.close();
                } catch (IOException ignore) {
                }
            }
            if (connection != null) {
                connection.disconnect();
            }
        }

        if (response == null) {
            response = Indexable.IRRECOVERABLE_NETWORK_ERROR_MESSAGE;
        }

        onTaskComplete(responseCode, response);
    }


    @Override
    protected void onTaskComplete(int returnedResponseCode, String returnedResponseLiteral) {
        super.onTaskComplete(returnedResponseCode, returnedResponseLiteral);
        parseJsonResponseAndPushToCallbackThread(returnedResponseLiteral);
    }

    void parseJsonResponseAndPushToCallbackThread(String jsonResponse) {
        int successCount = 0;
        int failureCount = 0;
        try {
            JSONObject rootNode = new JSONObject(jsonResponse);
            JSONArray logNode = rootNode.getJSONArray(RESTfulResponseTags.JSON_KEY_LOG);
            int length = logNode.length();
            for (int i = 0; i < length; ++i) {
                JSONObject recordStamp = (JSONObject) logNode.get(i);
                if (recordStamp.has(RESTfulResponseTags.JSON_KEY_PRIMARY_KEY_INDICATOR)) {
                    boolean insertSuccess = recordStamp.getString(RESTfulResponseTags.JSON_KEY_INSERT).equals(RESTfulResponseTags.JSON_VALUE_SUCCESS);
                    if (insertSuccess) {
                        ++successCount;
                    } else {
                        ++failureCount;
                    }
                }
            }
        } catch (JSONException oops) {
            successCount = RESTfulResponseTags.INVALID_JSON_RESPONSE;
            failureCount = RESTfulResponseTags.INVALID_JSON_RESPONSE;
        }

        HttpTask.executeTask(new InsertResponse(targetCoreName, successCount, failureCount, jsonResponse));
    }
}
