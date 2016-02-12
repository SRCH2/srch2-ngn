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
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
public abstract class TestableIndex extends Indexable {


    @Override
    public void onInsertComplete(int success, int failed, String JSONResponse) {
        super.onInsertComplete(success, failed, JSONResponse);
        insertResponse = JSONResponse;
        insertSuccessCount = success;
        insertFailedCount = failed;
    }

    @Override
    public void onUpdateComplete(int success, int upserts, int failed, String JSONResponse) {
        super.onUpdateComplete(success, upserts, failed, JSONResponse);
        updateResponse = JSONResponse;
        updateSuccessCount = success;
        updateFailedCount = failed;
        upsertSuccessCount = upserts;
    }

    @Override
    public void onDeleteComplete(int success, int failed, String JSONResponse) {
        super.onDeleteComplete(success, failed, JSONResponse);
        deleteResponse = JSONResponse;
        deleteSuccessCount = success;
        deleteFailedCount = failed;
    }

    @Override
    public void onGetRecordComplete(boolean success, JSONObject record, String JSONResponse) {
        super.onGetRecordComplete(success, record, JSONResponse);
        getRecordResponse = JSONResponse;
        recordRetrievedSuccess = success;
        recordRetreived = record;
    }

    @Override
    public void onIndexReady() {
        super.onIndexReady();
        indexIsReadyCalled = true;
    }

    // NOTE: the above is the callbacks from stateresponselistener (now goto indexable)
    // NOTE: below is the fields of the (now deprecated) response classes: after one of the
    // callbacks above is executed, it'll dump the values of its parameters into the
    // corresponding fields
    // Util.wait now waits for the String *response, in my MyActivity, after the waiting
    // is complete, since the values will have been set, the tests can use these fields
    // to confirm (insertcount = expected, for instance). After that, it can be
    // reset next time. Thus the previous Stateresponselistener is now a part of this
    // class, since these fields will apply FOR ALL subclasses of this class

    String insertResponse;
    int insertSuccessCount, insertFailedCount;
    String deleteResponse;
    int deleteSuccessCount, deleteFailedCount;
    String updateResponse;
    int updateSuccessCount, upsertSuccessCount, updateFailedCount;
    String getRecordResponse;
    boolean recordRetrievedSuccess;
    JSONObject recordRetreived;
    boolean indexIsReadyCalled = false;

    public void resetGetRecordResponseFields() {
        getRecordResponse = null;
        recordRetrievedSuccess = false;
        recordRetreived = null;
    }

    public void resetInsertResponseFields() {
        insertResponse = null;
        insertSuccessCount = insertFailedCount = -1;
    }

    public void resetUpdateResponseFields() {
        updateResponse = null;
        updateSuccessCount = upsertSuccessCount = updateFailedCount = -1;
    }

    public void resetDeleteResponseFields() {
        deleteResponse = null;
        deleteSuccessCount = deleteFailedCount = -1;
    }




    public abstract JSONObject getSucceedToInsertRecord() ;

    public abstract JSONObject getFailToInsertRecord();

    public abstract List<String> getSucceedToSearchString(JSONArray records);

    public abstract List<String> getFailToSearchString(JSONArray records);

    public abstract List<Query> getSucceedToSearchQuery(JSONArray records);

    public abstract List<Query> getFailToSearchQuery(JSONArray records);

    public abstract JSONObject getFailToUpdateRecord();

    public abstract String getPrimaryKeyFieldName();

    public abstract List<String> getFailToDeleteRecord();

    public abstract JSONArray getSucceedToInsertBatchRecords();

    public abstract JSONArray getFailToInsertBatchRecord();

    public abstract JSONArray getSucceedToUpdateBatchRecords();

    public abstract JSONArray getFailToUpdateBatchRecords();

    public abstract boolean verifyResult(String query, ArrayList<JSONObject> jsonObjects);

    public abstract boolean verifyResult(Query query, ArrayList<JSONObject> jsonObjects);

    public abstract JSONObject getSucceedToUpdateExistRecord();

    public abstract JSONObject getSucceedToUpsertRecord();
}
