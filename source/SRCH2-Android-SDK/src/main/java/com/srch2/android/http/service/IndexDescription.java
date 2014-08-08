package com.srch2.android.http.service;

import java.util.Iterator;
import java.util.Properties;

/**
 * Creates the necessary configuration for each index that will be searchable in the SRCH2 search server.
 * Each index in the SRCH2 search server requires a name and a schema which is defined in the construction
 * of this class.
 * <br><br>
 * The only place construction of an object of this class should occur is in the <code>Indexable</code>
 * implementation of the abstract method <code>public IndexDescription getIndexDescription()</code> when
 * returning from this method.
 */
final public class IndexDescription {

    private static final String RECORD_SCORE_EXPRESSION = "recordScoreExpression";
    private static final String FUZZY_MATCH_PENALTY = "fuzzyMatchPenalty";
    private static final String QUERY_TERM_SIMILARITY_THRESHOLD = "queryTermSimilarityThreshold";
    private static final String PREFIX_MATCH_PENALTY = "prefixMatchPenalty";
    private static final String CACHE_SIZE = "cacheSize";
    private static final String ROWS = "rows";
    private static final String FIELD_BASED_SEARCH = "fieldBasedSearch";
    private static final String SEARCHER_TYPE = "searcherType";
    private static final String QUERY_TERM_FUZZY_TYPE = "queryTermFuzzyType";
    private static final String QUERY_TERM_PREFIX_TYPE = "queryTermPrefixType";
    private static final String RESPONSE_FORMAT = "responseFormat";
    private static final String INDEX_NAME = "name";
    private static final String DATA_FILE = "dataFile";
    private static final String DATA_DIR = "dataDir";
    private static final String DATA_SOURCE_TYPE = "dataSourceType";
    private static final String ACCESS_LOG_FILE = "accessLogFile";
    private static final String INDEX_TYPE = "indexType";
    private static final String SUPPORT_SWAP_IN_EDIT_DISTANCE = "supportSwapInEditDistance";
    private static final String DEFAULT_QUERY_TERM_BOOST = "defaultQueryTermBoost";
    private static final String ENABLE_POSITION_INDEX = "enablePositionIndex";
    private static final String FIELD_BOOST = "fieldBoost";
    private static final String MERGE_EVERY_N_SECONDS = "mergeEveryNSeconds";
    private static final String MERGE_EVERY_M_WRITES = "mergeEveryMWrites";
    private static final String MAX_DOCS = "maxDocs";
    private static final String MAX_MEMORY = "maxMemory";
    private static final String LOG_LEVEL = "logLevel";

    // DEFAULT VALUES
    private static final String DEFAULT_VALUE_RecordScoreExpression = "idf_score*doc_boost";
    private static final String DEFAULT_VALUE_fuzzyMatchPenalty = "0.9";
    private static final String DEFAULT_VALUE_queryTermSimilarityThreshold = "0.65";
    private static final String DEFAULT_VALUE_prefixMatchPenalty = "0.95";
    private static final String DEFAULT_VALUE_cacheSize = "65536000";
    private static final String DEFAULT_VALUE_rows = "10";
    private static final String DEFAULT_VALUE_fieldBasedSearch = "1";
    private static final String DEFAULT_VALUE_searcherType = "0";
    private static final String DEFAULT_VALUE_queryTermFuzzyType = "0";
    private static final String DEFAULT_VALUE_queryTermPrefixType = "1";
    private static final String DEFAULT_VALUE_responseFormat = "1";
    private static final String DEFAULT_VALUE_dataFile = "empty-data.json";
    private static final String DEFAULT_VALUE_dataSourceType = "0";
    private static final String DEFAULT_VALUE_supportSwapInEditDistance = "true";
    private static final String DEFAULT_VALUE_defaultQueryTermBoost = "1";
    private static final String DEFAULT_VALUE_enablePositionIndex = "1";
    private static final String DEFAULT_VALUE_mergeEveryNSeconds = "60";
    private static final String DEFAULT_VALUE_mergeEveryMWrites = "1";
    private static final String DEFAULT_VALUE_maxDocs = "15000000";
    private static final String DEFAULT_VALUE_maxMemory = "10000000";
    private static final String DEFAULT_VALUE_logLevel = "3";
    private static final String DEFAULT_VALUE_accessLogFile = "srch2-log.txt";
    String name;
    Schema schema;
    private Properties queryProperties = new Properties();
    private Properties miscProperties = new Properties();
    private Properties indexProperties = new Properties();
    private Properties updateProperties = new Properties();

