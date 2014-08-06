package com.srch2.android.http.service;

import com.srch2.android.http.service.SRCH2Engine.IndexQueryPair;
import org.json.JSONArray;
import org.json.JSONObject;

import java.net.URL;

final class IndexInternal {
    private final IndexDescription indexDescription;
    private SearchTask currentSearchTask = null;

    IndexInternal(IndexDescription description) {
        this.indexDescription = description;
    }

    /**
     * Format the query string using all the default value. The default value
     * comes from the indexInternal description with which the user create the indexInternal
     *
     * @param queryString
     * @return The valid URL formatted string.
     */
    static String formatDefaultQueryURL(String queryString) {
        String[] tokens = queryString.split("\\s+");
        StringBuilder rawSearchInput = new StringBuilder();
        for (int i = 0; i < tokens.length - 1; i++) {
            tokens[i] = UrlBuilder.getURLEncodedUTF8String(tokens[i]);
            rawSearchInput.append(tokens[i]).append("~").append(" AND ");
        }
        rawSearchInput.append(tokens[tokens.length - 1]).append("*~");

        return rawSearchInput.toString();
    }

    /**
     * FIXME
     * this is the temporary solution for the GeoQuery. We need to give the engine one default range.
     *
     * @return
     */
    private static String formatDefaultGeoCircle() {
        return "&clat=33&clong=-117&radius=180";
    }

    IndexDescription getConf() {
        return indexDescription;
    }

    String getIndexCoreName() {
        return indexDescription.name;
    }

    /**
     * Inserts one record into the Index.
     *
     * @param record Json record to be inserted.
     */
    public void insert(JSONObject record) {
        if (SRCH2Engine.isReady()) {
            InsertTask insertTask = new InsertTask(UrlBuilder.getInsertUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener(), record);
            HttpTask.executeTask(insertTask);
            SRCH2Engine.isChanged.set(true);
        }
    }

    /**
     * Inserts a list of record into the Index in a batch way.
     *
     * @param records An array of Json objects of type JSONArray to be inserted.
     */
    public void insert(JSONArray records) {
        if (SRCH2Engine.isReady()) {

            URL url = UrlBuilder.getInsertUrl(SRCH2Engine.getConfig(),
                    indexDescription);

            InsertTask insertTask = new InsertTask(url, getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener(), records);
            HttpTask.executeTask(insertTask);
            SRCH2Engine.isChanged.set(true);
        }
    }

    /**
     * Update the record in the engine.
     *
     * @param record Json record to be updated.
     */
    public void update(JSONObject record) {
        if (SRCH2Engine.isReady()) {
            UpdateTask updateTask = new UpdateTask(UrlBuilder.getUpdateUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener(), record);
            HttpTask.executeTask(updateTask);
            SRCH2Engine.isChanged.set(true);
        }
    }

    /**
     * Update a list of records in the engine.
     *
     * @param records An array of Json objects of type JSONArray to be updated.
     */
    public void update(JSONArray records) {
        if (SRCH2Engine.isReady()) {
            UpdateTask updateTask = new UpdateTask(UrlBuilder.getUpdateUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener(), records);
            HttpTask.executeTask(updateTask);
            SRCH2Engine.isChanged.set(true);
        }
    }

    /**
     * Deletes the record using the record id, or a list of ids.
     *
     * @param id     id of the record to be deleted.
     */
    public void delete(String id) {
        if (SRCH2Engine.isReady()) {
            DeleteTask deleteTast = new DeleteTask(UrlBuilder.getDeleteUrl(
                    SRCH2Engine.getConfig(), indexDescription, id),
                    getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener());
            HttpTask.executeTask(deleteTast);
            SRCH2Engine.isChanged.set(true);
        }
    }

    /**
     * Get the information from the indexInternal
     */
    public void info() {
        if (SRCH2Engine.isReady()) {
            InfoTask infoTask = new InfoTask(UrlBuilder.getInfoUrl(
                    SRCH2Engine.getConfig(), indexDescription), getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener());
            HttpTask.executeTask(infoTask);
        }
    }

    void searchRawString(String rawString) {
        SRCH2Engine.lastQuery.set(new IndexQueryPair(this, rawString));
        if (SRCH2Engine.isReady()) {
            if (currentSearchTask != null) {
                currentSearchTask.cancel();
            }
            currentSearchTask = new SearchTask(

                    UrlBuilder.getSearchUrl(SRCH2Engine.getConfig(), indexDescription,
                            rawString), getIndexCoreName(),
                    SRCH2Engine.getSearchResultsListener());

            HttpTask.executeTask(currentSearchTask);
        }
    }

    /**
     * Does a basic search on the current index. The queryString containing the
     * space will be split and searched using multiple keywords. All query
     * keywords are treated as fuzzy, and the last keyword will
     * be treated as fuzzy and prefix. For the more specific setting on
     * the terms, please use {@link #advancedSearch(Query)} method.
     *
     * @param queryString query string to be searched.
     */
    public void search(String queryString) {
        if (queryString == null || queryString.trim().length() < 1) {
            throw new IllegalArgumentException("Invalid queryString");
        }
        queryString = formatDefaultQueryURL(queryString);
        if (this.indexDescription.isGeoIndex()) {
            queryString += formatDefaultGeoCircle();
        }
        searchRawString(queryString);
    }

    /**
     * Does a more powerful search using the {@link Query} object
     *
     * @param query query to be searched.
     */
    public void advancedSearch(Query query) {
        if (query == null) {
            throw new IllegalArgumentException("the query parameter is null");
        }
        String queryString = query.toString();
        searchRawString(queryString);
    }

    /**
     * Get the record by giving its primary key The record will be sent to the
     * <code>searchResultsListener</code>
     *
     * @param id the primary key
     */
    public void getRecordbyID(String id) {
        if (id == null || id.length() < 1) {
            return;
        }
        if (SRCH2Engine.isReady()) {
            HttpTask.executeTask(new GetRecordTask(UrlBuilder.getGetDocUrl(
                    SRCH2Engine.getConfig(), indexDescription, id), this
                    .getIndexCoreName(), SRCH2Engine
                    .getControlResponseListener()));
        }
    }

}
