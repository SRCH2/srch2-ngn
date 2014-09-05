package com.srch2.android.sdk.sandbox;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;
import com.srch2.android.sdk.Field;
import com.srch2.android.sdk.PrimaryKeyField;
import com.srch2.android.sdk.SQLiteIndexable;

public class DatabaseHelper extends SQLiteOpenHelper {

    public static final String TAG = "s2sb:: Sandbox-Database";

    public static class Schema {
        public static final String DATABASE_NAME = "demo";
        public static final int DATABASE_VERSION = 1; // can leave unchanging since this is just a demo
        public static final String TABLE_NAME = "company";
        public static final String COLUMN_PRIMARY_KEY = "_id";
        public static final String COLUMN_NAME = "name";
        public static final String COLUMN_AGE = "age";
        public static final String COLUMN_ADDRESS = "address";
        public static final String COLUMN_SALARY = "salary";
    }

    public static final String getCreateTableString() {
        return "CREATE TABLE " + Schema.TABLE_NAME + "(" +
                Schema.COLUMN_PRIMARY_KEY + " INTEGER PRIMARY KEY NOT NULL, " +
                Schema.COLUMN_NAME + " TEXT NOT NULL, " +
                Schema.COLUMN_AGE + " INTEGER NOT NULL, " +
                Schema.COLUMN_ADDRESS + " TEXT, " +
                Schema.COLUMN_SALARY + " REAL" +
                " )";
    }

    public DatabaseHelper(Context context) {
        super(context, Schema.DATABASE_NAME, null, Schema.DATABASE_VERSION); // assuming version will never change,
        SQLiteDatabase db = getReadableDatabase();
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
        db.execSQL("DROP TABLE IF EXISTS " + Schema.TABLE_NAME);
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
        SQLiteDatabase db = getWritableDatabase();
        try {
            for (int i = 0; i < 100; i++) {
                ContentValues cv = new ContentValues();
                cv.put(Schema.COLUMN_PRIMARY_KEY, i);
                cv.put(Schema.COLUMN_AGE, i);
                cv.put(Schema.COLUMN_NAME, "name " + RandomStringUtil.getRandomString(10));
                cv.put(Schema.COLUMN_ADDRESS, "address " + RandomStringUtil.getRandomString(20));
                cv.put(Schema.COLUMN_SALARY, (double) i);
                db.insert(Schema.TABLE_NAME, null, cv);
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

        Cursor c = db.query(Schema.TABLE_NAME, null, null, null, null, null, null);

        if (c != null) {
            Log.d(TAG, "cursor count " + c.getCount());
        } else {
            Log.d(TAG, "something wrong!");
        }

        db.close();

        Log.d(TAG, "insertRecords - finished");
    }

    public static class SqliteIndexable extends SQLiteIndexable {
        @Override
        public String getIndexName() {
            return Schema.TABLE_NAME;
        }

        @Override
        public com.srch2.android.sdk.Schema getSchema() {
            PrimaryKeyField pk = Field.createDefaultPrimaryKeyField(Schema.COLUMN_PRIMARY_KEY);
            Field name = Field.createSearchableField(Schema.COLUMN_NAME);
            Field age = Field.createRefiningField(Schema.COLUMN_AGE, Field.Type.INTEGER);
            Field salary = Field.createRefiningField(Schema.COLUMN_SALARY, Field.Type.FLOAT);
            Field address = Field.createSearchableField(Schema.COLUMN_ADDRESS);
            return com.srch2.android.sdk.Schema.createSchema(pk, name, age, salary, address);
        }

        @Override
        public String getTableName() {
            return Schema.TABLE_NAME;
        }

        @Override
        public String getDatabaseName() {
            return Schema.DATABASE_NAME;
        }
    }

}