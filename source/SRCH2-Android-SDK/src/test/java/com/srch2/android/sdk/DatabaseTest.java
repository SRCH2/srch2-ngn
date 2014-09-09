package com.srch2.android.sdk;

import android.database.sqlite.SQLiteOpenHelper;
import junit.framework.Assert;
import org.junit.Test;

import java.util.ArrayList;

public class DatabaseTest {



    @Test(expected = NullPointerException.class)
    public void testDatabaseConnectorConfigurationNullDatabaseName() {
        ArrayList<IndexableCore> idxs = new ArrayList<IndexableCore>();
        idxs.add(new DbIndex(null, "table"));
        SRCH2Configuration config = new SRCH2Configuration(idxs);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testDatabaseConnectorConfigurationInvalidDatabaseName() {
        ArrayList<IndexableCore> idxs = new ArrayList<IndexableCore>();
        idxs.add(new DbIndex("", "table"));
        SRCH2Configuration config = new SRCH2Configuration(idxs);
    }

    @Test(expected = NullPointerException.class)
    public void testDatabaseConnectorConfigurationNullTableName() {
        ArrayList<IndexableCore> idxs = new ArrayList<IndexableCore>();
        idxs.add(new DbIndex("db", null));
        SRCH2Configuration config = new SRCH2Configuration(idxs);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testDatabaseConnectorConfigurationInvalidTableName() {
        ArrayList<IndexableCore> idxs = new ArrayList<IndexableCore>();
        idxs.add(new DbIndex("db", ""));
        SRCH2Configuration config = new SRCH2Configuration(idxs);
    }

    @Test
    public void testValidDatabase() {
        ArrayList<IndexableCore> idxs = new ArrayList<IndexableCore>();
        idxs.add(new DbIndex("db", "table"));
        SRCH2Configuration config = new SRCH2Configuration(idxs);
        Assert.assertEquals(config.indexableMap.get(DbIndex.INDEX_NAME)
                .indexInternal.indexDescription
                .sqliteDatabaseProperties.get(IndexDescription.DB_DATABASE_NAME), DbIndex.DATABASE_NAME);
        Assert.assertEquals(config.indexableMap.get(DbIndex.INDEX_NAME)
                .indexInternal.indexDescription
                .sqliteDatabaseProperties.get(IndexDescription.DB_DATABASE_TABLE_NAME), DbIndex.TABLE_NAME);
    }


    static class DbIndex extends SQLiteIndexable {

        static String TABLE_NAME;
        static String DATABASE_NAME;
        static final String INDEX_NAME = "table";
        static final String INDEX_PK_NAME = "id";
        static final String INDEX_TITLE_NAME = "title";

        DbIndex(String dbName, String tableName) {
            TABLE_NAME = tableName;
            DATABASE_NAME = dbName;
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
            return null;
        }
    }
}
