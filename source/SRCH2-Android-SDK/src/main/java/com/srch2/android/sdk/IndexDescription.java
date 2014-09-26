package com.srch2.android.sdk;

import java.util.Iterator;
import java.util.Properties;


final class IndexDescription {

    static final String TAB_SINGLE = "    ";
    static final String TAB_DOUBLE = "        ";
    static final String TAB_TRIPLE = "            ";
    static final String TAB_QUADRUPLE = "                ";
    static final String TAB_QUINTUPLE = "                    ";

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
    private static final String SUPPORT_SWAP_IN_EDIT_DISTANCE = "supportSwapInEditDistance";
    private static final String DEFAULT_QUERY_TERM_BOOST = "defaultQueryTermBoost";
    private static final String ENABLE_POSITION_INDEX = "enablePositionIndex";
    private static final String FIELD_BOOST = "fieldBoost";
    private static final String RECORD_BOOST_FIELD = "recordBoostField";
    private static final String MERGE_EVERY_N_SECONDS = "mergeEveryNSeconds";
    private static final String MERGE_EVERY_M_WRITES = "mergeEveryMWrites";
    private static final String MAX_DOCS = "maxDocs";
    private static final String MAX_MEMORY = "maxMemory";

    static final String DB_SHARED_LIBRARY_PATH = "dbSharedLibraryPath";
    static final String DB_SHARED_LIBRARY_NAME = "dbSharedLibraryName";
    static final String DB_APP_DATABASE_PATH = "dbPath";
    static final String DB_DATABASE_NAME = "db";
    static final String DB_DATABASE_TABLE_NAME = "tableName";
    static final String DB_LISTENER_WAIT_TIME = "listenerWaitTime";
    static final String DB_MAX_RETRY_ON_FAILURE = "maxRetryOnFailure";

    // DEFAULT VALUES
    private static final String DEFAULT_VALUE_RecordScoreExpression = "idf_score*doc_boost";
    private static final String DEFAULT_VALUE_fuzzyMatchPenalty = "0.9";
    private static final String DEFAULT_VALUE_queryTermSimilarityThreshold = "0.65";
    private static final String DEFAULT_VALUE_prefixMatchPenalty = "0.95";
    private static final String DEFAULT_VALUE_cacheSize = "65536000";
    private static final String DEFAULT_VALUE_rows = "10";
    private static final int DEFAULT_VALUE_rows_count = 10;
    private static final String DEFAULT_VALUE_fieldBasedSearch = "1";
    private static final String DEFAULT_VALUE_searcherType = "0";
    private static final String DEFAULT_VALUE_queryTermFuzzyType = "0";
    private static final String DEFAULT_VALUE_queryTermPrefixType = "1";
    private static final String DEFAULT_VALUE_responseFormat = "1";
    private static final String DEFAULT_VALUE_dataFile = "empty-data.json";

    private static final String DEFAULT_VALUE_supportSwapInEditDistance = "true";
    private static final String DEFAULT_VALUE_defaultQueryTermBoost = "1";
    private static final String DEFAULT_VALUE_enablePositionIndex = "1";
    private static final String DEFAULT_VALUE_mergeEveryNSeconds = "60";
    private static final String DEFAULT_VALUE_mergeEveryMWrites = "1";
    private static final String DEFAULT_VALUE_maxDocs = "15000000";
    private static final String DEFAULT_VALUE_maxMemory = "10000000";

    private static final String DEFAULT_VALUE_dataSourceTypeDefault = "0";
    private static final String DEFAULT_VALUE_dataSourceTypeSqlite = "2";



    final Properties queryProperties = new Properties();
    final Properties coreProperties = new Properties();
    final Properties indexProperties = new Properties();
    final Properties updateProperties = new Properties();
    final Properties sqliteDatabaseProperties = new Properties();

    static enum IndexableType {
        Default, // Means this type of SRCH2 index has no connector attached
        Sqlite;
    }

    IndexableType type;
    String name;
    Schema schema;
    Highlighter highlighter;

    String getIndexName() {
        return name;
    }