    IndexDescription(String name, Schema schema) {
        if (name == null) {
            throw new NullPointerException(
                    "Index description must be initialized with a name.");
        }
        if (name.length() == 0) {
            throw new IllegalArgumentException(
                    "Index description must be initialized with a non-empty string.");
        }
        this.name = name;

        this.schema = schema;

        setQueryProperties();
        setMiscProperties();
        setIndexProperties();
        setUpdateProperties();
    }

    /**
     * Creates the necessary configuration for the index that the <code>Indexable</code>
     * implementation represents. Should only be constructed when returning from the
     * <code>Indexable</code> method <code>getIndexDescription()</code>.
     * <br><br>
     * The first argument <b>should always</b> be the name of the index. The value of this can
     * be used to reference the index when the static method calls of the <code>SRCH2Engine</code>
     * are used to request a task on an index (instead of using the non-static methods of the
     * <code>Indexable</code>); or it can also be used to identify which set of search results
     * belong to which indexes when the <code>SearchResultsListener</code> method
     * <code>onNewSearchResultsAvailable()</code> is triggered. Thus <i>it is recommended</i> that
     * this value be kept in a constant field in the <code>Indexable</code> class implementation.
     * <br><br>
     * The second argument <b>should always</b> be the primary key field, which can be used to
     * retrieve a specific record or as the handle to delete the record from the index. Each
     * record <b>should have a unique value</b> for its primary key.
     * <br><br>
     * The remaining set of arguments are the rest of the schema's fields as they are defined
     * for the index. They can be passed in any order.
     * <br><br>
     * This method will throw exceptions if <code>name</code> is null or empty; or if
     * any of the fields passed are null.
     * @param name            the value to assign to name the index
     * @param primaryKeyField the field which will be the primary key of the index's schema
     * @param remainingField  the set of any other fields needed to define the schema
     */
    public IndexDescription(String name, Field primaryKeyField,
                            Field... remainingField) {
        this(name, new Schema(primaryKeyField, remainingField));
    }

    /**
     * Creates the necessary configuration for the index that the <code>Indexable</code>
     * implementation represents for an index that includes geo-search capability. Should
     * only be constructed when returning from the <code>Indexable</code> method
     * <code>getIndexDescription()</code>.
     * <br><br>
     * The first argument <b>should always</b> be the name of the index. The value of this can
     * be used to reference the index when the static method calls of the <code>SRCH2Engine</code>
     * are used to request a task on an index (instead of using the non-static methods of the
     * <code>Indexable</code>); or it can also be used to identify which set of search results
     * belong to which indexes when the <code>SearchResultsListener</code> method
     * <code>onNewSearchResultsAvailable()</code> is triggered. Thus <i>it is recommended</i> that
     * this value be kept in a constant field in the <code>Indexable</code> class implementation.
     * <br><br>
     * The second argument <b>should always</b> be the primary key field, which can be used to
     * retrieve a specific record or as the handle to delete the record from the index. Each
     * record <b>should have a unique value</b> for its primary key.
     * <br><br>
     * The third and fourth arguments <b>should always</b> be the latitude and longitude
     * fields, in that order, that are defined for the index's schema.
     * The remaining set of arguments are the rest of the schema's fields as they are defined
     * for the index. They can be passed in any order.
     * <br><br>
     * This method will throw exceptions if <code>name</code> is null or empty; or if
     * any of the fields passed are null.
     * @param name            the value to assign to name the index
     * @param primaryKeyField the field which will be the primary key of the index's schema
     * @param latitudeFieldName the field which will be the latitude field of the index's schema
     * @param longitudeFieldName the field which will be the longitude field of the index's schema
     * @param remainingField  the set of any other fields needed to define the schema
     */
    public IndexDescription(String name, Field primaryKeyField,
                            String latitudeFieldName, String longitudeFieldName,
                            Field... remainingField) {
        this(name, new Schema(primaryKeyField, latitudeFieldName,
                longitudeFieldName, remainingField));
    }

