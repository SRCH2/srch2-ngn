package com.srch2.android.http.service;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by jianfeng on 8/4/14.
 */
public abstract class TestableIndex extends Indexable {
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
