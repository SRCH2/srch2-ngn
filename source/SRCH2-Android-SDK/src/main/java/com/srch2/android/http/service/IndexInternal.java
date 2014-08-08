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

    void insert(JSONObject record) {
        if (SRCH2Engine.isReady()) {
            InsertTask insertTask = new InsertTask(UrlBuilder.getInsertUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener(), record);
            HttpTask.executeTask(insertTask);
            SRCH2Engine.isChanged.set(true);
        }
    }


    void insert(JSONArray records) {
        if (SRCH2Engine.isReady()) {

            URL url = UrlBuilder.getInsertUrl(SRCH2Engine.getConfig(),
                    indexDescription);

            InsertTask insertTask = new InsertTask(url, getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener(), records);
            HttpTask.executeTask(insertTask);
            SRCH2Engine.isChanged.set(true);
        }
    }

    void update(JSONObject record) {
        if (SRCH2Engine.isReady()) {
            UpdateTask updateTask = new UpdateTask(UrlBuilder.getUpdateUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener(), record);
            HttpTask.executeTask(updateTask);
            SRCH2Engine.isChanged.set(true);
        }
    }

    void update(JSONArray records) {
        if (SRCH2Engine.isReady()) {
            UpdateTask updateTask = new UpdateTask(UrlBuilder.getUpdateUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener(), records);
            HttpTask.executeTask(updateTask);
            SRCH2Engine.isChanged.set(true);
        }
    }

    void delete(String id) {
        if (SRCH2Engine.isReady()) {
            DeleteTask deleteTast = new DeleteTask(UrlBuilder.getDeleteUrl(
                    SRCH2Engine.getConfig(), indexDescription, id),
                    getIndexCoreName(),
                    SRCH2Engine.getControlResponseListener());
            HttpTask.executeTask(deleteTast);
            SRCH2Engine.isChanged.set(true);
        }
    }


    void info() {
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

    void search(String searchInput) {
        if (searchInput == null || searchInput.trim().length() < 1) {
            throw new IllegalArgumentException("Invalid searchInput");
        }
        searchInput = formatDefaultQueryURL(searchInput);
        if (this.indexDescription.isGeoIndex()) {
            searchInput += formatDefaultGeoCircle();
        }
        searchRawString(searchInput);
    }

    void advancedSearch(Query query) {
        if (query == null) {
            throw new IllegalArgumentException("the query parameter is null");
        }
        String queryString = query.toString();
        searchRawString(queryString);
    }

    void getRecordbyID(String id) {
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
