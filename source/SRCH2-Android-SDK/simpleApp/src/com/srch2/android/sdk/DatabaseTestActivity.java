package com.srch2.android.sdk;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import static junit.framework.Assert.*;

public class DatabaseTestActivity extends TestableActivity {

    DbIndex dbIndex;
    DbHelper dbHelper;
    SearchResultsCallback searchResultsCallback;

    @Override
    public List<String> getTestMethodNameListWithOrder() {
        return Arrays.asList(new String[]{
            "testAll"
        });
    }

    @Override
    public void beforeAll() {

    }

    @Override
    public void afterAll() {

    }

    @Override
    public void beforeEach() {
        Log.d("SRCH2", "before each!");
        long t = SystemClock.uptimeMillis();
        onStopAndWaitForNotIsReady(getApplicationContext(), 10000);
        long e = SystemClock.uptimeMillis() - t;


        deleteSrch2Files();
        getApplicationContext().deleteDatabase(SQLiteSchema.DATABASE_NAME);
        SRCH2Engine.setSearchResultsListener(searchResultsCallback);
        SRCH2Engine.setAutomatedTestingMode(true);
    }

    @Override
    public void afterEach() {
        /*
        onStopAndWaitForNotIsReady(this, 7000);
        assertFalse(SRCH2Engine.isReady()); */
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }


    public void testAll() {
        // "Unit Tests" (requires context hence here in integration testing)
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test null database");
        testNullDatabaseName();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test null table name");
        testNullTableName();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test invalid database");
        testInvalidDatabaseName();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test invalid table name");
        testInvalidTableName();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test null sqlite open helper");
        testNullSQLiteOpenHelper();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test invalid latitude column");
        testInvalidLatitudeColumnNameGeo();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test invalid longitude column");
        testInvalidLongitudeColumnNameGeo();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test incorrect latitude column");
        testIncorrectLatitudeColumnNameGeo();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test incorrect longitude column");
        testIncorrectLongitudeColumnNameGeo();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test incorrect database name");
        testIncorrectDatabaseName();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test no searchable columns");
        testNoSearchableColumns();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test no primary key");
        testNoPrimaryKey();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test invalid record boost column name");
        testInvalidRecordBoostColumnName();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test incorrect record boost column name");
        testIncorrectRecordBoostColumnName();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test wrong column type for record boost column name");
        testWrongColumnTypeForRecordBoostColumnName();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test valid record boost column name");
        testValidRecordBoost();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test valid no record boost column name");
        testValidNoRecordBoost();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test valid column boost");
        testColumnBoostValid();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test column boost normalizes out of range");
        testColumnBoostNormalizesOutOfRangeValues();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test column boost valid without override");
        testColumnBoostWithoutOverride();

        // Integration tests
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test VALID GEO sqlite database connectored index");
        testValidGeo();
        beforeEach();
        Log.d("s2sdk:: databasetest", "%%%%%%%%%% test VALID sqlite database connectored index");
        testValid();
    }


