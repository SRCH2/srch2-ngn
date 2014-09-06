package com.srch2.android.sdk;

import android.util.Log;

abstract class IndexableCore {

    /**
     * If returned from {@link #getRecordCount()} indicates this value has not yet been set.
     * <br><br>
     * Has the <b>constant</b> value of <code>-1</code>.
     */
    public static final int INDEX_RECORD_COUNT_NOT_SET = -1;

    /**
     * The JSON key to use to retrieve each original record from each <code>JSONObject</code> in
     * the <code>ArrayList<JSONObject</code> of the callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}.
     * <br><br>
     * Has the <b>constant</b> value '<code>record</code>';
     */
    public static final String SEARCH_RESULT_JSON_KEY_RECORD = "record";

    /**
     * The JSON key to use to retrieve each set of highlighted fields from each record from each <code>JSONObject</code> in
     * the <code>ArrayList<JSONObject</code> of the callback method
     * {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}.
     * <br><br>
     * Has the <b>constant</b> value '<code>highlighted</code>';
     */
    public static final String SEARCH_RESULT_JSON_KEY_HIGHLIGHTED = "highlighted";

    /**
     * The default value for the numbers of search results to return per search request.
     * <br><br>
     * Has the <b>constant</b> value of <code>10</code>.
     */
    public static final int DEFAULT_NUMBER_OF_SEARCH_RESULTS_TO_RETURN_AKA_TOPK = 10;

    /**
     * The default value for the fuzziness similarity threshold. Approximately one character
     * per every three characters will be allowed to act as a wildcard during a search.
     * <br><br>
     * Has the <b>constant</b> value of <code>0.65</code>.
     */
    public static final float DEFAULT_FUZZINESS_SIMILARITY_THRESHOLD = 0.65f;

    IndexInternal indexInternal;

    /**
     * Implementing this method sets the name of the index this <code>Indexable</code>
     * or <code>SQLiteIndexable</code> represents.
     * @return the name of the index this <code>Indexable</code> or <code>SQLiteIndexable</code> represents
     */
    abstract public String getIndexName();

    /**
     * Implementing this method sets the schema of the index this <code>Indexable</code>
     * or <code>SQLiteIndexable</code> represents. The schema
     * defines the data fields of the index, much like the table structure of an SQLite database table. See
     * {@link Schema} for more information.
     * <br><br>
     * <b>Note that if this is <code>SQLiteIndexable</code> the schema must correspond to the
     * table structure</b>: the {@link com.srch2.android.sdk.PrimaryKeyField} must correspond to the
     * auto-incrementing primary key of the table; any text columns should be searchable; any integer or
     * real values should be refining (but can be searchable and refining).
     * @return the schema to define the index structure this <code>Indexable</code> or <code>SQLiteIndexable</code> represents
     */
    abstract public Schema getSchema();

    /**
     * Implementing this method sets the highlighter for this <code>Indexable</code> or
     * <code>SQLiteIndexable</code>. If this method is not overridden the default highlighter
     * will be returned which will make prefix and fuzzy matching text bold using HTML tagging.
     * See {@link com.srch2.android.sdk.Highlighter} and {@link com.srch2.android.sdk.Highlighter.SimpleHighlighter}
     * for more information.
     * @return the highlighter used to visually match the text of the search input to the text of each searchable field
     */
    public Highlighter getHighlighter() {
        return new Highlighter();
    }
    int numberOfDocumentsInTheIndex = INDEX_RECORD_COUNT_NOT_SET;
    /**
     * Returns the number of records that are currently in the index that this
     * <code>Indexable</code> or <code>SQLiteIndexable</code> represents.
     * @return the number of records in the index
     */
    public int getRecordCount() {
        return numberOfDocumentsInTheIndex;
    }

    final void setRecordCount(int recordCount) {
        numberOfDocumentsInTheIndex = recordCount;
    }

    /**
     * Override this method to set the number of search results to be returned per query or search task.
     * <br><br>
     * The default value of this method, if not overridden, is ten.
     * <br><br>
     * This method will cause an <code>IllegalArgumentException</code> to be thrown when calling
     * {@link SRCH2Engine#initialize()} and passing this
     * <code>Indexable</code> or <code>SQLiteIndexable</code> if the returned value
     *  is less than one.
     * @return the number of results to return per search
     */
    public int getTopK() {
        return DEFAULT_NUMBER_OF_SEARCH_RESULTS_TO_RETURN_AKA_TOPK;
    }

