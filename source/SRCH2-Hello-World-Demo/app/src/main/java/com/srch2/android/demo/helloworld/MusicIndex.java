package com.srch2.android.demo.helloworld;

import com.srch2.android.http.service.Field;
import com.srch2.android.http.service.IndexDescription;
import com.srch2.android.http.service.Indexable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class MusicIndex extends Indexable{
    public static final String INDEX_NAME = "music";

    public static final String INDEX_KEY_PRIMARY_KEY = "id";
    public static final String INDEX_KEY_SONG_TITLE = "song";
    public static final String INDEX_KEY_SONG_YEAR = "year";
    public static final String INDEX_KEY_ARTIST = "artist";
    public static final String INDEX_KEY_GENRE = "genre";

    @Override
    public IndexDescription getIndexDescription() {


        Field primaryKey = Field.getSearchableField(INDEX_KEY_PRIMARY_KEY);
        Field songTitle = Field.getSearchableField(INDEX_KEY_SONG_TITLE, 5);
        //Field songYear = Field.getRefiningField(INDEX_KEY_SONG_YEAR, Type.INTEGER);
        //Field artist = Field.getSearchableField(INDEX_KEY_ARTIST, 4);
        Field genre = Field.getSearchableField(INDEX_KEY_GENRE, 3);
        return new IndexDescription(INDEX_NAME, primaryKey, songTitle, genre);//songYear, artist, genre);
    }

    public void insertRecords() {
        JSONArray recordsJsonArray = new JSONArray();
        try {
            JSONObject sampleRecord1 = new JSONObject();
            sampleRecord1.put(INDEX_KEY_PRIMARY_KEY, "one");
            sampleRecord1.put(INDEX_KEY_SONG_TITLE, "RECORD_ONE_TITLE");
            //sampleRecord1.put(INDEX_KEY_SONG_YEAR, 1967);
            sampleRecord1.put(INDEX_KEY_ARTIST, "RECORD_ONE_ARTIST");
            sampleRecord1.put(INDEX_KEY_GENRE, "RECORD_ONE_GENRE");
            recordsJsonArray.put(sampleRecord1);

            /*
            JSONObject sampleRecord2 = new JSONObject();
            sampleRecord2.put(INDEX_KEY_SONG_TITLE, "RECORD_TWO_TITLE_REUSE_PRIMARY_KEY");
            //sampleRecord2.put(INDEX_KEY_SONG_YEAR, 1967);
            sampleRecord2.put(INDEX_KEY_ARTIST, "RECORD_TWO_ARTIST_REUSE_PRIMARY_KEY");
            sampleRecord2.put(INDEX_KEY_GENRE, "RECORD_TWO_GENRE_REUSE_PRIMARY_KEY");
            recordsJsonArray.put(sampleRecord2);
*/
            /*

            JSONObject sampleRecord3 = new JSONObject();
            sampleRecord3.put(INDEX_KEY_SONG_TITLE, "RECORD_THREE_TITLE_MISSING_PRIMARY_KEY");
            //sampleRecord2.put(INDEX_KEY_SONG_YEAR, 1967);
            sampleRecord3.put(INDEX_KEY_ARTIST, "RECORD_THREE_ARTIST_MISSING_PRIMARY_KEY");
            sampleRecord3.put(INDEX_KEY_GENRE, "RECORD_THREE_GENRE_MISSING_PRIMARY_KEY");
            recordsJsonArray.put(sampleRecord2);

            JSONObject sampleRecord4 = new JSONObject();
            sampleRecord2.put(INDEX_KEY_PRIMARY_KEY, 10);
            sampleRecord4.put(INDEX_KEY_SONG_TITLE, "RECORD_FOUR_TITLE_INVALID_TYPE_PRIMARY_KEY");
            //sampleRecord2.put(INDEX_KEY_SONG_YEAR, 1967);
            sampleRecord4.put(INDEX_KEY_ARTIST, "RECORD_FOUR_ARTIST_INVALID_TYPE_PRIMARY_KEY");
            sampleRecord4.put(INDEX_KEY_GENRE, "RECORD_FOUR_GENRE_INVALID_TYPE_PRIMARY_KEY");
            recordsJsonArray.put(sampleRecord4);




            JSONObject sampleRecord1 = new JSONObject();
            sampleRecord1.put(INDEX_KEY_PRIMARY_KEY, "1");
            sampleRecord1.put(INDEX_KEY_SONG_TITLE, "It's a Wonderful World");
            //sampleRecord1.put(INDEX_KEY_SONG_YEAR, 1967);
            sampleRecord1.put(INDEX_KEY_ARTIST, "Louis Armstrong");
            sampleRecord1.put(INDEX_KEY_GENRE, "Jazz");
            recordsJsonArray.put(sampleRecord1);


            JSONObject sampleRecord2 = new JSONObject();
            sampleRecord2.put(INDEX_KEY_PRIMARY_KEY, "2");
            sampleRecord2.put(INDEX_KEY_SONG_TITLE, "All You Need Is Love");
            //sampleRecord2.put(INDEX_KEY_SONG_YEAR, 1967);
            sampleRecord2.put(INDEX_KEY_ARTIST, "Beatles");
            sampleRecord2.put(INDEX_KEY_GENRE, "Rock'n'Roll");
            recordsJsonArray.put(sampleRecord2);

            JSONObject sampleRecord3 = new JSONObject();
            sampleRecord3.put(INDEX_KEY_PRIMARY_KEY, "3");
            sampleRecord3.put(INDEX_KEY_SONG_TITLE, "(I Can't Get No) Satisfaction");
            //sampleRecord3.put(INDEX_KEY_SONG_YEAR, 1965);
            sampleRecord3.put(INDEX_KEY_ARTIST, "The Rolling Stones");
            sampleRecord3.put(INDEX_KEY_GENRE, "Rock'n'Roll");
            recordsJsonArray.put(sampleRecord3);

            JSONObject sampleRecord4 = new JSONObject();
            sampleRecord4.put(INDEX_KEY_PRIMARY_KEY, "4");
            sampleRecord4.put(INDEX_KEY_SONG_TITLE, "Imagine");
            //sampleRecord4.put(INDEX_KEY_SONG_YEAR, 1971);
            sampleRecord4.put(INDEX_KEY_ARTIST, "John Lennon");
            sampleRecord4.put(INDEX_KEY_GENRE, "Rock");
            recordsJsonArray.put(sampleRecord4);

            JSONObject sampleRecord5 = new JSONObject();
            sampleRecord5.put(INDEX_KEY_PRIMARY_KEY, "5");
            sampleRecord5.put(INDEX_KEY_SONG_TITLE, "What's Going On");
            //sampleRecord5.put(INDEX_KEY_SONG_YEAR, 1971);
            sampleRecord5.put(INDEX_KEY_ARTIST, "Marvin Gaye");
            sampleRecord5.put(INDEX_KEY_GENRE, "Soul Rhythm And Blues");
            recordsJsonArray.put(sampleRecord5);

            JSONObject sampleRecord6 = new JSONObject();
            sampleRecord6.put(INDEX_KEY_PRIMARY_KEY, "6");
            sampleRecord6.put(INDEX_KEY_SONG_TITLE, "Respect");
            //sampleRecord6.put(INDEX_KEY_SONG_YEAR, 1967);
            sampleRecord6.put(INDEX_KEY_ARTIST, "Aretha Franklin");
            sampleRecord6.put(INDEX_KEY_GENRE, "Soul Jazz");
            recordsJsonArray.put(sampleRecord6);

            JSONObject sampleRecord7 = new JSONObject();
            sampleRecord7.put(INDEX_KEY_PRIMARY_KEY, "7");
            sampleRecord7.put(INDEX_KEY_SONG_TITLE, "Johnny B. Goode.");
            //sampleRecord7.put(INDEX_KEY_SONG_YEAR, 1958);
            sampleRecord7.put(INDEX_KEY_ARTIST, "Chuck Berry");
            sampleRecord7.put(INDEX_KEY_GENRE, "Rock'n'Roll");
            recordsJsonArray.put(sampleRecord7);

            JSONObject sampleRecord8 = new JSONObject();
            sampleRecord8.put(INDEX_KEY_PRIMARY_KEY, "8");
            sampleRecord8.put(INDEX_KEY_SONG_TITLE, "All Along the Watchtower");
            //sampleRecord8.put(INDEX_KEY_SONG_YEAR, 1968);
            sampleRecord8.put(INDEX_KEY_ARTIST, "Jimi Hendrix Experience");
            sampleRecord8.put(INDEX_KEY_GENRE, "Blues Rock");
            recordsJsonArray.put(sampleRecord8);*/

        } catch (JSONException oops) {
        }
        super.insert(recordsJsonArray);
    }

}
