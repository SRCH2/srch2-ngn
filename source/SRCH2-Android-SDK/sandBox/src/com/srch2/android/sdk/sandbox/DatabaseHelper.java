package com.srch2.android.sdk.sandbox;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.SystemClock;
import android.util.Log;
import com.srch2.android.sdk.Field;
import com.srch2.android.sdk.PrimaryKeyField;
import com.srch2.android.sdk.SQLiteIndexable;

import java.util.ArrayList;

public class DatabaseHelper extends SQLiteOpenHelper {


    public SqliteIdx idx;
    public static final String TAG = "s2sb:: Sandbox-Database";

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

        idx = new SqliteIdx();

        db.close();



    }

    SQLiteOpenHelper getSelf() { return this; }

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


    public void doPragma() {

        SQLiteDatabase db = null;
        Cursor c = null;

        try {
            db = getReadableDatabase();
            c = db.rawQuery("PRAGMA table_info(" + SQLiteSchema.TABLE_NAME + ")", null);
            if (c.moveToFirst()) {
                int i = 0;

                Log.d("sqlite", "$$$$$$$$$$$$$$$$$$$$$$ PRINTING PRAGMA INFO $$$$$$$$$$$$$$$$$$$$$$END");

                do {

                    Log.d("sqlite", "i@" + i);
                    int columnCount = c.getColumnCount();
                    for (int j = 0; j < columnCount; ++j) {

                        Log.d("sqlite", "j@" + j + " name [" + c.getColumnName(j) + "] type [" +
                                SQLiteColumnType.getType(c.getType(j)).name() + "] val [" + c.getString(j) + "]");

                    }
                    ++i;
                    Log.d("sqlite", "---------------------------------------------------------");




                } while (c.moveToNext());
                Log.d("sqlite", "$$$$$$$$$$$$$$$$$$$$$$ END OF PRINTING PRAGMA INFO $$$$$$$$$$$$$$$$$$$$$$");

            }

        } finally {
            if (db != null) db.close();
            if (c != null) c.close();
        }



    }


    public void resolveSchemaFromPragma(SQLiteOpenHelper mSqliteOpenHelper) {

        long t = SystemClock.uptimeMillis();

        final int PRAGMA_COLUMN_INDEX_COLUMN_INDEX = 0;
        final int PRAGMA_COLUMN_INDEX_COLUMN_NAME = 1;
        final int PRAGMA_COLUMN_INDEX_COLUMN_TYPE = 2;
        final int PRAGMA_COLUMN_INDEX_COLUMN_IS_NOT_NULL = 3;
        final int PRAGMA_COLUMN_INDEX_COLUMN_DEFAULT_VALUE = 4;
        final int PRAGMA_COLUMN_INDEX_COLUMN_IS_PRIMARY_KEY = 5;

        SQLiteDatabase db = null;
        Cursor c = null;

        ArrayList<Field> fields = new ArrayList<Field>();
        PrimaryKeyField pkField = null;
        boolean containsPrimaryKey = false;
        boolean containsAtLeastOneSearchableField = false;

        try {
            db = getReadableDatabase();
            c = db.rawQuery("PRAGMA table_info(" + SQLiteSchema.TABLE_NAME + ")", null);
            if (c.moveToFirst()) {
                // each row represents a column in the table we do the pragma info on
                // each column in this row represents data about that table column
                // (see PRAMGA_COLUMN_INDEX_* above)
                do {
                    String name = null;

                    // first get the type of the this column: skip if blob (media) or null
                    // note we are comparing the value for the table column's type NOT getting the
                    // type of the column in the cursor
                    SQLiteColumnType columnType = SQLiteColumnType.getType(c.getString(PRAGMA_COLUMN_INDEX_COLUMN_TYPE));
                    if (columnType != SQLiteColumnType.BLOB && columnType != SQLiteColumnType.NULL) {

                        // get the name of the column
                        name = c.getString(PRAGMA_COLUMN_INDEX_COLUMN_NAME);


                        // check if it is it the primary key
                        boolean primaryKey = false;
                        if (pkField == null) {
                            primaryKey = c.getInt(PRAGMA_COLUMN_INDEX_COLUMN_IS_PRIMARY_KEY) == 1;
                            containsPrimaryKey = primaryKey;
                        }

                        if (name != null) {
                            if (primaryKey) {
                                if (columnType.schemaType == SQLiteColumnType.SchemaType.Searchable) {
                                    pkField = Field.createSearchablePrimaryKeyField(name);
                                }
                            } else {
                                Field extraField = null;
                                switch (columnType.schemaType) {
                                    case Searchable:
                                        extraField = Field.createSearchableField(name);
                                        break;
                                    case RefiningInteger:
                                        extraField = Field.createRefiningField(name, Field.Type.INTEGER);
                                        break;
                                    case RefiningReal:
                                        extraField = Field.createRefiningField(name, Field.Type.FLOAT);
                                        break;
                                }
                                fields.add(extraField);
                            }
                        }
                    }
                } while (c.moveToNext());
            }
        } finally {
            if (db != null) {
                db.close();
            }
            if (c != null) {
                c.close();
            }
        }

        if (!containsPrimaryKey) {
            throw new IllegalStateException("While generating com.srch2.android.sdk.Schema from SQLite table, " +
                    "table did not contain primary key. Table must contain one column that is PRIMARY KEY");
        }
        if (!containsAtLeastOneSearchableField) {
            throw new IllegalStateException("While generating com.srch2.android.sdk.Schema from SQLite table, " +
                    "table did not contain at least one searchable field. Table must contain at least one column" +
                    " that is TEXT.");
        }


        long e = SystemClock.uptimeMillis() - t;
        Log.d("sqlite", "generating schema took " + e);
    }

    static enum SQLiteColumnType {
        TEXT(SchemaType.Searchable),
        INTEGER(SchemaType.RefiningInteger),
        NULL(null),
        BLOB(null),
        REAL(SchemaType.RefiningReal);

        static enum SchemaType {
            Searchable,
            RefiningInteger,
            RefiningReal;
        }

        public SchemaType schemaType;

        SQLiteColumnType(SchemaType srch2FieldType) {
            schemaType = srch2FieldType;
        }

        public static SQLiteColumnType getType(int type) {
            switch (type) {
                case Cursor.FIELD_TYPE_STRING:
                    return SQLiteColumnType.TEXT;
                case Cursor.FIELD_TYPE_FLOAT:
                    return SQLiteColumnType.REAL;
                case Cursor.FIELD_TYPE_INTEGER:
                    return SQLiteColumnType.INTEGER;
                case Cursor.FIELD_TYPE_NULL:
                    return SQLiteColumnType.NULL;
                case Cursor.FIELD_TYPE_BLOB:
                    return SQLiteColumnType.BLOB;
                default:
                    return SQLiteColumnType.NULL;
            }
        }

        public static SQLiteColumnType getType(String typeName) {
            if (typeName.equals(SQLiteColumnType.TEXT.name())) {
                return SQLiteColumnType.TEXT;
            } else if (typeName.equals(SQLiteColumnType.INTEGER.name())) {
                return SQLiteColumnType.INTEGER;
            } else if (typeName.equals(SQLiteColumnType.REAL.name())) {
                return SQLiteColumnType.REAL;
            } else if (typeName.equals(SQLiteColumnType.NULL.name())) {
                return SQLiteColumnType.NULL;
            } else if (typeName.equals(SQLiteColumnType.BLOB.name())) {
                return SQLiteColumnType.BLOB;
            } else {
                return SQLiteColumnType.NULL;
            }
        }
    }
























    public class SqliteIdx extends SQLiteIndexable {

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
            return getSelf();
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
    }

}