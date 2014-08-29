package com.srch2.android.sdk;

import java.util.Iterator;
import java.util.Properties;


final class IndexDescription {

    static final String tMinus = "    ";
    static final String t = "        ";
    static final String tt = "            ";
    static final String ttt = "                ";
    static final String tttt = "                    ";

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
    Highlighter highlighter;

    String getIndexName() {
        return name;
    }

    IndexDescription(Indexable idx) {
        name = idx.getIndexName();
        schema = idx.getSchema();

        highlighter = idx.getHighlighter();
        highlighter.configureHighlightingForIndexDescription();

        queryProperties.setProperty("rows", String.valueOf(idx.getTopK()));
        queryProperties.setProperty("queryTermSimilarityThreshold",
                String.valueOf(idx.getFuzzinessSimilarityThreshold()));

        setQueryProperties();
        setMiscProperties();
        setIndexProperties();
        setUpdateProperties();
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
        queryProperties.setProperty("fuzzyPreTag",
                highlighter.highlightFuzzyPreTag);
        queryProperties.setProperty("fuzzyPostTag",
                highlighter.highlightFuzzyPostTag);
        queryProperties.setProperty("exactPreTag",
                highlighter.highlightExactPreTag);
        queryProperties.setProperty("exactPostTag",
                highlighter.highlightExactPostTag);
    }

    private void setMiscProperties() {
        miscProperties.setProperty("name", this.name);
        miscProperties.setProperty("dataFile", DEFAULT_VALUE_dataFile);
        miscProperties.setProperty("dataDir", name);
        miscProperties.setProperty("dataSourceType",
                DEFAULT_VALUE_dataSourceType);
    }