    boolean isGeoIndex() {
        return schema.indexType > 0;
    }

    void setQueryProperties() {
        queryProperties.setProperty("recordScoreExpression",
                DEFAULT_VALUE_RecordScoreExpression);
        queryProperties.setProperty("fuzzyMatchPenalty",
                DEFAULT_VALUE_fuzzyMatchPenalty);
        queryProperties.setProperty("queryTermSimilarityThreshold",
                DEFAULT_VALUE_queryTermSimilarityThreshold);
        queryProperties.setProperty("prefixMatchPenalty",
                DEFAULT_VALUE_prefixMatchPenalty);
        queryProperties.setProperty("cacheSize", DEFAULT_VALUE_cacheSize);
        queryProperties.setProperty("rows", DEFAULT_VALUE_rows);
        queryProperties.setProperty("fieldBasedSearch",
                DEFAULT_VALUE_fieldBasedSearch);
        queryProperties.setProperty("searcherType", DEFAULT_VALUE_searcherType);
        queryProperties.setProperty("queryTermFuzzyType",
                DEFAULT_VALUE_queryTermFuzzyType);
        queryProperties.setProperty("queryTermPrefixType",
                DEFAULT_VALUE_queryTermPrefixType);
        queryProperties.setProperty("responseFormat",
                DEFAULT_VALUE_responseFormat);
    }

    float getQueryTermSimilarityThreshold() {
        return Float.parseFloat(queryProperties.getProperty("queryTermSimilarityThreshold"));
    }

    /**
     * Set the default fuzziness similarity threshold. This will determine how many character
     * substitutions the original search input will match search results for: if set to 0.5,
     * the search performed will include results as if half of the characters of the original
     * search input were replaced by wild card characters.
     * <br><br>
     * <b>Note:</b> In the the formation of a <code>Query</code>, each <code>Term</code> can
     * have its own fuzziness similarity threshold value set by calling the method
     * <code>enableFuzzyMatching(Float value)</code>; by default it is disabled for terms.
     * <br><br>
     * This will throw an <code>IllegalArgumentException</code> if the value of
     * <code>threshold</code> is less than zero or greater than one.
     * @param threshold the similarity ratio to match against
     */
    public void setQueryTermSimilarityThreshold(float threshold) {
        if (threshold < 0 || threshold > 1) {
            throw new IllegalArgumentException("The threshold should between (0,1)");
        }
        queryProperties.setProperty("queryTermSimilarityThreshold",
                String.valueOf(threshold));
    }

    int getTopK() {
        return Integer.parseInt(queryProperties.getProperty("rows"));
    }

    /**
     * Sets the number of search results to be returned per query or search task.
     * <br><br>
     * This method will throw an <code>IllegalArgumentException</code> if the value of
     * <code>numberOfResultsToReturn</code> is less than one
     * @param numberOfResultsToReturn the number of results to return per search
     */
    public void setTopK(int numberOfResultsToReturn) {
        if (numberOfResultsToReturn < 1) {
            throw new IllegalArgumentException("The topK can not be negative number");
        }
        queryProperties.setProperty("rows", String.valueOf(numberOfResultsToReturn));
    }

    private void setMiscProperties() {
        miscProperties.setProperty("name", this.name);
        miscProperties.setProperty("dataFile", DEFAULT_VALUE_dataFile);
        miscProperties.setProperty("dataDir", name);
        miscProperties.setProperty("dataSourceType",
                DEFAULT_VALUE_dataSourceType);
    }

