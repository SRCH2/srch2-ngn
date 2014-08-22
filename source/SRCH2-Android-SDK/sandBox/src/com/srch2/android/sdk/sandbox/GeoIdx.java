package com.srch2.android.sdk.sandbox;

import com.srch2.android.sdk.Field;
import com.srch2.android.sdk.Indexable;
import com.srch2.android.sdk.PrimaryKeyField;
import com.srch2.android.sdk.Schema;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Created by ashton on 8/22/2014.
 */
public class GeoIdx extends Indexable {

    @Override
    public void onIndexReady() {
        super.onIndexReady();

        if (getRecordCount() == 0) {

        }
    }

    public JSONArray getRecords() {
        JSONArray recordArray = new JSONArray();
        try {

            JSONObject jo = new JSONObject();
            jo.put(INDEX_FIELD_PK, 1);
            jo.put(INDEX_FIELD_LATITUDE, "30");
            jo.put(INDEX_FIELD_LONGITUDE, "30");
            jo.put(INDEX_FIELD_NAME, "name one");
            recordArray.put(jo);

            jo = new JSONObject();
            jo.put(INDEX_FIELD_PK, 1);
            jo.put(INDEX_FIELD_LATITUDE, "30");
            jo.put(INDEX_FIELD_LONGITUDE, "30");
            jo.put(INDEX_FIELD_NAME, "name one");
            recordArray.put(jo);

        } catch (JSONException e) {
        }
        return recordArray;
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

    public static final String INDEX_NAME = "geo";
    public static final String INDEX_FIELD_PK = "id";
    public static final String INDEX_FIELD_NAME = "name";
    public static final String INDEX_FIELD_LATITUDE = "lat";
    public static final String INDEX_FIELD_LONGITUDE = "long";

    @Override
    public String getIndexName() {
        return INDEX_NAME;
    }

    @Override
    public Schema getSchema() {
        PrimaryKeyField pk = Field.createDefaultPrimaryKeyField(INDEX_FIELD_PK);
        Field f = Field.createSearchableField(INDEX_FIELD_NAME);
        return Schema.createGeoSchema(pk, INDEX_FIELD_LATITUDE, INDEX_FIELD_LONGITUDE, f);
    }
}
