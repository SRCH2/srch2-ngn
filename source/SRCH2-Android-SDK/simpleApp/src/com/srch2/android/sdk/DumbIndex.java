package com.srch2.android.sdk;

import org.json.JSONException;
import org.json.JSONObject;

public class DumbIndex extends Indexable {
    public static final String INDEX_NAME = "dumb-index";
    public static final String INDEX_FIELD_NAME_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_NAME_TITLE = "title";

    @Override
    public String getIndexName() {
        return INDEX_NAME;
    }

    @Override
    public Schema getSchema() {
        return Schema.createSchema(
                Field.createDefaultPrimaryKeyField(INDEX_FIELD_NAME_PRIMARY_KEY),
                Field.createSearchableField(INDEX_FIELD_NAME_TITLE));
    }

    @Override
    public void onInsertComplete(int success, int failed, String JSONResponse) {
        super.onInsertComplete(success, failed, JSONResponse);
    }

    @Override
    public void onUpdateComplete(int success, int upserts, int failed, String JSONResponse) {
        super.onUpdateComplete(success, upserts, failed, JSONResponse);
    }

    @Override
    public void onDeleteComplete(int success, int failed, String JSONResponse) {
        super.onDeleteComplete(success, failed, JSONResponse);
    }

    @Override
    public void onGetRecordComplete(boolean success, JSONObject record, String JSONResponse) {
        super.onGetRecordComplete(success, record, JSONResponse);
    }

    @Override
    public void onIndexReady() {
        super.onIndexReady();
    }

    static JSONObject getRecord(String id, String title) {
        JSONObject jo = new JSONObject();
        try {
            jo.put(INDEX_FIELD_NAME_PRIMARY_KEY, id);
            jo.put(INDEX_FIELD_NAME_TITLE, title);
        } catch (JSONException e) {
        }
        return jo;
    }
}
