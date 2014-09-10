package com.srch2.android.sdk;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabaseLockedException;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.Build;

import java.util.ArrayList;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;

/**
 * Represents an index in the SRCH2 search server that will be backed by an SQLite database table.
 * For every SQLite table to be searched on, users of the
 * SRCH2 Android SDK should implement a separate subclass instance of this class. This class only contains
 * methods for searching the data in the SQLite table as the SRCH2 search server will automatically observe
 * data content in the database's table. Note that it is
 * always possible to retrieve a reference to a specific <code>SQLiteIndexable</code> from the static method
 * {@link com.srch2.android.sdk.SRCH2Engine#getSQLiteIndex(String)}  where <code>
 * indexName</code> matches the return value of {@link #getIndexName()}.
 * <br><br>
 * <b>For each implementation of this class, it is necessary</b> to override the four methods:
 * {@link #getIndexName()} and {@link #getSchema()} which determine the basic configuration for the index
 * as it resides in the SRCH2 search server; additionally, {@link #getDatabaseName()} and {@link #getTableName()}
 * must be overridden returning the values as they are used in the {@link android.database.sqlite.SQLiteOpenHelper}
 * instance to create the database and table.
 * <br><br>
 * In addition, each implementation can optionally chose to override the methods {@link #getTopK()}
 * (which sets the number of search results returned per search) and {@link #getFuzzinessSimilarityThreshold()}
 * (which determines the number of wildcard substitutions that can occur per search input string). If
 * not overridden, these will take the default values {@link #DEFAULT_NUMBER_OF_SEARCH_RESULTS_TO_RETURN_AKA_TOPK} and
 * {@link #DEFAULT_FUZZINESS_SIMILARITY_THRESHOLD} respectively.
 * <br><br>
 * This class contains one state callback {@link #onIndexReady()} which will be triggered when the index the
 * <code>SQLiteIndexable</code> represents comes online and is available for searching the SRCH2 search server.
 * <br><br>
 * There is also one method that returns the number of records in the index: {@link #getRecordCount()}. The
 * value this method will return will be updated each time the SRCH2 search server comes online and each time an
 * insert, upsert or delete occurs in the original SQLite database table.
 * Note it can return {@link #INDEX_RECORD_COUNT_NOT_SET} if the SRCH2 search server
 * is not online such as when {@link com.srch2.android.sdk.SRCH2Engine#initialize()}
 * has been called but {@link com.srch2.android.sdk.SRCH2Engine#onStart(android.content.Context)} has not yet been
 * called).
 */
public abstract class SQLiteIndexable extends IndexableCore {

    final Schema getSchema() {

        if (getSQLiteOpenHelper() == null) {
            throw new NullPointerException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                    "the return value of getSQLiteOpenHelper() was null. Please provide the instance of the " +
                    "SQLiteOpenHelper used to manage the database.");
        }