    private void setGeneralProperties(IndexableCore idx) {
        highlighter = idx.getHighlighter();
        highlighter.configureHighlightingForIndexDescription();

        int topk = idx.getTopK();
        if (topk < 1 || topk > 499) {
            topk = DEFAULT_VALUE_rows_count;
        }

        queryProperties.setProperty("rows", String.valueOf(topk));
        queryProperties.setProperty("queryTermSimilarityThreshold",
                String.valueOf(idx.getFuzzinessSimilarityThreshold()));

        setQueryProperties();
        setMiscProperties();
        setIndexProperties();
        setUpdateProperties();
    }

    IndexDescription(Indexable idx) {
        name = idx.getIndexName();
        schema = idx.getSchema();
        type = IndexableType.Default;
        setGeneralProperties(idx);
    }

    IndexDescription(SQLiteIndexable idx, Schema s) {
        name = idx.getIndexName();
        schema = s;
        type = IndexableType.Sqlite;

        sqliteDatabaseProperties.setProperty(DB_DATABASE_NAME, idx.getDatabaseName());
        sqliteDatabaseProperties.setProperty(DB_DATABASE_TABLE_NAME, idx.getTableName());
        sqliteDatabaseProperties.setProperty(DB_MAX_RETRY_ON_FAILURE, "2");
        // TODO make setable by user
        sqliteDatabaseProperties.setProperty(DB_LISTENER_WAIT_TIME, "3");
        setGeneralProperties(idx);
    }

    void setQueryProperties() {
        queryProperties.setProperty("recordScoreExpression",
                DEFAULT_VALUE_RecordScoreExpression);
        queryProperties.setProperty("fuzzyMatchPenalty",
                DEFAULT_VALUE_fuzzyMatchPenalty);
        queryProperties.setProperty("prefixMatchPenalty",
                DEFAULT_VALUE_prefixMatchPenalty);
        queryProperties.setProperty("cacheSize", DEFAULT_VALUE_cacheSize);
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
        coreProperties.setProperty("name", name);
        switch (type) {
            case Default:
                coreProperties.setProperty(DATA_FILE, DEFAULT_VALUE_dataFile);
                coreProperties.setProperty(DATA_DIR, name);
                coreProperties.setProperty(DATA_SOURCE_TYPE,
                        DEFAULT_VALUE_dataSourceTypeDefault);
                break;
            case Sqlite:
                coreProperties.setProperty("name", name);
                coreProperties.setProperty("dataDir", name);
                coreProperties.setProperty("dataSourceType",
                        DEFAULT_VALUE_dataSourceTypeSqlite);
                break;
        }
    }

