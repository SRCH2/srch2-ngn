package com.srch2.android.http.service;

import java.util.Iterator;
import java.util.Properties;

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
     * implementation represents. 
     * <br><br>
     * The first argument <b>should always</b> be the name of the index, which can be used to
     * reference the index when the static method calls of the <code>SRCH2Engine</code> are used
     * to access an index (instead of using the non-static methods of the <code>Indexable</code>).
     * <br><br>
     * The second argument <b>should always</b> be the primary key field,
     *
     *
     * and rest of the argument are of type Field.
     * It accepts an array of fields, and the first field is used as the primary key field.
     * Each entry of the primary key field should be unique.
     *
     * @param name            the name of the Index
     * @param primaryKeyField the primaryKey {@link Field}
     * @param remainingField  other Fields, could be empty
     */
    public IndexDescription(String name, Field primaryKeyField,
                            Field... remainingField) {
        this(name, new Schema(primaryKeyField, remainingField));
    }

    /**
     * Creates a description of the GeoLocation <code>Index</code>
     * Notice that first argument is the name of the index, second the primary key field, third and fourth
     * are strings denoting the name of latitude and longitude. And remaining variable arguments are of
     * type Field.
     *
     * @param name               the name of the Index
     * @param primaryKeyField    the primaryKey {@link Field}
     * @param latitudeFieldName  the name of the latitudeField
     * @param longitudeFieldName the name of the longitudeField
     * @param remainingField     other Fields, could be empty
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
     * Set the default fuzziness similarity threshold. The Query string will take this fuzziness setting.
     * User can use {@link com.srch2.android.http.service.Query} to enable the query specific similarity setting.
     *
     * @param threshold
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
     * It sets number of results that has to be returned by a query.
     *
     * @param rows specifies number of results on querying.
     */
    public void setTopK(int rows) {
        if (rows < 0) {
            throw new IllegalArgumentException("The topK can not be negative number");
        }
        queryProperties.setProperty("rows", String.valueOf(rows));
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

    protected String getBoostStatementString() {
        Iterator<Field> iter = this.schema.fields.iterator();
        String boostValue = "";
        while (iter.hasNext()) {
            Field f = iter.next();
            if (f.searchable)
                boostValue = boostValue + f.name + "^" + f.boost + " ";
        }
        return boostValue;
    }

    protected String indexStructureToXML() {

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
     * Gets the name tag of the index.
     *
     * @return name of the index.
     */
    public String getIndexName() {
        return name;
    }
}