    private void setIndexProperties() {
        indexProperties.setProperty("indexType",
                Integer.toString(schema.indexType));
        indexProperties.setProperty("supportSwapInEditDistance",
                DEFAULT_VALUE_supportSwapInEditDistance);
        indexProperties.setProperty("defaultQueryTermBoost",
                DEFAULT_VALUE_defaultQueryTermBoost);
        indexProperties.setProperty("enablePositionIndex",
                DEFAULT_VALUE_enablePositionIndex);
        indexProperties.setProperty("fieldBoost", getBoostStatementString());

    }

    private void setUpdateProperties() {
        updateProperties.setProperty("mergeEveryNSeconds",
                DEFAULT_VALUE_mergeEveryNSeconds);
        updateProperties.setProperty("mergeEveryMWrites",
                DEFAULT_VALUE_mergeEveryMWrites);
        updateProperties.setProperty("maxDocs", DEFAULT_VALUE_maxDocs);
        updateProperties.setProperty("maxMemory", DEFAULT_VALUE_maxMemory);
        updateProperties.setProperty("logLevel", DEFAULT_VALUE_logLevel);
        updateProperties.setProperty("accessLogFile",
                DEFAULT_VALUE_accessLogFile);

    }

    /**
     * This function configures the parameter on how often the engine will merge
     * the data.
     *
     * @param mergeEveryNSeconds This parameter sets the number of seconds the background
     *                           thread sleeps before it wakes up and does the merge. This
     *                           number should be at least 1 (second).
     * @param mergeEveryMWrites  This parameter specifies the number of record changes after
     *                           which the thread will awaken to do the merge. This number
     *                           should be at least 1.
     */
    void setMergeProperties(int mergeEveryNSeconds, int mergeEveryMWrites) {
        updateProperties.setProperty("mergeEveryNSeconds",
                String.valueOf(mergeEveryNSeconds));
        updateProperties.setProperty("mergeEveryMWrites",
                String.valueOf(mergeEveryMWrites));
    }

    String getBoostStatementString() {
        Iterator<Field> iter = this.schema.fields.iterator();
        String boostValue = "";
        while (iter.hasNext()) {
            Field f = iter.next();
            if (f.searchable)
                boostValue = boostValue + f.name + "^" + f.boost + " ";
        }
        return boostValue;
    }