        if (getDatabaseName() == null) {
            throw new NullPointerException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                    "database name was not found. Please verify the return value of " +
                    "getDatabaseName() matches that which was passed into the SqliteHelper implementation's constructor.");
        } else if (getDatabaseName().length() < 1) {
            throw new IllegalArgumentException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                    "database name was found to be invalid. Please verify the return value of " +
                    "getTableName() matches that which was passed into the SqliteHelper implementation's constructor.");
        }

        if (getTableName() == null) {
            throw new NullPointerException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                    "table name was not found. Please verify the return value of " +
                    "getTableName() matches that which was passed into the CREATE table string.");
        } else if (getTableName().length() < 1) {
            throw new IllegalArgumentException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                    "table name was found to be invalid. Please verify the return value of " +
                    "getTableName() matches that which was passed into the CREATE table string.");
        }


        String longitudeName = getLongitudeColumnName();
        String latitudeName = getLatitudeColumnName();

        boolean hasLongitude = false, hasLatitude = false;

        if (longitudeName != null) {
            if (longitudeName.length() < 1) {
                throw new IllegalArgumentException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                        "getLongitudeColumnName() was found to return a non-null but invalid value. Please verify the value" +
                        " returned by getLongitudeColumnName() is greater than zero and matches the column in the SQLite" +
                        "table " + getTableName() + " that contains the longitude data.");
            } else {
                hasLongitude = true;
            }
        }

        if (latitudeName != null) {
            if (latitudeName.length() < 1) {
                throw new IllegalArgumentException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                        "getLatitudeColumnName() was found to return a non-null but invalid value. Please verify the value" +
                        " returned by getLatitudeColumnName() is greater than zero and matches the column in the SQLite" +
                        "table " + getTableName() + " that contains the latitude data.");
            } else {
                hasLatitude = true;
            }
        }

        boolean isProbablyGeoIndex = false;

        if (!hasLatitude && hasLongitude) {
            throw new IllegalArgumentException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                    "getLongitudeColumnName() returned a valid String value while getLatitudeColumnName() did not. Please" +
                    " verify getLatitudeColumnName() return value matches the column in the SQLite " +
                    "table " + getTableName() + " that contains the latitude data.");
        } else if (!hasLongitude && hasLatitude) {
            throw new IllegalArgumentException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                    "getLatitudeColumnName() returned a valid String value while getLongitudeColumnName() did not. Please" +
                    " verify getLongitudeColumnName() return value matches the column in the SQLite " +
                    "table " + getTableName() + " that contains the longitude data.");
        } else if (hasLatitude && hasLongitude) {
            if (getLongitudeColumnName() == null) {
                throw new NullPointerException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                        "getLongitudeColumnName() returned a null String value while getLatitudeColumnName() did not. Please" +
                        " verify getLatitudeColumnName() return value matches the column in the SQLite " +
                        "table " + getTableName() + " that contains the latitude data.");
            } else if (getLatitudeColumnName() == null) {
                throw new NullPointerException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                        "getLatitudeColumnName() returned a null String value while getLongitudeColumnName() did not. Please" +
                        " verify getLongitudeColumnName() return value matches the column in the SQLite " +
                        "table " + getTableName() + " that contains the longitude data.");
            } else {
                isProbablyGeoIndex = true;
            }
        }

        boolean success = false;
        Schema s = null;
        while (!success) {
            boolean wasLocked = false;
            try {
                s = resolveSchemaFromSqliteOpenHelper(getTableName(), getSQLiteOpenHelper(), isProbablyGeoIndex);
            } catch (SQLiteDatabaseLockedException locked) {
                wasLocked = true;
            }
            if (s != null && !wasLocked) {
                success = true;
            }
        }
        return s;
    }


    /**
     * Implementing this method enables the SRCH2 search server to automatically observe data content
     * of the table that will be used to create and update the index this <code>SQLiteIndexable</code> represents.
     * <br><br>
     * The instance of the <code>SQLiteOpenHelper</code> is momentarily used to read the database table
     * information when {@link SRCH2Engine#initialize()} is called; a reference is not kept: a cursor is obtained
     * to read the database information, then both the readable database and cursor are closed within milliseconds.
     * @return the SQLiteOpenHelper used to manage the database
     */
    public abstract SQLiteOpenHelper getSQLiteOpenHelper();

    /**
     * Implementing this method enables the SRCH2 search server to automatically observe data content
     * of the table that will be used to create and update the index this <code>SQLiteIndexable</code> represents.
     * <br><br>
     * <b>It should exactly match</b> the value used in the create table string.
     * @return the table name <b>exactly</b> as it was specified in the create table string
     */
    public abstract String getTableName();

    /**
     * Implementing this method enables the SRCH2 search server to automatically observe data content
     * of the table that will be used to create and update the index this <code>SQLiteIndexable</code> represents.
     * <br><br>
     * <b>It should exactly match</b> the value used in the super constructor call of the
     * {@link android.database.sqlite.SQLiteOpenHelper} used to manage the database.
     * @return the database name <b>exactly</b> as it was specified in the constructor of the <code>SQLiteOpenHelper</code>
     */
    public abstract String getDatabaseName();

    /**
     * Implementing this method enables the SRCH2 search server to automatically observe data content
     * of the table that will be used to create and update the index this <code>SQLiteIndexable</code> represents.
     * <br><br>
     * This method (and {@link #getLongitudeColumnName()}) must be overridden if the SQLite table contains geodata
     * that is to be searched on. <b>It should exactly match</b> the value used in the CREATE TABLE string matching
     * the name of the column that contains the latitude data.
     * <br><br>
     * Returning null is equivalent not overriding this method.
     * @return the name of the column in the SQLite table that contains the latitude data
     */
    public String getLatitudeColumnName() { return null; }


    /**
     * Implementing this method enables the SRCH2 search server to automatically observe data content
     * of the table that will be used to create and update the index this <code>SQLiteIndexable</code> represents.
     * <br><br>
     * This method (and {@link #getLatitudeColumnName()}) must be overridden if the SQLite table contains geodata
     * that is to be searched on. <b>It should exactly match</b> the value used in the CREATE TABLE string matching
     * the name of the column that contains the longitude data.
     * <br><br>
     * Returning null is equivalent not overriding this method.
     * @return the name of the column in the SQLite table that contains the longitude data
     */
    public String getLongitudeColumnName() { return null; }


    final private Schema resolveSchemaFromSqliteOpenHelper(String tableName, SQLiteOpenHelper mSqliteOpenHelper, boolean isProbablyGeo) {

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            String databaseNameCheck = mSqliteOpenHelper.getDatabaseName();
            if (!getDatabaseName().equals(databaseNameCheck)) {
                throw new IllegalStateException("While generating com.srch2.android.sdk.Schema from SQLite table, " +
                        "database name was not found or found to be incorrect. Please verify the return value of " +
                        "getDatabaseName() matches that which was passed into the SQLiteOpenHelper super constructor.");
            }
        }

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
        boolean containsLongitude = false;
        boolean containsLatitude = false;

        try {
            db = mSqliteOpenHelper.getReadableDatabase();
            c = db.rawQuery("PRAGMA table_info(" + tableName + ")", null);
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
                        Cat.d("SQLITEIndexable", "name of pragmacolumn is " + name);

                        // check if it is it the primary key
                        boolean primaryKey = false;
                        if (pkField == null) {
                            primaryKey = c.getInt(PRAGMA_COLUMN_INDEX_COLUMN_IS_PRIMARY_KEY) == 1;
                            if (primaryKey) {
                                containsPrimaryKey = true;
                            }
                        }

                        if (name != null) {
                            if (primaryKey) {
                                if (columnType.schemaType == SQLiteColumnType.SchemaType.Searchable) {
                                    pkField = Field.createSearchablePrimaryKeyField(name);
                                } else {
                                    pkField = Field.createDefaultPrimaryKeyField(name);
                                }
                            } else {
                                Field extraField = null;
                                switch (columnType.schemaType) {
                                    case Searchable:
                                        extraField = Field.createSearchableField(name);
                                        if (!containsAtLeastOneSearchableField) {
                                            containsAtLeastOneSearchableField = true;
                                        }
                                        break;
                                    case RefiningInteger:
                                        extraField = Field.createRefiningField(name, Field.Type.INTEGER);
                                        break;
                                    case RefiningReal:
                                        if (isProbablyGeo) {
                                            if (!containsLatitude) {
                                                containsLatitude = name.equals(getLatitudeColumnName());
                                            }
                                            if (!containsLongitude) {
                                                containsLongitude = name.equals(getLongitudeColumnName());
                                            }
                                        } else {
                                            extraField = Field.createRefiningField(name, Field.Type.FLOAT);
                                        }
                                        break;
                                }
                                if (extraField != null) {
                                    fields.add(extraField);
                                }
                            }
                        }
                    }
                } while (c.moveToNext());
            } else {
                // This check throws if the sdk is not above ICE_CREAM_SANDWICH since the table won't exist if
                // the database name is incorrect
                throw new IllegalStateException("While generating com.srch2.android.sdk.Schema from SQLite table, " +
                        "table was not found. Please verify the return value of getTableName() matches the name of the table" +
                        "as it was entered in the CREATE TABLE string.");
            }
        } finally {
            if (db != null) {
                db.close();
            }
            if (c != null) {
                c.close();
            }
        }

        boolean isGeoIndex = false;
        if (!containsPrimaryKey) {
            throw new IllegalStateException("While generating com.srch2.android.sdk.Schema from SQLite table " +
                    tableName + ", " +
                    "table did not contain primary key. Table must contain one column that is PRIMARY KEY; please " +
                    "verify CREATE TABLE string contains PRIMARY KEY.");
        }
        if (!containsAtLeastOneSearchableField) {
            throw new IllegalStateException("While generating com.srch2.android.sdk.Schema from SQLite table " +
                    tableName + ", " +
                    "table did not contain at least one searchable field. Table must contain at least one column" +
                    " that is TEXT; please verify CREATE TABLE string contains at least one column of type TEXT.");
        }
        if (isProbablyGeo && containsLatitude && !containsLongitude) {
            throw new IllegalStateException("While generating com.srch2.android.sdk.Schema from SQLite table " +
                    tableName + ", " +
                    "table contained column containing latitude data but did not contain column containing " +
                    "longitude data. Please verify getLongitudeColumnName() matches the name of the column" +
                    "in the CREATE TABLE string that contains the longitude data and is of type REAL.");
        } else if (isProbablyGeo && !containsLatitude && containsLongitude)  {
            throw new IllegalStateException("While generating com.srch2.android.sdk.Schema from SQLite table " +
                    tableName + ", " +
                    "table contained column containing longitude data but did not contain column containing " +
                    "latitude data. Please verify getLatitudeColumnName() matches the name of the column" +
                    "in the CREATE TABLE string that contains the latitude data and is of type REAL.");
        } else if (isProbablyGeo && !containsLatitude && !containsLongitude) {
            throw new IllegalStateException("While generating com.srch2.android.sdk.Schema from SQLite table " +
                    tableName + ", " +
                    "table did not contain columns corresponding to latitude or longitude data. Please verify" +
                    " both getLatitudeColumnName() and getLongitudeColumnName() return values matching the " +
                    "column names in the CREATE TABLE string that correspond to the columns containing the" +
                    "latitude and longitude data and are of type REAL. Otherwise do not override these" +
                    "methods--are you sure this table is supposed to represent geodata?");
        } else if (isProbablyGeo && containsLatitude && containsLongitude) {
            isGeoIndex = true;
        }

        if (isGeoIndex) {
            Cat.d("SQLITEIndexable", "returning geo index");
            return new Schema(pkField, getLatitudeColumnName(), getLongitudeColumnName(), fields.toArray(new Field[fields.size()]));
        } else {
            Cat.d("SQLITEIndexable", "returning default index");
            return new Schema(pkField, fields.toArray(new Field[fields.size()]));
        }
    }







    private static enum SQLiteColumnType {
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

        SchemaType schemaType;

        SQLiteColumnType(SchemaType srch2FieldType) {
            schemaType = srch2FieldType;
        }

        static SQLiteColumnType getType(int type) {
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

        static SQLiteColumnType getType(String typeName) {
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

    /**
     * Returns the number of records that are currently in the index that this
     * <code>SQLiteIndexable</code> represents.
     * <br><br>
     * <b>Note that this is a blocking call</b>: however it typically will block less than 50ms.
     * @return the number of records in the index
     */
    public final int getRecordCount() {
        GetRecordCountTask t = new GetRecordCountTask(this);
        int count = INDEX_RECORD_COUNT_NOT_SET;
        try {
            count = HttpTask.doSQLiteBlockingGetRecordCountTask(t).get();
        } catch (InterruptedException e) {
        } catch (ExecutionException e) {
        }
        return count;
    }

    static class GetRecordCountTask implements Callable<Integer> {
        SQLiteIndexable requestingIndexable;

        public GetRecordCountTask(SQLiteIndexable idx) {
            requestingIndexable = idx;
        }

        @Override
        public Integer call() throws Exception {
            if (requestingIndexable != null) {
                InternalInfoTask iit = new InternalInfoTask(UrlBuilder
                        .getInfoUrl(
                                SRCH2Engine.conf, requestingIndexable.indexInternal.indexDescription), 400, false);
                InternalInfoResponse iir = iit.getInfo();
                if (iir.isValidInfoResponse) {
                    return iir.numberOfDocumentsInTheIndex;
                } else {
                    return INDEX_RECORD_COUNT_NOT_SET;
                }
            }
            return INDEX_RECORD_COUNT_NOT_SET;
        }
    }




}
