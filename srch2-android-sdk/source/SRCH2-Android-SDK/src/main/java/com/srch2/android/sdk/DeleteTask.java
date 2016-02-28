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
