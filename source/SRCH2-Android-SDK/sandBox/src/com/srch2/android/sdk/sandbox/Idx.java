package com.srch2.android.sdk.sandbox;

import android.util.Log;
import com.srch2.android.sdk.*;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class Idx extends Indexable {

    public static final String INDEX_NAME = "name";
    public static final String INDEX_FIELD_NAME_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_NAME_TITLE = "title";
    public static final String INDEX_FIELD_NAME_TITLE2 = "title2";
    public static final String INDEX_FIELD_NAME_SCORE = "score";

    @Override
    public String getIndexName() {
        return INDEX_NAME;
    }

    @Override
    public Schema getSchema() {
        PrimaryKeyField pk = Field.createDefaultPrimaryKeyField(INDEX_FIELD_NAME_PRIMARY_KEY);
        Field title = Field.createSearchableField(INDEX_FIELD_NAME_TITLE).enableHighlighting();
        Field title2 = Field.createSearchableField(INDEX_FIELD_NAME_TITLE2);
        RecordBoostField score = Field.createRecordBoostField(INDEX_FIELD_NAME_SCORE);
        return Schema.createSchema(pk, score, title, title2);

    }

    final public JSONArray getRecords() {
        JSONArray records = new JSONArray();
        try {
            JSONObject o1 = new JSONObject();
            o1.put(INDEX_FIELD_NAME_PRIMARY_KEY, "1");
            o1.put(INDEX_FIELD_NAME_TITLE, "Title Apple One");
            o1.put(INDEX_FIELD_NAME_TITLE2, "b");
            o1.put(INDEX_FIELD_NAME_SCORE, 10);
            records.put(o1);

            o1 = new JSONObject();
            o1.put(INDEX_FIELD_NAME_PRIMARY_KEY, "7");
            o1.put(INDEX_FIELD_NAME_TITLE, "Title Parrot Seven");
            o1.put(INDEX_FIELD_NAME_SCORE, 5);
            o1.put(INDEX_FIELD_NAME_TITLE2, "b");
            records.put(o1);

            o1 = new JSONObject();
            o1.put(INDEX_FIELD_NAME_PRIMARY_KEY, "6");
            o1.put(INDEX_FIELD_NAME_TITLE, "Title Penguin Six");
            o1.put(INDEX_FIELD_NAME_SCORE, 10);
            o1.put(INDEX_FIELD_NAME_TITLE2, "b");
            records.put(o1);

            o1 = new JSONObject();
            o1.put(INDEX_FIELD_NAME_PRIMARY_KEY, "2");
            o1.put(INDEX_FIELD_NAME_TITLE, "Title Chicken Two");
            o1.put(INDEX_FIELD_NAME_SCORE, 90);
            o1.put(INDEX_FIELD_NAME_TITLE2, "b");
            records.put(o1);

            o1 = new JSONObject();
            o1.put(INDEX_FIELD_NAME_PRIMARY_KEY, "3");
            o1.put(INDEX_FIELD_NAME_TITLE, "Title Turkey Three");
            o1.put(INDEX_FIELD_NAME_SCORE, 80);
            o1.put(INDEX_FIELD_NAME_TITLE2, "b");
            records.put(o1);

            o1 = new JSONObject();
            o1.put(INDEX_FIELD_NAME_PRIMARY_KEY, "4");
            o1.put(INDEX_FIELD_NAME_TITLE, "Title Parakeet Four");
            o1.put(INDEX_FIELD_NAME_SCORE, 65);
            o1.put(INDEX_FIELD_NAME_TITLE2, "b");
            records.put(o1);

            o1 = new JSONObject();
            o1.put(INDEX_FIELD_NAME_PRIMARY_KEY, "5");
            o1.put(INDEX_FIELD_NAME_TITLE, "Title Elephant Five");
            o1.put(INDEX_FIELD_NAME_SCORE, 25);
            o1.put(INDEX_FIELD_NAME_TITLE2, "b");
            records.put(o1);

        } catch (JSONException ee) {
        }
        return records;
    }

    static public ArrayList<SearchResultsAdapter.SearchResultItem> wrap(ArrayList<JSONObject> jsonResultsToWrap) {
        ArrayList<SearchResultsAdapter.SearchResultItem> newResults = new ArrayList<SearchResultsAdapter.SearchResultItem>();
        for (JSONObject jsonObject : jsonResultsToWrap) {
            Log.d("SEARCH RESULT OBJECT", jsonObject.toString());
            SearchResultsAdapter.SearchResultItem searchResult = null;
            try {
                searchResult = new SearchResultsAdapter.SearchResultItem(
                        jsonObject.getString(INDEX_FIELD_NAME_TITLE),
                        "Score: " + String.valueOf(jsonObject.getDouble(INDEX_FIELD_NAME_SCORE)),
                        "Primary Key: " + String.valueOf(jsonObject.getInt(INDEX_FIELD_NAME_PRIMARY_KEY)));
            } catch (JSONException oops) {
                continue;
            }

            if (searchResult != null) {
                newResults.add(searchResult);
            }
        }
        return newResults;
    }

    @Override
    public void onIndexReady() {
        super.onIndexReady();
        if (getRecordCount() == 0) {
            insert(getRecords());
        }
    }

    @Override
    public void onInsertComplete(int success, int failed, String JSONResponse) {
        super.onInsertComplete(success, failed, JSONResponse);
    }
}