    private void setIndexProperties() {
        indexProperties.setProperty("supportSwapInEditDistance",
                DEFAULT_VALUE_supportSwapInEditDistance);
        indexProperties.setProperty("defaultQueryTermBoost",
                DEFAULT_VALUE_defaultQueryTermBoost);
        indexProperties.setProperty("enablePositionIndex",
                DEFAULT_VALUE_enablePositionIndex);
        indexProperties.setProperty("fieldBoost", getBoostStatementString());
        if (schema != null && schema.recordBoostKey != null) {
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


        if (type == IndexableType.Sqlite) {
            sqliteDatabaseProperties.setProperty(DB_SHARED_LIBRARY_PATH, SRCH2Engine.conf.getAppBinDirectory());
            sqliteDatabaseProperties.setProperty(DB_SHARED_LIBRARY_NAME, "sqliteconn");
            sqliteDatabaseProperties.setProperty(DB_APP_DATABASE_PATH, SRCH2Engine.conf.getAppDatabasePath());
        }



        StringBuilder core = new StringBuilder()
            .append(TAB_DOUBLE)
                .append("<core name=\"")
                    .append(coreProperties.getProperty(INDEX_NAME))
                .append("\">\n")/*
            .append(TAB_TRIPLE) ONLY USED FOR JSON LEAVE OUT FOR NOW
                .append("<dataFile>")
                    .append(coreProperties.getProperty(DATA_FILE))
                .append("</dataFile>\n")*/
            .append(TAB_TRIPLE)
                .append("<dataDir>")
                    .append(coreProperties.getProperty(DATA_DIR))
                .append("</dataDir>\n")
            .append(TAB_TRIPLE)
                .append("<dataSourceType>")
                    .append(coreProperties.getProperty(DATA_SOURCE_TYPE))
                .append("</dataSourceType>\n\n");
/* SQLITE CONFIGURATION NODE START */
        if (type == IndexableType.Sqlite) {
            core.append(sqliteDatabaseParametersToXML().toString());
        }
/* SQLITE CONFIGURATION NODE END */
/* INDEX CONFIGURATION NODE START */
            core
            .append(TAB_TRIPLE)
                .append("<indexConfig>\n")
            .append(TAB_QUADRUPLE)
                .append("<supportSwapInEditDistance>")
                    .append(indexProperties.getProperty(SUPPORT_SWAP_IN_EDIT_DISTANCE))
                .append("</supportSwapInEditDistance>\n")
            .append(TAB_QUADRUPLE)
                .append("<fieldBoost>")
                    .append(indexProperties.getProperty(FIELD_BOOST))
                .append("</fieldBoost>\n");

        if (schema != null && schema.recordBoostKey != null) {
            core
            .append(TAB_QUADRUPLE)
                .append("<recordBoostField>")
                    .append(indexProperties.getProperty(RECORD_BOOST_FIELD))
                .append("</recordBoostField>\n");
        }
        core
            .append(TAB_QUADRUPLE)
                .append("<defaultQueryTermBoost>")
                    .append(indexProperties.getProperty(DEFAULT_QUERY_TERM_BOOST))
                .append("</defaultQueryTermBoost>\n")
            .append(TAB_QUADRUPLE)
                .append("<enablePositionIndex>")
                    .append(indexProperties.getProperty(ENABLE_POSITION_INDEX))
                .append("</enablePositionIndex>\n")
            .append(TAB_TRIPLE)
                .append("</indexConfig>\n\n")
/* INDEX CONFIGURATION NODE END */
/* QUERY CONFIGURATION NODE START */
            .append(TAB_TRIPLE)
                .append("<query>\n")
            .append(TAB_QUADRUPLE)
                .append("<rankingAlgorithm>\n")
            .append(TAB_QUINTUPLE)
                .append("<recordScoreExpression>")
                    .append(queryProperties.getProperty(RECORD_SCORE_EXPRESSION))
                .append("</recordScoreExpression>\n")
            .append(TAB_QUADRUPLE)
                .append("</rankingAlgorithm>\n")
            .append(TAB_QUADRUPLE)
                .append("<fuzzyMatchPenalty>")
                    .append(queryProperties.getProperty(FUZZY_MATCH_PENALTY))
                .append("</fuzzyMatchPenalty>\n")
            .append(TAB_QUADRUPLE)
                .append("<queryTermSimilarityThreshold>")
                    .append(queryProperties.getProperty(QUERY_TERM_SIMILARITY_THRESHOLD))
                .append("</queryTermSimilarityThreshold>\n")
            .append(TAB_QUADRUPLE)
                .append("<prefixMatchPenalty>")
                    .append(queryProperties.getProperty(PREFIX_MATCH_PENALTY))
                .append("</prefixMatchPenalty>\n")
            .append(TAB_QUADRUPLE)
                .append("<cacheSize>")
                    .append(queryProperties.getProperty(CACHE_SIZE))
                .append("</cacheSize>\n")
            .append(TAB_QUADRUPLE)
                .append("<rows>")
                    .append(queryProperties.getProperty(ROWS))
                .append("</rows>\n")
            .append(TAB_QUADRUPLE)
                .append("<fieldBasedSearch>")
                    .append(queryProperties.getProperty(FIELD_BASED_SEARCH))
                .append("</fieldBasedSearch>\n")
            .append(TAB_QUADRUPLE)
                .append("<searcherType>")
                    .append(queryProperties.getProperty(SEARCHER_TYPE))
                .append("</searcherType>\n")
            .append(TAB_QUADRUPLE)
                .append("<queryTermFuzzyType>")
                    .append(queryProperties.getProperty(QUERY_TERM_FUZZY_TYPE))
                .append("</queryTermFuzzyType>\n")
            .append(TAB_QUADRUPLE)
                .append("<queryTermPrefixType>")
                    .append(queryProperties.getProperty(QUERY_TERM_PREFIX_TYPE))
                .append("</queryTermPrefixType>\n")
            .append(TAB_QUADRUPLE)
                .append("<queryResponseWriter>\n")
            .append(TAB_QUINTUPLE)
                .append("<responseFormat>")
                    .append(queryProperties.getProperty(RESPONSE_FORMAT))
                .append("</responseFormat>\n")
            .append(TAB_QUADRUPLE)
                .append("</queryResponseWriter>\n")
            .append(TAB_QUADRUPLE)
                .append("<highlighter>\n")
            .append(TAB_QUINTUPLE)
                .append("<snippetSize>")
                    .append(250)
                .append("</snippetSize>\n")
            .append(TAB_QUINTUPLE)
                .append("<fuzzyTagPre value = \'")
                    .append(queryProperties.get("fuzzyPreTag"))
                .append("\'></fuzzyTagPre>\n")
            .append(TAB_QUINTUPLE)
                .append("<fuzzyTagPost value = \'")
                    .append(queryProperties.get("fuzzyPostTag"))
                .append("\'></fuzzyTagPost>\n")
            .append(TAB_QUINTUPLE)
                .append("<exactTagPre value = \'")
                    .append(queryProperties.get("exactPreTag"))
                .append("\'></exactTagPre>\n")
            .append(TAB_QUINTUPLE)
                .append("<exactTagPost value = \'")
                    .append(queryProperties.get("exactPostTag"))
                .append("\'></exactTagPost>\n")
            .append(TAB_QUADRUPLE)
                .append("</highlighter>\n")
            .append(TAB_TRIPLE)
                .append("</query>\n\n");
/* QUERY CONFIGURATION NODE END */

/* SCHEMA CONFIGURATION NODE START */
        core
            .append(schemaToXml());
/* SCHEMA CONFIGURATION NODE END */

/* UPDATE CONFIGURATION NODE START */
        core
            .append(TAB_TRIPLE)
                .append("<updatehandler>\n")
            .append(TAB_QUADRUPLE)
                .append("<maxDocs>")
                    .append(updateProperties.getProperty(MAX_DOCS))
                .append("</maxDocs>\n")
            .append(TAB_QUADRUPLE)
                .append("<maxMemory>")
                    .append(updateProperties.getProperty(MAX_MEMORY))
                .append("</maxMemory>\n")
            .append(TAB_QUADRUPLE)
                .append("<mergePolicy>\n")
            .append(TAB_QUINTUPLE)
                .append("<mergeEveryNSeconds>")
                    .append(updateProperties.getProperty(MERGE_EVERY_N_SECONDS))
                .append("</mergeEveryNSeconds>\n")
            .append(TAB_QUINTUPLE)
                .append("<mergeEveryMWrites>")
                    .append(updateProperties.getProperty(MERGE_EVERY_M_WRITES))
                .append("</mergeEveryMWrites>\n")
            .append(TAB_QUADRUPLE)
                .append("</mergePolicy>\n")
            .append(TAB_TRIPLE)
                .append("</updatehandler>\n")
/* UPDATE CONFIGURATION NODE END */
            .append(TAB_DOUBLE)
                .append("</core>\n");
        return core.toString();
    }

    String schemaToXml() {
        StringBuilder schemaXML = new StringBuilder()
            .append(TAB_TRIPLE)
                .append("<schema>\n")
            .append(TAB_QUADRUPLE)
                .append("<fields>\n");

        for (Field field : schema.fields) {
            schemaXML.append(Field.toXML(field));
        }
        schemaXML
            .append(TAB_QUADRUPLE)
                .append("</fields>\n")
            .append(TAB_QUADRUPLE)
                .append("<uniqueKey>")
                    .append(schema.uniqueKey)
                .append("</uniqueKey>\n")
/* ADDING FACETS */
                .append(facetToXML())
/* DEFAULT CONFIGURATION FOR SCHEMA */
            .append(TAB_QUADRUPLE)
                .append("<types>\n")
            .append(TAB_QUINTUPLE)
                .append("<fieldType name=\"text_standard\">\n")
            .append(TAB_QUINTUPLE).append(TAB_SINGLE)
                .append("<analyzer>\n")
            .append(TAB_QUINTUPLE).append(TAB_DOUBLE)
                .append("<filter name=\"PorterStemFilter\" dictionary=\"\" />\n")
            .append(TAB_QUINTUPLE).append(TAB_DOUBLE)
                .append("<filter name=\"StopFilter\" words=\"stop-words.txt\" />\n")
            .append(TAB_QUINTUPLE).append(TAB_SINGLE)
                .append("</analyzer>\n")
            .append(TAB_QUINTUPLE)
                .append("</fieldType>\n")
            .append(TAB_QUADRUPLE)
                .append("</types>\n")
            .append(TAB_TRIPLE)
                .append("</schema>\n\n");
        return schemaXML.toString();
    }

    String facetToXML() {
        StringBuilder facetNodeXML = new StringBuilder()
            .append(TAB_QUADRUPLE)
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
                                    .append(TAB_QUINTUPLE).append(TAB_DOUBLE)
                                        .append("<facetField name=\"")
                                            .append(f.name)
                                    .append("\" facetType=\"categorical\"/>\n");
                            break;
                        case RANGE:
                        default:
                            facetFieldsXML
                                    .append(TAB_QUINTUPLE).append(TAB_DOUBLE)
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
                    .append(TAB_QUINTUPLE)
                        .append("<facetFields>\n")
                            .append(facetFieldsXML)
                    .append(TAB_QUINTUPLE)
                        .append("</facetFields>\n");
        }
        return facetNodeXML.toString();
    }

