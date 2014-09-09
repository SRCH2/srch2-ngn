package com.srch2.android.sdk;

import android.database.sqlite.SQLiteOpenHelper;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;

/**
 * Created by jianfeng on 7/23/14.
 */
public class PrepareEngine {

    static final String DEFAULT_SRCH2HOME_PATH = "/tmp/data/com.srch2.server" + File.separator;
    static final String OAUTH = "SRCH2-OAUTH";
    static int DEFAULT_SRCH2SERVER_PORT = 55555;
    static final MusicIndex musicIndex = new MusicIndex();
    static final MovieIndex movieIndex = new MovieIndex();
    static final GeoIndex geoIndex = new GeoIndex();
    static final DbIndex dbIndex = new DbIndex();
    static Indexable[] orderedIndexes = {musicIndex, movieIndex, geoIndex};
    static boolean isPrepared = false;

    static void prepareEngine() {
        ArrayList<IndexableCore> idxs = new ArrayList<IndexableCore>();
        idxs.add(musicIndex);
        idxs.add(movieIndex);
        idxs.add(geoIndex);
        idxs.add(dbIndex);
        SRCH2Engine.conf = new SRCH2Configuration(idxs);
        SRCH2Engine.conf.setSRCH2Home(DEFAULT_SRCH2HOME_PATH);
        DEFAULT_SRCH2SERVER_PORT = SRCH2Engine.detectFreePort();
        SRCH2Engine.conf.setPort(DEFAULT_SRCH2SERVER_PORT);
        SRCH2Engine.setAuthorizationKey(OAUTH);
    }

    static class DbIndex extends SQLiteIndexable {

        static final String TABLE_NAME = "table";
        static final String DATABASE_NAME = "db";
        static final String INDEX_NAME = "table";
        static final String INDEX_PK_NAME = "id";
        static final String INDEX_TITLE_NAME = "title";

        @Override
        public String getTableName() {
            return TABLE_NAME;
        }

        @Override
        public String getDatabaseName() {
            return DATABASE_NAME;
        }

        @Override
        public String getIndexName() {
            return INDEX_NAME;
        }

        @Override
        public SQLiteOpenHelper getSQLiteOpenHelper() {
            return null;
        }
    }


    static class MusicIndex extends Indexable {
        public static final String INDEX_NAME = "music";
        public static final String INDEX_KEY_PRIMARY_KEY = "id";
        public static final String INDEX_KEY_SONG_TITLE = "song";
        public static final String INDEX_KEY_SONG_YEAR = "year";
        public static final String INDEX_KEY_ARTIST = "artist";
        public static final String INDEX_KEY_GENRE = "genre";



        @Override
        public String getIndexName() {
            return INDEX_NAME;
        }

        @Override
        public Schema getSchema() {



            PrimaryKeyField primaryKey = Field.createDefaultPrimaryKeyField(INDEX_KEY_PRIMARY_KEY);
            Field songTitle = Field.createSearchableField(INDEX_KEY_SONG_TITLE, 5);
            //Field songYear = Field.createRefiningField(INDEX_KEY_SONG_YEAR, Type.INTEGER);
            //Field artist = Field.createSearchableField(INDEX_KEY_ARTIST, 4);
            Field genre = Field.createSearchableField(INDEX_KEY_GENRE, 3);


            Schema s = new Schema(primaryKey, songTitle, genre);


            return s;
        }


