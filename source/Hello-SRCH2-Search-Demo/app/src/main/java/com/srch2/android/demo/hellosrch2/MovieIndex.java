package com.srch2.android.demo.hellosrch2;

import com.srch2.android.http.service.Field;
import com.srch2.android.http.service.IndexDescription;
import com.srch2.android.http.service.Indexable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class MovieIndex extends Indexable {

    // The name of the index. Used to identify the index when calling on the SRCH2Engine.
    public static final String INDEX_NAME = "movies";

    // The fields defining the schema of this index
    public static final String INDEX_FIELD_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_TITLE = "title";
    public static final String INDEX_FIELD_YEAR = "year";
    public static final String INDEX_FIELD_GENRE = "genre";

    @Override
    public IndexDescription getIndexDescription() {
        // Each index needs to be defined in terms of its schema: fields are comparable to the
        // the columns of an SQLite database table. A primary key field, whose value must be unique
        // for each record, is always required.

        Field primaryKey = Field.getRefiningField(INDEX_FIELD_PRIMARY_KEY,
                Field.Type.INTEGER);
        Field title = Field.getSearchableField(INDEX_FIELD_TITLE, 3);
        Field year = Field.getSearchableAndRefiningField(INDEX_FIELD_YEAR,
                Field.Type.INTEGER);
        Field genre = Field.getSearchableField(INDEX_FIELD_GENRE);

        // Enables the SRCH2Engine to configure the SRCH2 server to add this index.
        return new IndexDescription(INDEX_NAME, primaryKey, title, year, genre);
    }

    public static JSONArray getAFewRecordsToInsert() {
        // Records are inserted into an Indexable instance in the form of JSONObjects. For batch
        // insertion, insert the JSONObjects representing the records into a JSONArray. 

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