    public void testColumnBoostWithoutOverride() {
        FieldBoostDbHelper h = new FieldBoostDbHelper(getApplicationContext());
        FieldBoostIndex i = new FieldBoostIndex(h, FieldBoostIndex.FieldBoostTestMode.DoNotOverride);
        SRCH2Engine.setSQLiteIndexables(i);
        SRCH2Engine.onStart();
        String configFile = SRCH2Configuration.generateConfigurationFileString(SRCH2Engine.conf);
        int start = configFile.indexOf("<fieldBoost>");
        int end = configFile.indexOf("</fieldBoost>");
        String fieldBoostString = configFile.substring(start, end + 1);
        boolean hasCorrectFieldBoosts = true;
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_ONE + "^1")) {
            hasCorrectFieldBoosts = false;
        }
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_TWO + "^1")) {
            hasCorrectFieldBoosts = false;
        }
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_THREE + "^1")) {
            hasCorrectFieldBoosts = false;
        }
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_FOUR + "^1")) {
            hasCorrectFieldBoosts = false;
        }
        assertTrue(hasCorrectFieldBoosts);
    }

    public void testColumnBoostNormalizesOutOfRangeValues() {
        FieldBoostDbHelper h = new FieldBoostDbHelper(getApplicationContext());
        FieldBoostIndex i = new FieldBoostIndex(h, FieldBoostIndex.FieldBoostTestMode.OverrideWithOutOfRangeValues);
        SRCH2Engine.setSQLiteIndexables(i);
        SRCH2Engine.onStart();
        String configFile = SRCH2Configuration.generateConfigurationFileString(SRCH2Engine.conf);
        int start = configFile.indexOf("<fieldBoost>");
        int end = configFile.indexOf("</fieldBoost>");
        String fieldBoostString = configFile.substring(start, end + 1);
        boolean hasCorrectFieldBoosts = true;
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_ONE + "^1")) {
            hasCorrectFieldBoosts = false;
        }
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_TWO + "^1")) {
            hasCorrectFieldBoosts = false;
        }
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_THREE + "^100")) {
            hasCorrectFieldBoosts = false;
        }
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_FOUR + "^4")) {
            hasCorrectFieldBoosts = false;
        }
        assertTrue(hasCorrectFieldBoosts);
    }

    public void testColumnBoostValid() {
        FieldBoostDbHelper h = new FieldBoostDbHelper(getApplicationContext());
        FieldBoostIndex i = new FieldBoostIndex(h, FieldBoostIndex.FieldBoostTestMode.OverrideInOrder);
        SRCH2Engine.setSQLiteIndexables(i);
        SRCH2Engine.onStart();
        String configFile = SRCH2Configuration.generateConfigurationFileString(SRCH2Engine.conf);
        int start = configFile.indexOf("<fieldBoost>");
        int end = configFile.indexOf("</fieldBoost>");
        String fieldBoostString = configFile.substring(start, end + 1);
        boolean hasCorrectFieldBoosts = true;
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_ONE + "^1")) {
            hasCorrectFieldBoosts = false;
        }
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_TWO + "^2")) {
            hasCorrectFieldBoosts = false;
        }
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_THREE + "^3")) {
            hasCorrectFieldBoosts = false;
        }
        if (!fieldBoostString.contains(FieldBoostDbHelper.COLUMN_FOUR + "^4")) {
            hasCorrectFieldBoosts = false;
        }
        assertTrue(hasCorrectFieldBoosts);
    }


    public void testValidNoRecordBoost() {
        RecordBoostDbHelper h = new RecordBoostDbHelper(getApplicationContext(), RecordBoostDbIndex.RecordBoostTestMode.NoRecordBoost);
        RecordBoostDbIndex i = new RecordBoostDbIndex(h, RecordBoostDbIndex.RecordBoostTestMode.NoRecordBoost);
        SRCH2Engine.setSQLiteIndexables(i);
        SRCH2Engine.onStart();
        String configFile = SRCH2Configuration.generateConfigurationFileString(SRCH2Engine.conf);
        assertTrue(!configFile.contains("<recordBoostField>"));
    }


    public void testValidRecordBoost() {
        RecordBoostDbHelper h = new RecordBoostDbHelper(getApplicationContext(), RecordBoostDbIndex.RecordBoostTestMode.Valid);
        RecordBoostDbIndex i = new RecordBoostDbIndex(h, RecordBoostDbIndex.RecordBoostTestMode.Valid);
        SRCH2Engine.setSQLiteIndexables(i);
        SRCH2Engine.onStart();
        String configFile = SRCH2Configuration.generateConfigurationFileString(SRCH2Engine.conf);

        assertTrue(configFile.contains("<recordBoostField>" + RecordBoostDbHelper.COLUMN_RECORD_BOOST + "</recordBoostField>"));
    }


    public void testWrongColumnTypeForRecordBoostColumnName() {
        RecordBoostDbHelper h = new RecordBoostDbHelper(getApplicationContext(), RecordBoostDbIndex.RecordBoostTestMode.WrongColumnType);
        RecordBoostDbIndex i = new RecordBoostDbIndex(h, RecordBoostDbIndex.RecordBoostTestMode.WrongColumnType);
        SRCH2Engine.setSQLiteIndexables(i);
        boolean test = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalArgumentException e) {
            test = true;
        }
        assertTrue(test);
    }

    public void testIncorrectRecordBoostColumnName() {
        RecordBoostDbHelper h = new RecordBoostDbHelper(getApplicationContext(), RecordBoostDbIndex.RecordBoostTestMode.IncorrectName);
        RecordBoostDbIndex i = new RecordBoostDbIndex(h, RecordBoostDbIndex.RecordBoostTestMode.IncorrectName);
        SRCH2Engine.setSQLiteIndexables(i);
        boolean test = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalArgumentException e) {
            test = true;
        }
        assertTrue(test);
    }

    public void testInvalidRecordBoostColumnName() {
        RecordBoostDbHelper h = new RecordBoostDbHelper(getApplicationContext(), RecordBoostDbIndex.RecordBoostTestMode.InvalidName);
        RecordBoostDbIndex i = new RecordBoostDbIndex(h, RecordBoostDbIndex.RecordBoostTestMode.InvalidName);
        SRCH2Engine.setSQLiteIndexables(i);
        boolean test = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalArgumentException e) {
            test = true;
        }
        assertTrue(test);
    }

    public void testNoPrimaryKey() {
        NoPrimaryKeyDbHelper h = new NoPrimaryKeyDbHelper(getApplicationContext());
        NoPrimaryKeyDbIndex i = new NoPrimaryKeyDbIndex(h);
        SRCH2Engine.setSQLiteIndexables(i);
        boolean wasInvalid = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalStateException e) {
            wasInvalid = true;
        }
        assertTrue(wasInvalid);
    }

    public void testNoSearchableColumns() {
        NoSearchableDbHelper h = new NoSearchableDbHelper(getApplicationContext());
        NoSearchableDbIndex i = new NoSearchableDbIndex(h);
        SRCH2Engine.setSQLiteIndexables(i);
        boolean wasInvalid = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalStateException e) {
            wasInvalid = true;
        }
        assertTrue(wasInvalid);
    }

    public void testIncorrectLatitudeColumnNameGeo() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.GeoIncorrectLatitude);
        dbIndex = new DbIndex(SQLiteSchema.DATABASE_NAME, SQLiteSchema.TABLE_NAME, dbHelper, DbIndex.GeoFailureMode.GeoIncorrectLatitude);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        boolean wasInvalid = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalStateException e) {
            wasInvalid = true;
        }
        assertTrue(wasInvalid);
    }

    public void testIncorrectLongitudeColumnNameGeo() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.GeoIncorrectLongitude);
        dbIndex = new DbIndex(SQLiteSchema.DATABASE_NAME, SQLiteSchema.TABLE_NAME, dbHelper, DbIndex.GeoFailureMode.GeoIncorrectLongitude);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        boolean wasInvalid = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalStateException e) {
            wasInvalid = true;
        }
        assertTrue(wasInvalid);
    }

    public void testIncorrectDatabaseName() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.GeoValid);
        dbIndex = new DbIndex("egeg", SQLiteSchema.TABLE_NAME, dbHelper, DbIndex.GeoFailureMode.GeoValid);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        boolean wasInvalid = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalStateException e) {
            wasInvalid = true;
        }
        assertTrue(wasInvalid);
    }

    public void testInvalidLongitudeColumnNameGeo() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.GeoInvalidLongitude);
        dbIndex = new DbIndex(SQLiteSchema.DATABASE_NAME, SQLiteSchema.TABLE_NAME, dbHelper, DbIndex.GeoFailureMode.GeoInvalidLongitude);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        boolean wasInvalid = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalArgumentException e) {
            wasInvalid = true;
        }
        assertTrue(wasInvalid);
    }

    public void testInvalidLatitudeColumnNameGeo() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.GeoInvalidLatitude);
        dbIndex = new DbIndex(SQLiteSchema.DATABASE_NAME, SQLiteSchema.TABLE_NAME, dbHelper, DbIndex.GeoFailureMode.GeoInvalidLatitude);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        boolean wasInvalid = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalArgumentException e) {
            wasInvalid = true;
        }
        assertTrue(wasInvalid);
    }

    public void testValidGeo() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.GeoValid);
        dbIndex = new DbIndex(SQLiteSchema.DATABASE_NAME, SQLiteSchema.TABLE_NAME, dbHelper, DbIndex.GeoFailureMode.GeoValid);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        SRCH2Engine.onStart();
        onStartAndWaitForIsReady(this, 60000);
        assertTrue(SRCH2Engine.isReady());
        dbHelper.insertRecords();
        sleep(20000);

        assertEquals(dbIndex.getRecordCount(), SQLiteSchema.NUMBER_OF_RECORDS_TO_INSERT);
        onStopAndWaitForNotIsReady(this, 7000);
        assertFalse(SRCH2Engine.isReady());
    }

    public void testValid() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.NotGeo);
        dbIndex = new DbIndex(SQLiteSchema.DATABASE_NAME, SQLiteSchema.TABLE_NAME, dbHelper, DbIndex.GeoFailureMode.NotGeo);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        SRCH2Engine.onStart();
        onStartAndWaitForIsReady(this, 60000);
        assertTrue(SRCH2Engine.isReady());
        sleep(5000);
        assertEquals(dbIndex.getRecordCount(), SQLiteSchema.NUMBER_OF_RECORDS_TO_INSERT);
        onStopAndWaitForNotIsReady(this, 7000);
        assertFalse(SRCH2Engine.isReady());
    }

    public void testNullSQLiteOpenHelper() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.NotGeo);
        dbIndex = new DbIndex(SQLiteSchema.DATABASE_NAME, SQLiteSchema.TABLE_NAME, null, DbIndex.GeoFailureMode.NotGeo);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        boolean wasNull = false;
        try {
            SRCH2Engine.onStart();
        } catch (NullPointerException npe) {
            wasNull = true;
        }
        assertTrue(wasNull);
    }

    public void testNullDatabaseName() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.NotGeo);
        dbIndex = new DbIndex(null, SQLiteSchema.TABLE_NAME, dbHelper, DbIndex.GeoFailureMode.NotGeo);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        boolean wasNull = false;
        try {
            SRCH2Engine.onStart();
        } catch (NullPointerException npe) {
            wasNull = true;
        }
        assertTrue(wasNull);
    }

    public void testNullTableName() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.NotGeo);
        dbIndex = new DbIndex(SQLiteSchema.DATABASE_NAME, null, dbHelper, DbIndex.GeoFailureMode.NotGeo);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        boolean wasNull = false;
        try {
            SRCH2Engine.onStart();
        } catch (NullPointerException npe) {
            wasNull = true;
        }
        assertTrue(wasNull);
    }

    public void testInvalidDatabaseName() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.NotGeo);
        dbIndex = new DbIndex("", SQLiteSchema.TABLE_NAME, dbHelper, DbIndex.GeoFailureMode.NotGeo);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        boolean wasInvalid = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalArgumentException npe) {
            wasInvalid = true;
        }
        assertTrue(wasInvalid);
    }

    public void testInvalidTableName() {
        dbHelper = new DbHelper(getApplicationContext(), DbIndex.GeoFailureMode.NotGeo);
        dbIndex = new DbIndex(SQLiteSchema.DATABASE_NAME, "", dbHelper, DbIndex.GeoFailureMode.NotGeo);
        SRCH2Engine.setSQLiteIndexables(dbIndex);
        boolean wasInvalid = false;
        try {
            SRCH2Engine.onStart();
        } catch (IllegalArgumentException npe) {
            wasInvalid = true;
        }
        assertTrue(wasInvalid);
    }


    static class SearchResultsCallback implements SearchResultsListener {
        @Override
        public void onNewSearchResults(int HTTPResponseCode, String JSONResponse, HashMap<String, ArrayList<JSONObject>> resultMap) {

        }
    }

    public static class SQLiteSchema {
        public static final int NUMBER_OF_RECORDS_TO_INSERT = 10;
        public static final String DATABASE_NAME = "demo";
        public static final int DATABASE_VERSION = 1;
        public static final String TABLE_NAME = "company";
        public static final String COLUMN_PRIMARY_KEY = "_id";
        public static final String COLUMN_NAME = "name";
        public static final String COLUMN_AGE = "age";
        public static final String COLUMN_ADDRESS = "address";
        public static final String COLUMN_BLOB = "blobo";
        public static final String COLUMN_SALARY = "salary";
        public static final String COLUMN_LONGITUDE = "longitude";
        public static final String COLUMN_LATITUDE = "latitude";

    }

    static class DbIndex extends SQLiteIndexable {
        static String TABLE_NAME;
        static String DATABASE_NAME;
        SQLiteOpenHelper dbHelper;
        static String INDEX_NAME;

        GeoFailureMode failureMode;

        DbIndex(String dbName, String tableName, SQLiteOpenHelper helper, GeoFailureMode geoFailureMode) {
            TABLE_NAME = tableName;
            DATABASE_NAME = dbName;
            dbHelper = helper;
            INDEX_NAME = SQLiteSchema.TABLE_NAME;
            failureMode = geoFailureMode;
        }

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
            return dbHelper;
        }

        @Override
        public String getLatitudeColumnName() {
            switch (failureMode) {
                case GeoValid:
                case GeoInvalidLongitude:
                case GeoNoLongitude:
                case GeoIncorrectLongitude:
                    return SQLiteSchema.COLUMN_LATITUDE;
                case GeoNoLatitude:
                    return null;
                case GeoInvalidLatitude:
                    return "";
                case GeoIncorrectLatitude:
                    return "xsfsdf";
                case NotGeo:
                default:
                    return null;
            }
        }

        @Override
        public String getLongitudeColumnName() {
            switch (failureMode) {
                case GeoValid:
                case GeoIncorrectLatitude:
                case GeoInvalidLatitude:
                case GeoNoLatitude:
                    return SQLiteSchema.COLUMN_LONGITUDE;
                case GeoNoLongitude:
                    return null;
                case GeoInvalidLongitude:
                    return "";
                case GeoIncorrectLongitude:
                    return "sdfsdfg";
                case NotGeo:
                default:
                    return null;
            }
        }

        public static enum GeoFailureMode {
            NotGeo,
            GeoValid,
            GeoNoLatitude,
            GeoNoLongitude,
            GeoInvalidLatitude,
            GeoInvalidLongitude,
            GeoIncorrectLatitude,
            GeoIncorrectLongitude;
        }
    }

    public static class DbHelper extends SQLiteOpenHelper {

        private static String INCORRECT_LATITUDE_NAME = "lat";
        private static String INCORRECT_LONGITUDE_NAME = "long";

        private DbIndex.GeoFailureMode failureMode;

        public DbHelper(Context context, DbIndex.GeoFailureMode geofailureMode) {
            super(context, SQLiteSchema.DATABASE_NAME, null, SQLiteSchema.DATABASE_VERSION);
            failureMode = geofailureMode;
        }

        public static final String getCreateTableString(DbIndex.GeoFailureMode mode) {
            StringBuilder sb = new StringBuilder();
            sb.append("CREATE TABLE " + SQLiteSchema.TABLE_NAME + "(");
            sb.append(SQLiteSchema.COLUMN_PRIMARY_KEY + " INTEGER PRIMARY KEY NOT NULL, ");
            sb.append(SQLiteSchema.COLUMN_NAME + " TEXT NOT NULL, ");
            sb.append(SQLiteSchema.COLUMN_AGE + " INTEGER NOT NULL, ");
            sb.append(SQLiteSchema.COLUMN_ADDRESS + " TEXT, ");
            sb.append(SQLiteSchema.COLUMN_SALARY + " REAL, ");
            sb.append(SQLiteSchema.COLUMN_BLOB + " BLOB, ");
            switch (mode) {
                case NotGeo:
                    sb.deleteCharAt(sb.lastIndexOf(","));
                case GeoValid:
                    sb.append(SQLiteSchema.COLUMN_LATITUDE + " REAL, ");
                    sb.append(SQLiteSchema.COLUMN_LONGITUDE + " REAL");
                    break;
                case GeoNoLatitude:
                case GeoInvalidLatitude:
                    sb.append(SQLiteSchema.COLUMN_LONGITUDE + " REAL");
                    break;
                case GeoNoLongitude:
                case GeoInvalidLongitude:
                    sb.append(SQLiteSchema.COLUMN_LATITUDE + " REAL");
                    break;
                case GeoIncorrectLongitude:
                    sb.append(SQLiteSchema.COLUMN_LATITUDE + " REAL, ");
                    sb.append(INCORRECT_LONGITUDE_NAME + " REAL");
                    break;
                case GeoIncorrectLatitude:
                    sb.append(SQLiteSchema.COLUMN_LATITUDE + " REAL, ");
                    sb.append(INCORRECT_LATITUDE_NAME + " REAL");
            }
            sb.append(")");
            Log.d("SRCH2", "createTableString for failureMode " + mode.name() + " is " + sb.toString());
            return sb.toString();
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL("DROP TABLE IF EXISTS " + SQLiteSchema.TABLE_NAME);
            db.execSQL(getCreateTableString(failureMode));
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            onCreate(db);
        }

        public void insertRecords() {
            SQLiteDatabase db = getWritableDatabase();
            Log.d("SRCH2", "INSERTING RECORDS!");
            try {
                ContentValues cv = new ContentValues();
                for (int i = 0; i < SQLiteSchema.NUMBER_OF_RECORDS_TO_INSERT; i++) {
                    cv.clear();
                    cv.put(SQLiteSchema.COLUMN_PRIMARY_KEY, i);
                    cv.put(SQLiteSchema.COLUMN_AGE, i);
                    cv.put(SQLiteSchema.COLUMN_NAME, "name " + RandomStringUtil.getRandomString(10));
                    cv.put(SQLiteSchema.COLUMN_ADDRESS, "address " + RandomStringUtil.getRandomString(20));
                    cv.put(SQLiteSchema.COLUMN_SALARY, (double) i);
                    switch (failureMode) {
                        case GeoValid:
                            cv.put(SQLiteSchema.COLUMN_LATITUDE, (double) i + .01);
                            cv.put(SQLiteSchema.COLUMN_LONGITUDE, (double) i + .01);
                            break;
                        case GeoNoLatitude:
                        case GeoInvalidLatitude:
                            cv.put(SQLiteSchema.COLUMN_LONGITUDE, (double) i + .01);
                            break;
                        case GeoNoLongitude:
                        case GeoInvalidLongitude:
                            cv.put(SQLiteSchema.COLUMN_LATITUDE, (double) i + .01);
                            break;
                        case GeoIncorrectLatitude:
                            cv.put(INCORRECT_LATITUDE_NAME, (double) i + .01);
                            cv.put(SQLiteSchema.COLUMN_LONGITUDE, (double) i + .01);
                            break;
                        case GeoIncorrectLongitude:
                            cv.put(SQLiteSchema.COLUMN_LATITUDE, (double) i + .01);
                            cv.put(INCORRECT_LONGITUDE_NAME, (double) i + .01);
                    }
                    db.insert(SQLiteSchema.TABLE_NAME, null, cv);
                }
            } catch (Exception e) {
                Log.d("SRCH", "error inserting records for test database!&&&&&&&&&&&&&&&&&&&&&&%%%%%%%%%%%%%");
                e.printStackTrace();
            } finally {
                if (db != null) {
                    db.close();
                }
            }
            db = getReadableDatabase();
            Cursor c = db.rawQuery("select * from " + SQLiteSchema.TABLE_NAME, null);
            if (c != null) {
                Log.d("SRCH2", "INSERTED TEST RECORDS FOR SQLITE DB INDEX count in table is now " + c.getCount());
                c.close();
            }
            if (db != null) {
                db.close();
            }
        }
    }







    static class NoSearchableDbIndex extends SQLiteIndexable {
        SQLiteOpenHelper h;
        public NoSearchableDbIndex(SQLiteOpenHelper helper) {
            h = helper;
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
        public String getIndexName() {
            return SQLiteSchema.TABLE_NAME;
        }

        @Override
        public SQLiteOpenHelper getSQLiteOpenHelper() {
            return h;
        }
    }

    public static class NoSearchableDbHelper extends SQLiteOpenHelper {

        public NoSearchableDbHelper(Context context) {
            super(context, SQLiteSchema.DATABASE_NAME, null, SQLiteSchema.DATABASE_VERSION);
        }

        public static final String getCreateTableString() {
            StringBuilder sb = new StringBuilder();
            sb.append("CREATE TABLE " + SQLiteSchema.TABLE_NAME + "(");
            sb.append(SQLiteSchema.COLUMN_PRIMARY_KEY + " INTEGER PRIMARY KEY NOT NULL, ");
            sb.append("helloColumn" + " REAL ");
            sb.append(")");
            return sb.toString();
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL("DROP TABLE IF EXISTS " + SQLiteSchema.TABLE_NAME);
            db.execSQL(getCreateTableString());
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            onCreate(db);
        }
    }




    static class NoPrimaryKeyDbIndex extends SQLiteIndexable {
        SQLiteOpenHelper h;
        public NoPrimaryKeyDbIndex(SQLiteOpenHelper helper) {
            h = helper;
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
        public String getIndexName() {
            return SQLiteSchema.TABLE_NAME;
        }

        @Override
        public SQLiteOpenHelper getSQLiteOpenHelper() {
            return h;
        }
    }

    public static class NoPrimaryKeyDbHelper extends SQLiteOpenHelper {
        public NoPrimaryKeyDbHelper(Context context) {
            super(context, SQLiteSchema.DATABASE_NAME, null, SQLiteSchema.DATABASE_VERSION);
        }

        public static final String getCreateTableString() {
            StringBuilder sb = new StringBuilder();
            sb.append("CREATE TABLE " + SQLiteSchema.TABLE_NAME + "(");
            sb.append("helloColumn" + " REAL ");
            sb.append(")");
            return sb.toString();
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL("DROP TABLE IF EXISTS " + SQLiteSchema.TABLE_NAME);
            db.execSQL(getCreateTableString());
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            onCreate(db);
        }
    }

    static class RecordBoostDbIndex extends SQLiteIndexable {
        public static enum RecordBoostTestMode {
            InvalidName,
            IncorrectName,
            Valid,
            NoRecordBoost,
            WrongColumnType,
        }

        RecordBoostTestMode m;
        SQLiteOpenHelper h;
        public RecordBoostDbIndex(SQLiteOpenHelper helper, RecordBoostTestMode mode) {
            h = helper;
            m = mode;
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
        public String getIndexName() {
            return SQLiteSchema.TABLE_NAME;
        }

        @Override
        public SQLiteOpenHelper getSQLiteOpenHelper() {
            return h;
        }

        @Override
        public String getRecordBoostColumnName() {
            switch (m) {
                case WrongColumnType:
                case Valid:
                    return RecordBoostDbHelper.COLUMN_RECORD_BOOST;
                case InvalidName:
                    return "";
                case IncorrectName:
                    return "b00st";
                default:
                case NoRecordBoost:
                    return null;
            }
        }
    }

    public static class RecordBoostDbHelper extends SQLiteOpenHelper {

        public static final String COLUMN_RECORD_BOOST = "boost";
        public static final String COLUMN_SEARCHABLE = "searchable";

        static RecordBoostDbIndex.RecordBoostTestMode testMode;

        public RecordBoostDbHelper(Context context, RecordBoostDbIndex.RecordBoostTestMode mode) {
            super(context, SQLiteSchema.DATABASE_NAME, null, SQLiteSchema.DATABASE_VERSION);
            testMode = mode;
        }

        public static final String getCreateTableString(RecordBoostDbIndex.RecordBoostTestMode mode) {
            StringBuilder sb = new StringBuilder();
            sb.append("CREATE TABLE " + SQLiteSchema.TABLE_NAME + "(");
            sb.append(SQLiteSchema.COLUMN_PRIMARY_KEY + " INTEGER PRIMARY KEY NOT NULL, ");
            sb.append(COLUMN_SEARCHABLE + " TEXT, ");
            switch (testMode) {
                case Valid:
                case InvalidName:
                case IncorrectName:
                    sb.append(COLUMN_RECORD_BOOST + " REAL ");
                    break;
                case WrongColumnType:
                    sb.append(COLUMN_RECORD_BOOST + " TEXT ");
                    break;
                default:
                case NoRecordBoost:
                    sb.deleteCharAt(sb.lastIndexOf(","));
                    break;
            }
            sb.append(")");
            return sb.toString();
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL("DROP TABLE IF EXISTS " + SQLiteSchema.TABLE_NAME);
            db.execSQL(getCreateTableString(testMode));
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            onCreate(db);
        }
    }







    static class FieldBoostIndex extends SQLiteIndexable {
        public static enum FieldBoostTestMode {
            DoNotOverride,
            OverrideInOrder,
            OverrideWithOutOfRangeValues,
        }

        FieldBoostTestMode m;
        SQLiteOpenHelper h;
        public FieldBoostIndex(SQLiteOpenHelper helper, FieldBoostTestMode mode) {
            h = helper;
            m = mode;
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
        public String getIndexName() {
            return SQLiteSchema.TABLE_NAME;
        }

        @Override
        public SQLiteOpenHelper getSQLiteOpenHelper() {
            return h;
        }

        @Override
        public int getColumnBoostValue(String textTypeColumnName) {
            int val = 1;
            switch (m) {
                case OverrideInOrder:
                    if (textTypeColumnName.equals(FieldBoostDbHelper.COLUMN_ONE)) {
                        val = 1;
                    } else if (textTypeColumnName.equals(FieldBoostDbHelper.COLUMN_TWO)) {
                        val = 2;
                    } else if (textTypeColumnName.equals(FieldBoostDbHelper.COLUMN_THREE)) {
                        val = 3;
                    } else if (textTypeColumnName.equals(FieldBoostDbHelper.COLUMN_FOUR)) {
                        val = 4;
                    }
                    return val;
                case OverrideWithOutOfRangeValues:
                    if (textTypeColumnName.equals(FieldBoostDbHelper.COLUMN_ONE)) {
                        val = 0;
                    } else if (textTypeColumnName.equals(FieldBoostDbHelper.COLUMN_TWO)) {
                        val = -1;
                    } else if (textTypeColumnName.equals(FieldBoostDbHelper.COLUMN_THREE)) {
                        val = 101;
                    } else if (textTypeColumnName.equals(FieldBoostDbHelper.COLUMN_FOUR)) {
                        val = 4;
                    }
                    return val;
                default:
                case DoNotOverride:
                    return super.getColumnBoostValue(textTypeColumnName);
            }
        }
    }

    public static class FieldBoostDbHelper extends SQLiteOpenHelper {

        public static final String COLUMN_ONE = "one";
        public static final String COLUMN_TWO = "two";
        public static final String COLUMN_THREE = "three";
        public static final String COLUMN_FOUR = "four";

        static FieldBoostIndex.FieldBoostTestMode testMode;

        public FieldBoostDbHelper(Context context ) {
            super(context, SQLiteSchema.DATABASE_NAME, null, SQLiteSchema.DATABASE_VERSION);
        }

        public static final String getCreateTableString() {
            StringBuilder sb = new StringBuilder();
            sb.append("CREATE TABLE " + SQLiteSchema.TABLE_NAME + "(");
            sb.append(SQLiteSchema.COLUMN_PRIMARY_KEY + " INTEGER PRIMARY KEY NOT NULL, ");
            sb.append(COLUMN_ONE + " TEXT, ");
            sb.append(COLUMN_TWO + " TEXT, ");
            sb.append(COLUMN_THREE + " TEXT, ");
            sb.append(COLUMN_FOUR + " TEXT ");
            sb.append(")");
            return sb.toString();
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL("DROP TABLE IF EXISTS " + SQLiteSchema.TABLE_NAME);
            db.execSQL(getCreateTableString());
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            onCreate(db);
        }
    }
}
