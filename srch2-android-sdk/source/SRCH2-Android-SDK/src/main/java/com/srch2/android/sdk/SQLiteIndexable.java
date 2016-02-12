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
 * always possible to retrieve a reference to a specific {@code SQLiteIndexable} from the static method
 * {@link com.srch2.android.sdk.SRCH2Engine#getSQLiteIndex(String)}  where {@code 
 * indexName} matches the return value of {@link #getIndexName()}.
 * <br><br>
 * Each {@code Indexable } instance should be registered prior to calling
 * {@link com.srch2.android.sdk.SRCH2Engine#onResume(android.content.Context)} by calling
 * {@link com.srch2.android.sdk.SRCH2Engine#setSQLiteIndexables(SQLiteIndexable, SQLiteIndexable...)} and passing in all
 * {@code SQLiteIndexable } instances.
 * <br><br>
 * For each implementation of this class, it is necessary to override the following methods:
 * <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getIndexName()} (index's handle) <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getDatabaseName()} (name of source database) <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getTableName()} (name of source database table) <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getSQLiteOpenHelper()} (instance used to manage the database)
 * <br><br>
 * Unlike a {@code Indexable} where a {@code Schema} must be created, the {@code Schema} is generated automatically:
 * for the specified table in the specified database, all columns of that table that are of type {@code TEXT} will
 * be resolved as searchable fields, and all columns of type {@code REAL} or {@code INTEGER} will be resolved as
 * refining fields. This table must contain one column that is created as the {@code PRIMARY KEY} when creating
 * the table.
 * <br><br>
 * To set the {@code RecordBoostField}, field boost values, and highlighted fields use the methods:
 * <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getRecordBoostColumnName()} (name of column with the record boost values) <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getColumnBoostValue(String)} (value of boost to assign to each {@code TEXT} type column) <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getColumnIsHighlighted(String)} (whether to highlight each {@code TEXT} type column)
 * <br><br>
 * The first of these methods should return the name of the column that should serve as the {@link com.srch2.android.sdk.RecordBoostField}
 * for the index this {@code Indexable} represents. The two second two methods will pass each the name of each column
 * of type {@code TEXT} and should return the appropriate values for that column.
 * <br><br>
 * In addition, each implementation can customize search results from the index this {@code SQLiteIndexable} represents
 * by optionally overriding the methods:
 * <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getTopK()} (number of search results) <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getFuzzinessSimilarityThreshold()} (number of typos per search input) <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;{@link #getHighlighter()} (formatting of search results)
 * <br><br>
 * This class contains one state callback {@link #onIndexReady()} which will be triggered when the index the
 * {@code SQLiteIndexable} represents comes online and is available for searching the SRCH2 search server.
 * <br><br>
 * There is also one method that returns the number of records in the index: {@link #getRecordCount()}. The
 * value this method will return will be updated each time the SRCH2 search server comes online and each time an
 * insert, upsert or delete occurs in the original SQLite database table.
 * Note it can return {@link #INDEX_RECORD_COUNT_NOT_SET} if the SRCH2 search server
 * is not online and has not loaded the indexes into memory yet.
 */
public abstract class SQLiteIndexable extends IndexableCore {

    /**
     * If returned from {@link #getRecordCount()} indicates this value has not yet been set.
     * <br><br>
     * Constant Value: {@value}
     */
    public static final int INDEX_RECORD_COUNT_NOT_SET = -1;

    /**
     * The JSON key to use to retrieve each original record from each {@code JSONObject } in
     * the {@code ArrayList<JSONObject>} of the callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}.
     * <br><br>
     * Constant Value: "{@value}"
     */
    public static final String SEARCH_RESULT_JSON_KEY_RECORD = "record";

    /**
     * The JSON key to use to retrieve each set of highlighted fields from each record from each {@code JSONObject} in
     * the {@code ArrayList<JSONObject>} of the callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}.
     * <br><br>
     * Constant Value: "{@value}"
     */
    public static final String SEARCH_RESULT_JSON_KEY_HIGHLIGHTED = "highlighted";

    /**
     * The default value for the numbers of search results to return per search request.
     * <br><br>
     * Constant Value: {@value}
     */
    public static final int DEFAULT_NUMBER_OF_SEARCH_RESULTS_TO_RETURN_AKA_TOPK = 10;

    /**
     * The default value for the fuzziness similarity threshold. Approximately one character
     * per every three characters will be allowed to act as a wildcard during a search.
     * <br><br>
     * Constant Value: {@value}
     */
    public static final float DEFAULT_FUZZINESS_SIMILARITY_THRESHOLD = 0.65f;


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

        boolean hasRecordBoostColumn = false;
        if (getRecordBoostColumnName() != null && getRecordBoostColumnName().length() < 1) {
            throw new IllegalArgumentException("While generating com.srch2.android.sdk.Schema from SQLite database, " +
                    "getRecordBoostColumnName() was found to return an invalid String value. Please verify the return " +
                    "value of getRecordBoostColumnName() matches a column of type REAL or INTEGER that is supposed to" +
                    " determine the relative ranking of each row in the table.");
        } else if (getRecordBoostColumnName() != null) {
            hasRecordBoostColumn = true;
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
            isProbablyGeoIndex = true;
        }

        boolean success = false;
        Schema schema = null;
        while (!success) {
            boolean wasLocked = false;
            try {
                schema = resolveSchemaFromSqliteOpenHelper(getTableName(), getSQLiteOpenHelper(),
                        isProbablyGeoIndex, hasRecordBoostColumn);
            } catch (SQLiteDatabaseLockedException locked) {
                wasLocked = true;
            }
            if (schema != null && !wasLocked) {
                success = true;
            }
        }
        return schema;
    }


    /**
     * Determines the column that functions as the {@link com.srch2.android.sdk.RecordBoostField} for the index
     * this {@code SQLiteIndexable} represents. This column should of type {@code INTEGER} or {@code REAL} and
     * should contain values greater than or equal to 1 and less than 100.
     * @return the name of the column that should serve as the {@code RecordBoostField} for the index
     */
    public String getRecordBoostColumnName() { return null; }


    /**
     * Determines the field boost value for each column of type {@code TEXT} in the table. Each column of
     * type {@code TEXT} will be resolved as a searchable field for the schema of the index this {@code SQLiteIndexable}
     * represents: by returning an integer value for each column name of type {@code TEXT} in the table, the
     * field's boost value can be set. This is equivalent to constructing a field using the
     * {@link Field#createSearchablePrimaryKeyField(String, int)}; the name of the column will be passed
     * as the argument and the boost value that column should take should be returned.
     * <br><br>
     * By default each column
     * of type {@code TEXT} will receive a boost value of 1.
     * @return the value of boost to give to the column with the name corresponding to the value of {@code textTypeColumnName} passed
     */
    public int getColumnBoostValue(String textTypeColumnName) { return 1; }


    /**
     * Determines whether a column of type {@code TEXT} should be highlighted or not. This is equivalent to calling
     * {@link com.srch2.android.sdk.Field#enableHighlighting()} on an instance of a field; the name of each column
     * of type {@code TEXT}
     * will be passed and whether that column should be highlighted when returned as a search result record should
     * be returned.
     * <br><br>
     * By default highlighting is disabled for each column of type {@code TEXT}.
     * @return whether to highlight the column with the name corresponding to the value of {@code textTypeColumnName} passed
     */
    public boolean getColumnIsHighlighted(String textTypeColumnName) { return false; }

    /**
     * Implementing this method enables the SRCH2 search server to automatically observe data content
     * of the table that will be used to create and update the index this {@code SQLiteIndexable} represents.
     * <br><br>
     * The instance of the {@code SQLiteOpenHelper} is momentarily used to read the database table
     * information when {@link SRCH2Engine#initialize()} is called; a reference is not kept: a cursor is obtained
     * to read the database information, then both the readable database and cursor are closed within milliseconds.
     * @return the SQLiteOpenHelper used to manage the database
     */
    public abstract SQLiteOpenHelper getSQLiteOpenHelper();

    /**
     * Implementing this method enables the SRCH2 search server to automatically observe data content
     * of the table that will be used to create and update the index this {@code SQLiteIndexable} represents.
     * <br><br>
     * <b>It should exactly match</b> the value used in the create table string.
     * @return the table name <b>exactly</b> as it was specified in the create table string
     */
    public abstract String getTableName();

    /**
     * Implementing this method enables the SRCH2 search server to automatically observe data content
     * of the table that will be used to create and update the index this {@code SQLiteIndexable} represents.
     * <br><br>
     * <b>It should exactly match</b> the value used in the super constructor call of the
     * {@link android.database.sqlite.SQLiteOpenHelper} used to manage the database.
     * @return the database name <b>exactly</b> as it was specified in the constructor of the {@code SQLiteOpenHelper}
     */
    public abstract String getDatabaseName();

    /**
     * Implementing this method enables the SRCH2 search server to automatically observe data content
     * of the table that will be used to create and update the index this {@code SQLiteIndexable} represents.
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
     * of the table that will be used to create and update the index this {@code SQLiteIndexable} represents.
     * <br><br>
     * This method (and {@link #getLatitudeColumnName()}) must be overridden if the SQLite table contains geodata
     * that is to be searched on. <b>It should exactly match</b> the value used in the CREATE TABLE string matching
     * the name of the column that contains the longitude data.
     * <br><br>
     * Returning null is equivalent not overriding this method.
     * @return the name of the column in the SQLite table that contains the longitude data
     */
    public String getLongitudeColumnName() { return null; }


    final private Schema resolveSchemaFromSqliteOpenHelper(String tableName, SQLiteOpenHelper mSqliteOpenHelper,
                                                            boolean isProbablyGeo, boolean hasRecordBoostColumn) {

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
        RecordBoostField recordBoostField = null;
        boolean containsPrimaryKey = false;
        boolean containsAtLeastOneSearchableField = false;
        boolean containsLongitude = false;
        boolean containsLatitude = false;
        boolean containsRecordBoost = false;

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
                                // check if it is the record boost field

                                if (hasRecordBoostColumn && recordBoostField == null && name.equals(getRecordBoostColumnName())) {
                                    if (columnType == SQLiteColumnType.REAL || columnType == SQLiteColumnType.INTEGER) {
                                        recordBoostField = Field.createRecordBoostField(name);
                                        containsRecordBoost = true;
                                    } else {
                                        throw new IllegalArgumentException("While generating com.srch2.android.sdk.Schema from SQLite table, " +
                                           tableName + " the column " + name + " which matches the return value of getRecordBoostColumnName()" +
                                           " was found to be of type " + columnType.name() + ". This column must be of type REAL or INTEGER " +
                                            "in order to function as the RecordBoostField.");
                                    }
                                } else {
                                    Field extraField = null;
                                    switch (columnType.schemaType) {
                                        case Searchable:
                                            int boostValue = getColumnBoostValue(name);
                                            if (boostValue < 1) {
                                                boostValue = 1;
                                            }
                                            if (boostValue > 100) {
                                                boostValue = 100;
                                            }
                                            extraField = Field.createSearchableField(name, boostValue);

                                            if (getColumnIsHighlighted(name)) {
                                                extraField.enableHighlighting();
                                            }

                                            if (!containsAtLeastOneSearchableField) {
                                                containsAtLeastOneSearchableField = true;
                                            }

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
        if (!containsRecordBoost && hasRecordBoostColumn) {
            throw new IllegalArgumentException("While generating com.srch2.android.sdk.Schema from SQLite table, " +
                    tableName + " getRecordBoostColumn() returned a value that did not match the name of any columns" +
                    " in the table. Please verify the value getRecordBoostColumnName() returns matches the name of" +
                    "a column in that table that is of type REAL or INTEGER that represents the relative ranking of" +
                    "each row in the table");
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
            if (containsRecordBoost) {
                return new Schema(pkField, recordBoostField, getLatitudeColumnName(), getLongitudeColumnName(), fields.toArray(new Field[fields.size()]));
            } else {
                return new Schema(pkField, getLatitudeColumnName(), getLongitudeColumnName(), fields.toArray(new Field[fields.size()]));
            }
        } else {
            if (containsRecordBoost) {
                return new Schema(pkField, recordBoostField, fields.toArray(new Field[fields.size()]));
            } else {
                return new Schema(pkField, fields.toArray(new Field[fields.size()]));
            }
        }
    }







    private static enum SQLiteColumnType {
        TEXT(SchemaType.Searchable),
        INTEGER(SchemaType.RefiningInteger),
        NULL(null),
        BLOB(null), // represents a field of raw binary data (media, images, etc)
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
     * {@code SQLiteIndexable} represents.
     * <br><br>
     * <b>Note that this is a blocking call</b>: however it typically will block less than a dozen milliseconds.
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
                                SRCH2Engine.conf, requestingIndexable.indexInternal.indexDescription), InternalInfoTask.SHORT_CONNECTION_TIMEOUT_MS, false);
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
