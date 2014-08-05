package com.srch2.android.demo.hellosrch2;

import com.srch2.android.http.service.Field;
import com.srch2.android.http.service.IndexDescription;
import com.srch2.android.http.service.Indexable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class MovieIndex extends Indexable {

    public static final String INDEX_NAME = "movies";

    public static final String INDEX_FIELD_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_TITLE = "title";
    public static final String INDEX_FIELD_YEAR = "year";
    public static final String INDEX_FIELD_GENRE = "genre";

    @Override
    public IndexDescription getIndexDescription() {
        Field primaryKey = Field.getRefiningField(INDEX_FIELD_PRIMARY_KEY, Field.Type.INTEGER);
        Field title = Field.getSearchableField(INDEX_FIELD_TITLE, 3);
        Field year = Field.getRefiningField(INDEX_FIELD_YEAR, Field.Type.INTEGER);
        Field genre = Field.getSearchableField(INDEX_FIELD_GENRE);
        return new IndexDescription(INDEX_NAME, primaryKey, title, year, genre);
    }

    public static JSONArray getAFewRecordsToInsert() {
        JSONArray jsonRecordsToInsert = new JSONArray();
        try {
            JSONObject record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 1);
            record.put(INDEX_FIELD_TITLE, "The Good, the Bad And the Ugly");
            record.put(INDEX_FIELD_YEAR, 1966);
            record.put(INDEX_FIELD_GENRE, "Western Adventure");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 2);
            record.put(INDEX_FIELD_TITLE, "Citizen Kane");
            record.put(INDEX_FIELD_YEAR, 1941);
            record.put(INDEX_FIELD_GENRE, "Mystery Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 3);
            record.put(INDEX_FIELD_TITLE, "大红灯笼高高挂 (Raise the Red Lantern)");
            record.put(INDEX_FIELD_YEAR, 1991);
            record.put(INDEX_FIELD_GENRE, "Drama");
            jsonRecordsToInsert.put(record);
        } catch (JSONException oops) {
            // We know there are no errors
        }
        return jsonRecordsToInsert;
    }
}
