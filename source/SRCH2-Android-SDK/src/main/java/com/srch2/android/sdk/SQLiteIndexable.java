package com.srch2.android.sdk;

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






}