    String indexStructureToXML() {

        StringBuilder core = new StringBuilder("");

        core = core
                .append(" <core name=\"")
                .append(miscProperties.getProperty(INDEX_NAME))
                .append("\">\n")
                .append("            <dataFile>")
                .append(miscProperties.getProperty(DATA_FILE))
                .append("</dataFile>\n")
                .append("            <dataDir>")
                .append(miscProperties.getProperty(DATA_DIR))
                .append("</dataDir>\n")
                .append("            <dataSourceType>")
                .append(miscProperties.getProperty(DATA_SOURCE_TYPE))
                .append("</dataSourceType>\n")
                .append("            <indexConfig>\n")
                .append("                <indexType>")
                .append(indexProperties.getProperty(INDEX_TYPE))
                .append("</indexType>\n")
                .append("                <supportSwapInEditDistance>")
                .append(indexProperties
                        .getProperty(SUPPORT_SWAP_IN_EDIT_DISTANCE))
                .append("</supportSwapInEditDistance>\n")
                .append("                <fieldBoost>")
                .append(indexProperties.getProperty(FIELD_BOOST))
                .append("</fieldBoost>\n")
                .append("                <defaultQueryTermBoost>")
                .append(indexProperties.getProperty(DEFAULT_QUERY_TERM_BOOST))
                .append("</defaultQueryTermBoost>\n")
                .append("                <enablePositionIndex>")
                .append(indexProperties.getProperty(ENABLE_POSITION_INDEX))
                .append("</enablePositionIndex>\n")
                .append("            </indexConfig>\n")
                .append("            <query>\n")
                .append("                <rankingAlgorithm>\n")
                .append("                    <recordScoreExpression>")
                .append(queryProperties.getProperty(RECORD_SCORE_EXPRESSION))
                .append("</recordScoreExpression>\n")
                .append("                </rankingAlgorithm>\n")
                .append("                <fuzzyMatchPenalty>")
                .append(queryProperties.getProperty(FUZZY_MATCH_PENALTY))
                .append("</fuzzyMatchPenalty>\n")
                .append("                <queryTermSimilarityThreshold>")
                .append(queryProperties
                        .getProperty(QUERY_TERM_SIMILARITY_THRESHOLD))
                .append("</queryTermSimilarityThreshold>\n")
                .append("                <prefixMatchPenalty>")
                .append(queryProperties.getProperty(PREFIX_MATCH_PENALTY))
                .append("</prefixMatchPenalty>\n")
                .append("                <cacheSize>")
                .append(queryProperties.getProperty(CACHE_SIZE))
                .append("</cacheSize>\n")
                .append("                <rows>")
                .append(queryProperties.getProperty(ROWS))
                .append("</rows>\n")
                .append("                <fieldBasedSearch>")
                .append(queryProperties.getProperty(FIELD_BASED_SEARCH))
                .append("</fieldBasedSearch>\n")
                .append("                <searcherType>")
                .append(queryProperties.getProperty(SEARCHER_TYPE))
                .append("</searcherType>\n")
                .append("\n")
                .append("                <!-- 0: exact match; 1: fuzzy match.-->\n")
                .append("                <queryTermFuzzyType>")
                .append(queryProperties.getProperty(QUERY_TERM_FUZZY_TYPE))
                .append("</queryTermFuzzyType>\n")
                .append("\n")
                .append("                <!-- 0: prefix match; 1: complete match -->\n")
                .append("                <queryTermPrefixType>")
                .append(queryProperties.getProperty(QUERY_TERM_PREFIX_TYPE))
                .append("</queryTermPrefixType>\n").append("\n")
                .append("                <queryResponseWriter>\n")
                .append("                    <responseFormat>")
                .append(queryProperties.getProperty(RESPONSE_FORMAT))
                .append("</responseFormat>\n")
                .append("                </queryResponseWriter>\n")
                .append("            </query>\n");
        core.append(schema.schemaToXML());
        core.append("	  <updatehandler>\n").append("                <maxDocs>")
                .append(updateProperties.getProperty(MAX_DOCS))
                .append("</maxDocs>\n").append("                <maxMemory>")
                .append(updateProperties.getProperty(MAX_MEMORY))
                .append("</maxMemory>\n")
                .append("                <mergePolicy>\n")
                .append("                    <mergeEveryNSeconds>")
                .append(updateProperties.getProperty(MERGE_EVERY_N_SECONDS))
                .append("</mergeEveryNSeconds>\n")
                .append("                    <mergeEveryMWrites>")
                .append(updateProperties.getProperty(MERGE_EVERY_M_WRITES))
                .append("</mergeEveryMWrites>\n")
                .append("                </mergePolicy>\n")
                .append("                <updateLog>\n")
                .append("                    <logLevel>")
                .append(updateProperties.getProperty(LOG_LEVEL))
                .append("</logLevel>\n")
                .append("                    <accessLogFile>")
                .append(updateProperties.getProperty(ACCESS_LOG_FILE))
                .append("</accessLogFile>\n")
                .append("                </updateLog>\n")
                .append("            </updatehandler>\n")
                .append("        </core>\n");

        return core.toString();
    }

    /**
     * Gets the name of the index this <code>Indexable</code> represents as it was set in the
     * <code>IndexDescription</code> returned in the <code>Indexable</code> implementation of
     * <code>getIndexDescription()</code>. This can be used to call the static CRUD methods of
     * the <code>SRCH2Engine</code> and used to identify which index the two callbacks
     * <code>StateResponseListener</code> and <code>SearchResultsListener</code> refer to in
     * their callback methods.
     * @return the name of the index this <code>Indexable</code> represents
     */
    public String getIndexName() {
        return name;
    }
}
