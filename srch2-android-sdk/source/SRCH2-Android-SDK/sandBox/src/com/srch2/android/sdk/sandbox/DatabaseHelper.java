package com.srch2.android.sdk.sandbox;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;
import com.srch2.android.sdk.Indexable;
import com.srch2.android.sdk.SQLiteIndexable;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class DatabaseHelper extends SQLiteOpenHelper {


    public SqliteIdx idx;
    public static final String TAG = "s2sb:: Sandbox-Database";


    public SqliteIdx getIndexable() { return idx; }

    public static class SQLiteSchema {
        public static final String DATABASE_NAME = "demo";
        public static final int DATABASE_VERSION = 1; // can leave unchanging since this is just a demo
        public static final String TABLE_NAME = "company";
        public static final String COLUMN_PRIMARY_KEY = "_id";
        public static final String COLUMN_NAME = "name";
        public static final String COLUMN_AGE = "age";
        public static final String COLUMN_ADDRESS = "address";
        public static final String COLUMN_SALARY = "salary";
        public static final String COLUMN_LONGITUDE = "longitude";
        public static final String COLUMN_LATITUDE = "latitude";
    }

    public static final String getCreateTableString() {
        return "CREATE TABLE " + SQLiteSchema.TABLE_NAME + "(" +
                SQLiteSchema.COLUMN_PRIMARY_KEY + " INTEGER PRIMARY KEY NOT NULL, " +
                SQLiteSchema.COLUMN_NAME + " TEXT NOT NULL, " +
                SQLiteSchema.COLUMN_AGE + " INTEGER NOT NULL, " +
                SQLiteSchema.COLUMN_ADDRESS + " TEXT, " +
                SQLiteSchema.COLUMN_SALARY + " REAL, " +
                "Blobo BLOB, " +
                SQLiteSchema.COLUMN_LATITUDE + " REAL, " +
                SQLiteSchema.COLUMN_LONGITUDE + " REAL" +

                " )";
    }

    public DatabaseHelper(Context context) {
        super(context, SQLiteSchema.DATABASE_NAME, null, SQLiteSchema.DATABASE_VERSION); // assuming version will never change,
        SQLiteDatabase db = getReadableDatabase();

        idx = new SqliteIdx(this);

        db.close();



    }


    @Override
    public void onCreate(SQLiteDatabase db) {
        // this method is called anytime time an operation on the database this helper backs happens if the database does not already exist
        Log.d(TAG, "onCreate");
        db.execSQL(getCreateTableString());
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // if the value of DATABASE_VERSION passed in super(context, Schema.DATABASE_NAME, null, Schema.DATABASE_VERSION);
        // in constructor is changed, this method will be called: used by the developer to dynamically change the schema if underlying data set changes
        db.execSQL("DROP TABLE IF EXISTS " + SQLiteSchema.TABLE_NAME);
        onCreate(db);
    }

    // example usage of db backed by this sqlite open helper
    public void foo() {
        // Be sure to close the database after you open it
        SQLiteDatabase db = getWritableDatabase();
        try {
            // query, insert, etc etc
        } finally {
            if (db != null) {
                db.close();
            }
        }
    }

    public void insertRecords() {
        Log.d(TAG, "insertRecords");

        SQLiteDatabase db = getReadableDatabase();

        Cursor c = db.query(SQLiteSchema.TABLE_NAME, null, null, null, null, null, null);

        int count = 0;
        if (c != null) {
            count = c.getCount();
            c.close();
            Log.d(TAG, "cursor count " + c.getCount());
        } else {
            Log.d(TAG, "something wrong!");
        }

        db.close();

        if (count == 100) {
            return;
        }

        db = getWritableDatabase();
        try {
            ContentValues cv = new ContentValues();
            for (int i = 0; i < 100; i++) {
                cv.clear();
                cv.put(SQLiteSchema.COLUMN_PRIMARY_KEY, i);
                cv.put(SQLiteSchema.COLUMN_AGE, i);
                cv.put(SQLiteSchema.COLUMN_NAME, "name " + RandomStringUtil.getRandomString(10));
                cv.put(SQLiteSchema.COLUMN_ADDRESS, "address " + RandomStringUtil.getRandomString(20));
                cv.put(SQLiteSchema.COLUMN_SALARY, (double) i);
                cv.put(SQLiteSchema.COLUMN_LATITUDE, (double) (Math.random() * 50));
                cv.put(SQLiteSchema.COLUMN_LONGITUDE, (double) (Math.random() * 50));
                db.insert(SQLiteSchema.TABLE_NAME, null, cv);
            }
        } catch (Exception e) {
            Log.d(TAG, "OOPS threw exception " + e.getClass().getSimpleName() + " " + e.getMessage());
            e.printStackTrace();
        } finally {
            if (db != null) {
                db.close();
            }
        }

        db = getReadableDatabase();

        c = db.query(SQLiteSchema.TABLE_NAME, null, null, null, null, null, null);

        if (c != null) {
            Log.d(TAG, "cursor count " + c.getCount());
            c.close();
        } else {
            Log.d(TAG, "something wrong!");
        }

        db.close();

        Log.d(TAG, "insertRecords - finished");
    }























    static public ArrayList<SearchResultsAdapter.SearchResultItem> wrap(ArrayList<JSONObject> jsonResultsToWrap) {
        ArrayList<SearchResultsAdapter.SearchResultItem> newResults = new ArrayList<SearchResultsAdapter.SearchResultItem>();
        for (JSONObject jsonObject : jsonResultsToWrap) {
            Log.d("SEARCH RESULT OBJECT", jsonObject.toString());
            SearchResultsAdapter.SearchResultItem searchResult = null;
            try {

                JSONObject originalRecord = jsonObject.getJSONObject(Indexable.SEARCH_RESULT_JSON_KEY_RECORD);

                String title = originalRecord.getString(DatabaseHelper.SQLiteSchema.COLUMN_NAME);

                Log.d("Highlight", "title is " + title);

                if (title == null) {
                    title =  "null title";
                }

                searchResult = new SearchResultsAdapter.SearchResultItem(title, " ", " ");
            } catch (JSONException oops) {
                oops.printStackTrace();
                continue;
            }

            if (searchResult != null) {
                newResults.add(searchResult);
            }
        }
        return newResults;
    }


    public static class SqliteIdx extends SQLiteIndexable {

        SQLiteOpenHelper helper;

        public SqliteIdx(SQLiteOpenHelper h) {
            helper = h;
        }

        @Override
        public String getIndexName() {
            return SQLiteSchema.TABLE_NAME;
        }

        @Override
        public SQLiteOpenHelper getSQLiteOpenHelper() {
            return helper;
        }

        @Override
        public String getTableName() {
            return SQLiteSchema.TABLE_NAME;
        }

        @Override
        public String getDatabaseName() {
            return SQLiteSchema.DATABASE_NAME;
        }


        @Override
        public String getLatitudeColumnName() {
            return SQLiteSchema.COLUMN_LATITUDE;
        }

        @Override
        public String getLongitudeColumnName() {
            return SQLiteSchema.COLUMN_LONGITUDE;
        }


        @Override
        public String getRecordBoostColumnName() {
            return SQLiteSchema.COLUMN_SALARY;
        }

        @Override
        public void onIndexReady() {
            super.onIndexReady();
        }
    }

}