    private void setIndexProperties() {
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
        StringBuilder core = new StringBuilder()
            .append(t)
                .append("<core name=\"")
                    .append(miscProperties.getProperty(INDEX_NAME))
                .append("\">\n")
            .append(tt)
                .append("<dataFile>")
                    .append(miscProperties.getProperty(DATA_FILE))
                .append("</dataFile>\n")
            .append(tt)
                .append("<dataDir>")
                    .append(miscProperties.getProperty(DATA_DIR))
                .append("</dataDir>\n")
            .append(tt)
                .append("<dataSourceType>")
                    .append(miscProperties.getProperty(DATA_SOURCE_TYPE))
                .append("</dataSourceType>\n\n")

/* INDEX CONFIGURATION NODE START */
            .append(tt)
                .append("<indexConfig>\n")
            .append(ttt)
                .append("<supportSwapInEditDistance>")
                    .append(indexProperties.getProperty(SUPPORT_SWAP_IN_EDIT_DISTANCE))
                .append("</supportSwapInEditDistance>\n")
            .append(ttt)
                .append("<fieldBoost>")
                    .append(indexProperties.getProperty(FIELD_BOOST))
                .append("</fieldBoost>\n");

        if (schema.recordBoostKey != null) {
            core
            .append(ttt)
                .append("<recordBoostField>")
                    .append(indexProperties.getProperty(RECORD_BOOST_FIELD))
                .append("</recordBoostField>\n");
        }
        core
            .append(ttt)
                .append("<defaultQueryTermBoost>")
                    .append(indexProperties.getProperty(DEFAULT_QUERY_TERM_BOOST))
                .append("</defaultQueryTermBoost>\n")
            .append(ttt)
                .append("<enablePositionIndex>")
                    .append(indexProperties.getProperty(ENABLE_POSITION_INDEX))
                .append("</enablePositionIndex>\n")
            .append(tt)
                .append("</indexConfig>\n\n")
/* INDEX CONFIGURATION NODE END */
/* QUERY CONFIGURATION NODE START */
            .append(tt)
                .append("<query>\n")
            .append(ttt)
                .append("<rankingAlgorithm>\n")
            .append(tttt)
                .append("<recordScoreExpression>")
                    .append(queryProperties.getProperty(RECORD_SCORE_EXPRESSION))
                .append("</recordScoreExpression>\n")
            .append(ttt)
                .append("</rankingAlgorithm>\n")
            .append(ttt)
                .append("<fuzzyMatchPenalty>")
                    .append(queryProperties.getProperty(FUZZY_MATCH_PENALTY))
                .append("</fuzzyMatchPenalty>\n")
            .append(ttt)
                .append("<queryTermSimilarityThreshold>")
                    .append(queryProperties.getProperty(QUERY_TERM_SIMILARITY_THRESHOLD))
                .append("</queryTermSimilarityThreshold>\n")
            .append(ttt)
                .append("<prefixMatchPenalty>")
                    .append(queryProperties.getProperty(PREFIX_MATCH_PENALTY))
                .append("</prefixMatchPenalty>\n")
            .append(ttt)
                .append("<cacheSize>")
                    .append(queryProperties.getProperty(CACHE_SIZE))
                .append("</cacheSize>\n")
            .append(ttt)
                .append("<rows>")
                    .append(queryProperties.getProperty(ROWS))
                .append("</rows>\n")
            .append(ttt)
                .append("<fieldBasedSearch>")
                    .append(queryProperties.getProperty(FIELD_BASED_SEARCH))
                .append("</fieldBasedSearch>\n")
            .append(ttt)
                .append("<searcherType>")
                    .append(queryProperties.getProperty(SEARCHER_TYPE))
                .append("</searcherType>\n")
            .append(ttt)
                .append("<queryTermFuzzyType>")
                    .append(queryProperties.getProperty(QUERY_TERM_FUZZY_TYPE))
                .append("</queryTermFuzzyType>\n")
            .append(ttt)
                .append("<queryTermPrefixType>")
                    .append(queryProperties.getProperty(QUERY_TERM_PREFIX_TYPE))
                .append("</queryTermPrefixType>\n")
            .append(ttt)
                .append("<queryResponseWriter>\n")
            .append(tttt)
                .append("<responseFormat>")
                    .append(queryProperties.getProperty(RESPONSE_FORMAT))
                .append("</responseFormat>\n")
            .append(ttt)
                .append("</queryResponseWriter>\n")
            .append(ttt)
                .append("<highlighter>\n")
            .append(tttt)
                .append("<snippetSize>")
                    .append(250)
                .append("</snippetSize>\n")
            .append(tttt)
                .append("<fuzzyTagPre value = \'")
                    .append(queryProperties.get("fuzzyPreTag"))
                .append("\'></fuzzyTagPre>\n")
            .append(tttt)
                .append("<fuzzyTagPost value = \'")
                    .append(queryProperties.get("fuzzyPostTag"))
                .append("\'></fuzzyTagPost>\n")
            .append(tttt)
                .append("<exactTagPre value = \'")
                    .append(queryProperties.get("exactPreTag"))
                .append("\'></exactTagPre>\n")
            .append(tttt)
                .append("<exactTagPost value = \'")
                    .append(queryProperties.get("exactPostTag"))
                .append("\'></exactTagPost>\n")
            .append(ttt)
                .append("</highlighter>\n")
            .append(tt)
                .append("</query>\n\n");
/* QUERY CONFIGURATION NODE END */

/* SCHEMA CONFIGURATION NODE START */
        core
            .append(schemaToXml());
/* SCHEMA CONFIGURATION NODE END */

/* UPDATE CONFIGURATION NODE START */
        core
            .append(tt)
                .append("<updatehandler>\n")
            .append(ttt)
                .append("<maxDocs>")
                    .append(updateProperties.getProperty(MAX_DOCS))
                .append("</maxDocs>\n")
            .append(ttt)
                .append("<maxMemory>")
                    .append(updateProperties.getProperty(MAX_MEMORY))
                .append("</maxMemory>\n")
            .append(ttt)
                .append("<mergePolicy>\n")
            .append(tttt)
                .append("<mergeEveryNSeconds>")
                    .append(updateProperties.getProperty(MERGE_EVERY_N_SECONDS))
                .append("</mergeEveryNSeconds>\n")
            .append(tttt)
                .append("<mergeEveryMWrites>")
                    .append(updateProperties.getProperty(MERGE_EVERY_M_WRITES))
                .append("</mergeEveryMWrites>\n")
            .append(ttt)
                .append("</mergePolicy>\n")
            .append(ttt)
                .append("<updateLog>\n")
            .append(tttt)
                .append("<logLevel>")
                    .append(updateProperties.getProperty(LOG_LEVEL))
                .append("</logLevel>\n")
            .append(tttt)
                .append("<accessLogFile>")
                    .append(updateProperties.getProperty(ACCESS_LOG_FILE))
                .append("</accessLogFile>\n")
            .append(ttt)
                .append("</updateLog>\n")
            .append(tt)
                .append("</updatehandler>\n")
/* UPDATE CONFIGURATION NODE END */
            .append(t)
                .append("</core>\n");
        return core.toString();
    }

