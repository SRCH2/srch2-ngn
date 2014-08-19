package com.srch2.android.sdk;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
public abstract class TestableIndex extends Indexable {


    @Override
    public void onInsertComplete(int success, int failed, String JSONResponse) {
        insertResponse = JSONResponse;
        insertSuccessCount = success;
        insertFailedCount = failed;
    }

    @Override
    public void onUpdateComplete(int success, int upserts, int failed, String JSONResponse) {
        updateResponse = JSONResponse;
        updateSuccessCount = success;
        updateFailedCount = failed;
        upsertSuccessCount = upserts;
    }

    @Override
    public void onDeleteComplete(int success, int failed, String JSONResponse) {
        deleteResponse = JSONResponse;
        deleteSuccessCount = success;
        deleteFailedCount = failed;
    }

    @Override
    public void onGetRecordComplete(boolean success, JSONObject record, String JSONResponse) {
        getRecordResponse = JSONResponse;
        recordRetrievedSuccess = success;
        recordRetreived = record;
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

    public void resetStateResponseFields() {
        insertResponse = deleteResponse = updateResponse = getRecordResponse = null;
        insertSuccessCount = insertFailedCount = deleteSuccessCount = deleteFailedCount =
                updateSuccessCount = upsertSuccessCount = updateFailedCount = -1;
        recordRetrievedSuccess = false;
        recordRetreived = null;
    }

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

    public abstract JSONObject getSucceedToInsertRecord();

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