    String sqliteDatabaseParametersToXML() {
        StringBuilder sb = new StringBuilder()
                .append(TAB_DOUBLE)
                .append("<dbParameters>\n")
                .append(TAB_TRIPLE)
                .append("<dbSharedLibraryPath>")
                .append(sqliteDatabaseProperties.getProperty(DB_SHARED_LIBRARY_PATH))
                .append("</dbSharedLibraryPath>\n")
                .append(TAB_TRIPLE)
                .append("<dbSharedLibraryName>")
                .append(sqliteDatabaseProperties.get(DB_SHARED_LIBRARY_NAME))
                .append("</dbSharedLibraryName>\n")
                .append(TAB_TRIPLE)
                .append("<dbKeyValues>\n")
                .append(TAB_QUADRUPLE)
                .append("<dbKeyValue key=\"db\" value=\"" +
                        sqliteDatabaseProperties.getProperty(DB_DATABASE_NAME) + "\" />\n")
                .append(TAB_QUADRUPLE)
                .append("<dbKeyValue key=\"dbPath\" value=\"" +
                        sqliteDatabaseProperties.getProperty(DB_APP_DATABASE_PATH) + "\" />\n")
                .append(TAB_QUADRUPLE)
                .append("<dbKeyValue key=\"tableName\" value=\"" +
                        sqliteDatabaseProperties.getProperty(DB_DATABASE_TABLE_NAME) + "\" />\n")
                .append(TAB_QUADRUPLE)
                .append("<dbKeyValue key=\"listenerWaitTime\" value=\"" +
                        sqliteDatabaseProperties.getProperty(DB_LISTENER_WAIT_TIME) + "\"/>\n")
                .append(TAB_QUADRUPLE)
                .append("<dbKeyValue key=\"maxRetryOnFailure\" value=\"" +
                        sqliteDatabaseProperties.getProperty(DB_MAX_RETRY_ON_FAILURE) + "\"/>\n")
                .append(TAB_TRIPLE)
                .append("</dbKeyValues>\n")
                .append(TAB_DOUBLE)
                .append("</dbParameters>\n");
        return sb.toString();
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