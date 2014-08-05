package com.srch2.android.http.service;

import java.io.BufferedOutputStream;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;

import org.json.JSONArray;
import org.json.JSONObject;

import android.util.Log;

import com.srch2.android.http.service.HttpTask.ControlHttpTask;

class UpdateTask extends ControlHttpTask {
    private static final String TAG = "UpdateTask";


    private JSONArray recordsToUpdate;
    private JSONObject recordToUpdate;
    private boolean isSingleInsertion = false;


    public UpdateTask(URL theTargetUrl, String theTargetCoreName, ControlResponseListener theControlResponseListener, JSONArray recordsToBeUpdated) {
        super(theTargetUrl, theTargetCoreName, theControlResponseListener);
        recordsToUpdate = recordsToBeUpdated;
    }

    public UpdateTask(URL theTargetUrl, String theTargetCoreName, ControlResponseListener theControlResponseListener, JSONObject recordToBeUpdated) {
        super(theTargetUrl, theTargetCoreName, theControlResponseListener);
        isSingleInsertion = true;
        recordToUpdate = recordToBeUpdated;
    }

    @Override
    public void run() {
        if (isSingleInsertion) {
            doSingleUpdate();
        } else {
            doBatchUpdate();
        }
    }

    private void doSingleUpdate() {
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
            writer.write(recordToUpdate.toString());
            writer.flush();

            responseCode = connection.getResponseCode();
            response = handleStreams(connection, TAG);
        } catch (IOException e) {
            handleIOExceptionMessagePassing(e, response, TAG);
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
            response = RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE;
        }
        onTaskComplete(responseCode, response);
    }

    private void doBatchUpdate() {
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
            writer.write(recordsToUpdate.toString());
            writer.flush();

            responseCode = connection.getResponseCode();
            response = handleStreams(connection, TAG);
        } catch (IOException e) {
            handleIOExceptionMessagePassing(e, response, TAG);
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
            response = RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE;
        }
        onTaskComplete(responseCode, response);
    }


    @Override
    protected void onTaskComplete(int returnedResponseCode, String returnedResponseLiteral) {
        if (controlResponseObserver != null) {
            UpdateResponse updateResponse;
            if (returnedResponseLiteral == null || returnedResponseLiteral.equals(RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE)) {
                updateResponse = new UpdateResponse(RestfulResponse.FAILED_TO_CONNECT_RESPONSE_CODE, RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE);
            } else {
                updateResponse = new UpdateResponse(returnedResponseCode, returnedResponseLiteral);
            }
            controlResponseObserver.onUpdateRequestComplete(targetCoreName, updateResponse);
        }

        super.onTaskComplete(returnedResponseCode, returnedResponseLiteral);
    }

}
