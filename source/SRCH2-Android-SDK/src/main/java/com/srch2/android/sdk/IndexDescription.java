package com.srch2.android.sdk;

import java.util.Iterator;
import java.util.Properties;


final class IndexDescription {

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
    private static final String RECORD_BOOST_FIELD = "recordBoostField";
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

    private final Properties queryProperties = new Properties();
    private final Properties miscProperties = new Properties();
    private final Properties indexProperties = new Properties();
    private final Properties updateProperties = new Properties();

    String name;
    Schema schema;

    IndexDescription(Indexable idx) {

        name = idx.getIndexName();

        schema = idx.getSchema();

        queryProperties.setProperty("rows", String.valueOf(idx.getTopK()));

        queryProperties.setProperty("queryTermSimilarityThreshold",
                String.valueOf(idx.getFuzzinessSimilarityThreshold()));

        setQueryProperties();
        setMiscProperties();
        setIndexProperties();
        setUpdateProperties();
    }
/*

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


    IndexDescription(String theIndexName, Schema theSchema) {
        this(theIndexName, theSchema);
    }


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

    IndexDescription(String name, PrimaryKeyField primaryKeyField,
                            String latitudeFieldName, String longitudeFieldName,
                            Field... remainingField) {
        this(name, new Schema(primaryKeyField, latitudeFieldName,
                longitudeFieldName, remainingField));
    }
    */
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

    int getTopK() {
        return Integer.parseInt(queryProperties.getProperty("rows"));
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
        if (schema.recordBoostKey != null) {
            indexProperties.setProperty("recordBoostField", schema.recordBoostKey);
        }
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

    void setMergeProperties(int mergeEveryNSeconds, int mergeEveryMWrites) {
        updateProperties.setProperty("mergeEveryNSeconds",
                String.valueOf(mergeEveryNSeconds));
        updateProperties.setProperty("mergeEveryMWrites",
                String.valueOf(mergeEveryMWrites));
    }

    String getBoostStatementString() {
        Iterator<Field> iter = this.schema.fields.iterator();
        StringBuilder sb = new StringBuilder();
        while (iter.hasNext()) {
            Field f = iter.next();
            if (f.searchable) {
                sb.append(f.name);
                sb.append("^");
                sb.append(f.boost);
                sb.append(" ");
            }
        }
        return sb.substring(0, sb.length() - 1);
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
                .append("</fieldBoost>\n");
                // temporary fix since engine will crash if this is empty (ie has not been set by user)
                if (schema.recordBoostKey != null) {
                    core.append("                <recordBoostField>")
                            .append(indexProperties.getProperty(RECORD_BOOST_FIELD))
                            .append("</recordBoostField>\n");
                }
                core.append("                <defaultQueryTermBoost>")
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
        core.append(schemaToXml());
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


    String getIndexName() {
        return name;
    }

    String schemaToXml() {
        StringBuilder schemaXML = new StringBuilder("	<schema>\n"
                + "		<fields>\n");
        for (Field field : schema.fields) {
            schemaXML.append(Field.toXML(field));
        }
        schemaXML.append("		</fields>\n").append("		<uniqueKey>")
                .append(schema.uniqueKey).append("</uniqueKey>\n");

        schemaXML
                .append(facetToXML())
                .append("		<types>\n")
                .append("		  <fieldType name=\"text_en\">\n")
                .append("			<analyzer>\n")
                .append("				<filter name=\"PorterStemFilter\" dictionary=\"\" />\n")
                .append("				<filter name=\"StopFilter\" words=\"stop-words.txt\" />\n")
                .append("			</analyzer>\n").append("		  </fieldType>\n")
                .append("		</types>\n").append("	</schema>\n");

        return schemaXML.toString();

    }

    String facetToXML() {

        StringBuilder facetNodeXML = new StringBuilder("		<facetEnabled>")
                .append(schema.facetEnabled).append("</facetEnabled>\n");
        if (schema.facetEnabled) {
            Iterator<Field> iter = schema.fields.iterator();
            StringBuilder facetFieldsXML = new StringBuilder("");
            while (iter.hasNext()) {
                Field f = iter.next();
                if (f.facetEnabled) {
                    schema.facetEnabled = true;
                    switch (f.facetType) {
                        case CATEGORICAL:
                            facetFieldsXML.append("			<facetField name=\"")
                                    .append(f.name)
                                    .append("\" facetType=\"categorical\"/>\n");
                            break;
                        case RANGE:
                        default:
                            facetFieldsXML.append("			<facetField name=\"")
                                    .append(f.name)
                                    .append("\" facetType=\"range\" facetStart=\"")
                                    .append(f.facetStart).append("\" facetEnd=\"")
                                    .append(f.facetEnd).append("\"")
                                    .append(" facetGap=\"").append(f.facetGap)
                                    .append("\"/>\n");
                    }
                }
            }
            facetNodeXML = facetNodeXML.append("		<facetFields>\n")
                    .append(facetFieldsXML).append("		</facetFields>\n");
        }

        return facetNodeXML.toString();
    }


    static void throwIfNonValidTopK(int topK) {
        if (topK < 1) {
            throw new IllegalArgumentException("The number of results to return per search per index, AKA topK, must be greater to or equal to one.");
        }
    }

    static void throwIfNonValidFuzzinessSimilarityThreshold(float threshold) {
        if (threshold < 0 || threshold > 1) {
            throw new IllegalArgumentException("The fuzziness similarity threshold should between greater than zero or less than one.");
        }
    }

    static void throwIfNonValidIndexName(String indexName) {
        if (indexName == null) {
            throw new NullPointerException(
                    "The name of the index cannot be null.");
        }
        if (indexName.length() == 0) {
            throw new IllegalArgumentException(
                    "The name of the index must be a non-empty string");
        }
    }


}
