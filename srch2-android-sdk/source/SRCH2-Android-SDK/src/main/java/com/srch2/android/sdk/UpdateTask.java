/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
package com.srch2.android.sdk;


import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;

class UpdateTask extends HttpTask.InsertUpdateDeleteTask {
    private static final String TAG = "UpdateTask";

    private JSONArray recordsToUpdate;
    private JSONObject recordToUpdate;
    private boolean isSingleUpdate = false;


    public UpdateTask(URL theTargetUrl, String theTargetCoreName, JSONArray recordsToBeUpdated) {
        super(theTargetUrl, theTargetCoreName);
        recordsToUpdate = recordsToBeUpdated;
    }

    public UpdateTask(URL theTargetUrl, String theTargetCoreName, JSONObject recordToBeUpdated) {
        super(theTargetUrl, theTargetCoreName);
        isSingleUpdate = true;
        recordToUpdate = recordToBeUpdated;
    }

    @Override
    public void run() {
        if (isSingleUpdate) {
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
            response = prepareIOExceptionMessageForCallback();
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
            response = handleIOExceptionMessagePassing(e, response, TAG);
            if (response.contains(SRCH2Engine.ExceptionMessages.IO_EXCEPTION_ECONNREFUSED_CONNECTION_REFUSED)) {
                onTaskCrashedSRCH2SearchServer();
            }
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
        int successUpdate = 0;
        int successInsert = 0;
        int failureCount = 0;

        try {
            JSONObject rootNode = new JSONObject(jsonResponse);
            JSONArray logNode = rootNode.getJSONArray(RESTfulResponseTags.JSON_KEY_LOG);

            int length = logNode.length();
            for (int i = 0; i < length; ++i) {
                JSONObject recordStamp = (JSONObject) logNode.get(i);
                if (recordStamp.has(RESTfulResponseTags.JSON_KEY_PRIMARY_KEY_INDICATOR)) {
                    boolean updateFail = recordStamp.getString(RESTfulResponseTags.JSON_KEY_UPDATE).equals(RESTfulResponseTags.JSON_VALUE_UPDATE_FAIL);
                    if (updateFail) {
                        ++failureCount;
                    } else {
                        if (recordStamp.getString(RESTfulResponseTags.JSON_KEY_UPDATE).equals(RESTfulResponseTags.JSON_VALUE_UPDATED_EXISTS)){
                            ++successUpdate;
                        } else if (recordStamp.getString(RESTfulResponseTags.JSON_KEY_UPDATE).equals(RESTfulResponseTags.JSON_VALUE_UPSERT_SUCCESS)){
                            ++successInsert;
                        }else {
                            ++failureCount;
                        }
                    }
                }
            }
        } catch (JSONException oops) {
            successInsert = RESTfulResponseTags.INVALID_JSON_RESPONSE;
            successUpdate = RESTfulResponseTags.INVALID_JSON_RESPONSE;
            failureCount =RESTfulResponseTags.INVALID_JSON_RESPONSE;
        }
        onExecutionCompleted(TASK_ID_INSERT_UPDATE_DELETE_GETRECORD);
        HttpTask.executeTask(new UpdateResponse(targetCoreName, successUpdate, successInsert, failureCount, jsonResponse));
    }
}
