package com.srch2.android.sdk.sandbox;

import com.srch2.android.sdk.Field;
import com.srch2.android.sdk.Indexable;
import com.srch2.android.sdk.PrimaryKeyField;
import com.srch2.android.sdk.Schema;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class Idx extends Indexable {

    public static final String INDEX_NAME = "name";
    public static final String INDEX_FIELD_NAME_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_NAME_TITLE = "title";
    public static final String INDEX_FIELD_NAME_SCORE = "score";

    @Override
    public String getIndexName() {
        return INDEX_NAME;
    }

    @Override
    public Schema getSchema() {
        PrimaryKeyField pk = Field.createDefaultPrimaryKeyField(INDEX_FIELD_NAME_PRIMARY_KEY);
        Field title = Field.createSearchableField(INDEX_FIELD_NAME_TITLE);
        //  RecordBoostField score = Field.createRecordBoostField(INDEX_FIELD_NAME_SCORE);
        //   return Schema.createSchema(pk, score, title);
        return Schema.createSchema(pk, title);
    }

    final public JSONArray getRecords() {
        JSONArray records = new JSONArray();
        try {
            JSONObject o1 = new JSONObject();
            o1.put(INDEX_FIELD_NAME_PRIMARY_KEY, "1");
            o1.put(INDEX_FIELD_NAME_TITLE, "titleOne 1");
            records.put(o1);
        } catch (JSONException ee) {
        }
        return records;
    }

    static public ArrayList<SearchResultsAdapter.SearchResultItem> wrap(ArrayList<JSONObject> jsonResultsToWrap) {
        ArrayList<SearchResultsAdapter.SearchResultItem> newResults = new ArrayList<SearchResultsAdapter.SearchResultItem>();
        for (JSONObject jsonObject : jsonResultsToWrap) {
            SearchResultsAdapter.SearchResultItem searchResult = null;
            try {
                searchResult = new SearchResultsAdapter.SearchResultItem(
                        jsonObject.getString(INDEX_FIELD_NAME_TITLE),
                        " ",
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


}