        public void insertRecords() {
            JSONArray recordsJsonArray = new JSONArray();
            try {
                JSONObject recordOne = new JSONObject();
                recordOne.put(INDEX_KEY_PRIMARY_KEY, "1");
                recordOne.put(INDEX_KEY_SONG_TITLE, "It's a Wonderful World");
                //recordOne.put(INDEX_KEY_SONG_YEAR, 1967);
                //recordOne.put(INDEX_KEY_ARTIST, "Louis Armstrong");
                recordOne.put(INDEX_KEY_GENRE, "Jazz");
                recordsJsonArray.put(recordOne);

                JSONObject recordTwo = new JSONObject();
                recordTwo.put(INDEX_KEY_PRIMARY_KEY, "2");
                recordTwo.put(INDEX_KEY_SONG_TITLE, "All You Need Is Love");
                //recordOne.put(INDEX_KEY_SONG_YEAR, 1967);
                //recordOne.put(INDEX_KEY_ARTIST, "Beatles");
                recordTwo.put(INDEX_KEY_GENRE, "Rock'n'Roll");
                recordsJsonArray.put(recordTwo);
            } catch (JSONException oops) {
            }
            super.insert(recordsJsonArray);
        }




    }

    static class MovieIndex extends Indexable {
        public static final String INDEX_NAME = "movies";

        public static final String INDEX_KEY_PRIMARY_KEY = "id";
        public static final String INDEX_KEY_TITLE = "title";
        public static final String INDEX_KEY_YEAR = "year";
        public static final String INDEX_KEY_GENRE = "genre";



        @Override
        public String getIndexName() {
            return INDEX_NAME;
        }

        @Override
        public Schema getSchema() {
            PrimaryKeyField primaryKey = Field.createDefaultPrimaryKeyField(INDEX_KEY_PRIMARY_KEY);
            Field title = Field.createSearchableField(INDEX_KEY_TITLE);
            //Field year = Field.createRefiningField(INDEX_KEY_YEAR, Type.INTEGER);
            Field genre = Field.createSearchableField(INDEX_KEY_GENRE);

            return new Schema(primaryKey, title, genre);

        }

        public void insertRecords() {
            JSONArray recordsJsonArray = new JSONArray();
            try {
                JSONObject recordOne = new JSONObject();
                recordOne.put(INDEX_KEY_PRIMARY_KEY, "1");
                recordOne.put(INDEX_KEY_TITLE, "Mission Impossible");
                //recordOne.put(INDEX_KEY_YEAR, 2002);
                recordOne.put(INDEX_KEY_GENRE, "Action");
                recordsJsonArray.put(recordOne);

                JSONObject recordTwo = new JSONObject();
                recordTwo.put(INDEX_KEY_PRIMARY_KEY, "2");
                recordTwo.put(INDEX_KEY_TITLE, "Independence Day");
                //recordOne.put(INDEX_KEY_YEAR, 1999);
                recordTwo.put(INDEX_KEY_GENRE, "Science Fiction");
                recordsJsonArray.put(recordTwo);
            } catch (JSONException oops) {
            }
            super.insert(recordsJsonArray);
        }
    }

    static class GeoIndex extends Indexable {
        static final String NAME = "geo";
        static final String INDEX_KEY_PRIMARY_KEY = "title";
        static final String LAT = "lat";
        static final String LON = "long";

        static final PrimaryKeyField fieldID = Field.createSearchablePrimaryKeyField(INDEX_KEY_PRIMARY_KEY);



        @Override
        public String getIndexName() {
            return NAME;
        }

        @Override
        public Schema getSchema() {
            return new Schema(fieldID, LAT, LON);
        }

        public void insertRecord() {
            JSONArray recordsJsonArray = new JSONArray();
            try {
                JSONObject recordOne = new JSONObject();
                recordOne.put(INDEX_KEY_PRIMARY_KEY, "Irvine");
                recordOne.put(LAT, 33);
                recordOne.put(LON, -117);
                recordsJsonArray.put(recordOne);

                JSONObject recordTwo = new JSONObject();
                recordTwo.put(INDEX_KEY_PRIMARY_KEY, "Irvine");
                recordTwo.put(LAT, 34);
                recordTwo.put(LON, -118);

                recordsJsonArray.put(recordTwo);
            } catch (JSONException oops) {
            }
            super.insert(recordsJsonArray);
        }
    }
}