    String schemaToXml() {
        StringBuilder schemaXML = new StringBuilder()
            .append(tt)
                .append("<schema>\n")
            .append(ttt)
                .append("<fields>\n");

        for (Field field : schema.fields) {
            schemaXML.append(Field.toXML(field));
        }
        schemaXML
            .append(ttt)
                .append("</fields>\n")
            .append(ttt)
                .append("<uniqueKey>")
                    .append(schema.uniqueKey)
                .append("</uniqueKey>\n")
/* ADDING FACETS */
                .append(facetToXML())
/* DEFAULT CONFIGURATION FOR SCHEMA */
            .append(ttt)
                .append("<types>\n")
            .append(tttt)
                .append("<fieldType name=\"text_en\">\n")
            .append(tttt).append(tMinus)
                .append("<analyzer>\n")
            .append(tttt).append(t)
                .append("<filter name=\"PorterStemFilter\" dictionary=\"\" />\n")
            .append(tttt).append(t)
                .append("<filter name=\"StopFilter\" words=\"stop-words.txt\" />\n")
            .append(tttt).append(tMinus)
                .append("</analyzer>\n")
            .append(tttt)
                .append("</fieldType>\n")
            .append(ttt)
                .append("</types>\n")
            .append(tt)
                .append("</schema>\n\n");
        return schemaXML.toString();
    }

    String facetToXML() {
        StringBuilder facetNodeXML = new StringBuilder()
            .append(ttt)
                .append("<facetEnabled>")
                    .append(schema.facetEnabled)
                .append("</facetEnabled>\n");

        if (schema.facetEnabled) {
            Iterator<Field> iter = schema.fields.iterator();
            StringBuilder facetFieldsXML = new StringBuilder("");
            while (iter.hasNext()) {
                Field f = iter.next();
                if (f.facetEnabled) {
                    schema.facetEnabled = true;
                    switch (f.facetType) {
                        case CATEGORICAL:
                            facetFieldsXML
                                    .append(tttt).append(t)
                                        .append("<facetField name=\"")
                                            .append(f.name)
                                    .append("\" facetType=\"categorical\"/>\n");
                            break;
                        case RANGE:
                        default:
                            facetFieldsXML
                                    .append(tttt).append(t)
                                        .append("<facetField name=\"")
                                        .append(f.name)
                                        .append("\" facetType=\"range\" facetStart=\"")
                                        .append(f.facetStart).append("\" facetEnd=\"")
                                        .append(f.facetEnd).append("\"")
                                        .append(" facetGap=\"").append(f.facetGap)
                                        .append("\"/>\n");
                    }
                }
            }
            facetNodeXML
                    .append(tttt)
                        .append("<facetFields>\n")
                            .append(facetFieldsXML)
                    .append(tttt)
                        .append("</facetFields>\n");
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