    /**
     * Set the default fuzziness similarity threshold. This will determine how many character
     * substitutions the original search input will match search results for: if set to 0.5,
     * the search performed will include results as if half of the characters of the original
     * search input were replaced by wild card characters.
     * <br><br>
     * <b>Note:</b> In the the formation of a {@link Query}, each <code>Term</code> can
     * have its own fuzziness similarity threshold value set by calling the method
     * {@link SearchableTerm#enableFuzzyMatching(float)}; by default it is disabled for terms.
     * <br><br>
     * The default value of this method, if not overridden, is 0.65.
     * <br><br>
     * This method will cause an <code>IllegalArgumentException</code> to be thrown when calling
     * {@link SRCH2Engine#initialize()} and passing this <code>Indexable</code> or
     * <code>SQLiteIndexable</code> if the
     * value returned is less than zero or greater than one.
     * @return the fuzziness similarity ratio to match against search keywords against
     */
    public float getFuzzinessSimilarityThreshold() { return DEFAULT_FUZZINESS_SIMILARITY_THRESHOLD; }

    /**
     * Does a basic search on the index that this <code>Indexable</code> pr
     * <code>SQLiteIndexable</code> represents. A basic
     * search means that all distinct keywords (delimited by white space) of the
     * <code>searchInput</code> are treated as fuzzy, and the last keyword will
     * be treated as fuzzy and prefix.
     * <br><br>
     * For more configurable searches, use the
     * {@link Query} class in conjunction with the {@link #advancedSearch(Query)}
     * method.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * {@link SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     *  will be triggered. The
     * <code>HashMap resultMap</code> argument will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw an exception is the value of <code>searchInput</code> is null
     * or has a length less than one.
     * @param searchInput the textual input to search on
     */
    public final void search(String searchInput) {
        if (indexInternal != null) {
            indexInternal.search(searchInput);
        }
    }

    /**
     * Does an advanced search by forming search request input as a {@link Query}. This enables
     * searches to use all the advanced features of the SRCH2 search engine, such as per term
     * fuzziness similarity thresholds, limiting the range of results to a refining field, and
     * much more. See the {@link Query} for more details.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * {@link SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * will be triggered. The argument
     * <code>HashMap resultMap</code> will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw an exception if the value of <code>query</code> is null.
     * @param query the formation of the query to do the advanced search
     */
    public final void advancedSearch(Query query) {
        if (indexInternal != null) {
            indexInternal.advancedSearch(query);
        }
    }

    /**
     * Callback executed very shortly after the call to
     * {@link com.srch2.android.sdk.SRCH2Engine#onStart(android.content.Context)} is made:
     * when the SRCH2 search server is initialized by the <code>SRCH2Engine</code> (by the method just
     * mentioned), it will load each index into memory; this can take anywhere from a couple of milliseconds
     * to three seconds (depending on the number of records, how much data each record contains, and the
     * processing power of the device). When the index this <code>Indexable</code> or <code>SQLiteIndexable</code></code>
     * represents is finished loading,
     * this method is thus triggered. At this point, all state operations on this index if it is
     * an <code>Indexable</code> are valid: insert, update, et
     * cetera; for both <code>Indexable</code> and <code>SQLiteIndexable</code> instances searches are now ready
     * at this point.
     * <br><br>
     * By overriding this method, its implementation can be used to verify the integrity of the index such as if
     * records need to be inserted (by checking {@link #getRecordCount()} for the first time) or likewise if the index
     * needs to be updated.
     * <br><br>
     * <i>This method does not have to be overridden</i> (thought it is <b>strongly encouraged</b> to do so). If it is
     * not, the number of records it contains upon being loaded will be printed to logcat
     * under the tag 'SRCH2' with the message prefixed by the name of the index this <code>Indexable</code> or
     * <code>SQLiteIndexable</code> represents.
     */
    public void onIndexReady() {
        Log.d("SRCH2", "Index " + getIndexName() + " is ready to be accessed and contains " + getRecordCount() + " records");
    }
}
