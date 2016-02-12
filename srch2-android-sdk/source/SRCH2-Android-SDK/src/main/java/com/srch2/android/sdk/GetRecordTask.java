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
