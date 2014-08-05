package com.srch2.android.demo.helloworld;

import android.util.Log;

import com.srch2.android.http.service.Field;
import com.srch2.android.http.service.IndexDescription;
import com.srch2.android.http.service.Indexable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class MovieIndex extends Indexable{
    public static final String INDEX_NAME = "movies";

    public static final String INDEX_KEY_PRIMARY_KEY = "id";
    public static final String INDEX_KEY_TITLE = "title";
    public static final String INDEX_KEY_YEAR = "year";
    public static final String INDEX_KEY_GENRE = "genre";

    @Override
    public IndexDescription getIndexDescription() {
        Field primaryKey = Field.getRefiningField(INDEX_KEY_PRIMARY_KEY, Field.Type.TEXT);
        Field title = Field.getSearchableField(INDEX_KEY_TITLE);
        //Field year = Field.getRefiningField(INDEX_KEY_YEAR, Type.INTEGER);
        Field genre = Field.getSearchableField(INDEX_KEY_GENRE);
        return new IndexDescription(INDEX_NAME, primaryKey, title, genre);//, year, genre);
    }

    public void insertRecords() {
        JSONArray recordsJsonArray = new JSONArray();
        try {
            JSONObject sampleRecord1 = new JSONObject();
            sampleRecord1.put(INDEX_KEY_PRIMARY_KEY, "1");
            sampleRecord1.put(INDEX_KEY_TITLE, "The Good, the Bad And the Ugly");
            //sampleRecord1.put(INDEX_KEY_YEAR, 1966);
            sampleRecord1.put(INDEX_KEY_GENRE, "Western Adventure");
            recordsJsonArray.put(sampleRecord1);

            JSONObject sampleRecord2 = new JSONObject();
            sampleRecord2.put(INDEX_KEY_PRIMARY_KEY, "2");
            sampleRecord2.put(INDEX_KEY_TITLE, "Independence Day");
            //sampleRecord2.put(INDEX_KEY_YEAR, 1999);
            sampleRecord2.put(INDEX_KEY_GENRE, "Science Fiction Action");
            recordsJsonArray.put(sampleRecord2);
   /*

            JSONObject sampleRecord3 = new JSONObject();
            sampleRecord3.put(INDEX_KEY_PRIMARY_KEY, "3");
            sampleRecord3.put(INDEX_KEY_TITLE, "The Matrix");
            //sampleRecord.put(INDEX_KEY_YEAR, 1999);
            sampleRecord3.put(INDEX_KEY_GENRE, "Science Fiction Action");
            recordsJsonArray.put(sampleRecord3);

            JSONObject sampleRecord4 = new JSONObject();
            sampleRecord4.put(INDEX_KEY_PRIMARY_KEY, "4");
            sampleRecord4.put(INDEX_KEY_TITLE, "Léon: The Professional");
            //sampleRecord4.put(INDEX_KEY_YEAR, 1994);
            sampleRecord4.put(INDEX_KEY_GENRE, "Crime Drama");
            recordsJsonArray.put(sampleRecord4);

            JSONObject sampleRecord5 = new JSONObject();
            sampleRecord5.put(INDEX_KEY_PRIMARY_KEY, "5");
            sampleRecord5.put(INDEX_KEY_TITLE, "Apocalypse Now");
            //sampleRecord5.put(INDEX_KEY_YEAR, 1979);
            sampleRecord5.put(INDEX_KEY_GENRE, "War Drama");
            recordsJsonArray.put(sampleRecord5);

            JSONObject sampleRecord6 = new JSONObject();
            sampleRecord6.put(INDEX_KEY_PRIMARY_KEY, "6");
            sampleRecord6.put(INDEX_KEY_TITLE, "大红灯笼高高挂 (Raise the Red Lantern)");
            //sampleRecord6.put(INDEX_KEY_YEAR, 1991);
            sampleRecord6.put(INDEX_KEY_GENRE, "Drama");
            recordsJsonArray.put(sampleRecord6);

            JSONObject sampleRecord7 = new JSONObject();
            sampleRecord7.put(INDEX_KEY_PRIMARY_KEY, "7");
            sampleRecord7.put(INDEX_KEY_TITLE, "Toy Story");
            //sampleRecord7.put(INDEX_KEY_YEAR, 1995);
            sampleRecord7.put(INDEX_KEY_GENRE, "Family Comedy");
            recordsJsonArray.put(sampleRecord7);

            JSONObject sampleRecord8 = new JSONObject();
            sampleRecord8.put(INDEX_KEY_PRIMARY_KEY, "8");
            sampleRecord8.put(INDEX_KEY_TITLE, "Citizen Kane");
            //sampleRecord8.put(INDEX_KEY_YEAR, 1941);
            sampleRecord8.put(INDEX_KEY_GENRE, "Mystery Drama");
            recordsJsonArray.put(sampleRecord8);

            JSONObject sampleRecord9 = new JSONObject();
            sampleRecord9.put(INDEX_KEY_PRIMARY_KEY, "9");
            sampleRecord9.put(INDEX_KEY_TITLE, "Back to the Future");
            //sampleRecord9.put(INDEX_KEY_YEAR, 1985);
            sampleRecord9.put(INDEX_KEY_GENRE, "Science Fiction Comedy");
            recordsJsonArray.put(sampleRecord9);

            JSONObject sampleRecord10 = new JSONObject();
            sampleRecord10.put(INDEX_KEY_PRIMARY_KEY, "10");
            sampleRecord10.put(INDEX_KEY_TITLE, "Alien");
            //sampleRecord.put(INDEX_KEY_YEAR, 1979);
            sampleRecord10.put(INDEX_KEY_GENRE, "Science Fiction Horror");
            recordsJsonArray.put(sampleRecord10); */
/*
            Log.d("srch2:: demo index", "INSERTING 20k");
            int totalCount = 20000;
            for (int i = 11; i < totalCount; ++i) {

                if (i % 1000 == 0) {
                    Log.d("srch2:: demo index", "at count " + i);
                }

                JSONObject record = new JSONObject();
                record.put(INDEX_KEY_PRIMARY_KEY, i);
                record.put(INDEX_KEY_TITLE, getRandomString());
                record.put(INDEX_KEY_GENRE, getRandomString());
                recordsJsonArray.put(record);
            }
            Log.d("srch2:: demo index", "finished INSERTING 20k");*/
        } catch (JSONException oops) {
        }


        super.insert(recordsJsonArray);
    }

    private final static char[] chars = "abcdefghijklmnopqrstuvwxyz".toCharArray();
    private final static StringBuilder sb = new StringBuilder();
    static String getRandomString() {
        sb.setLength(0);
        int randomLength = (int) ((Math.random() * 20) + 5);
        for (int i = 0; i < randomLength; ++i) {
            sb.append(chars[(int) (Math.random() * chars.length)]);
        }
        return sb.toString();
    }
}
