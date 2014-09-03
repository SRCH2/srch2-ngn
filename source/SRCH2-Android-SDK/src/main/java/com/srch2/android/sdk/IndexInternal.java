package com.srch2.android.sdk;


import org.json.JSONArray;
import org.json.JSONObject;

import java.net.URL;
import java.util.concurrent.atomic.AtomicBoolean;

final class IndexInternal {
    private static final String TAG = "IndexInternal";

    final IndexDescription indexDescription;
    private SearchTask currentSearchTask = null;

    AtomicBoolean isDirty;

    IndexInternal(IndexDescription description) {
        isDirty = new AtomicBoolean(false);
        this.indexDescription = description;
    }

    static String formatDefaultQueryURL(String queryString) {
        String[] tokens = queryString.split("\\s+");
        StringBuilder rawSearchInput = new StringBuilder("q=");
        for (int i = 0; i < tokens.length - 1; i++) {
            tokens[i] = UrlBuilder.getURLEncodedUTF8String(tokens[i]);
            rawSearchInput.append(tokens[i]).append("~").append(" AND ");
        }
        rawSearchInput.append(tokens[tokens.length - 1]).append("*~");

        return rawSearchInput.toString();
    }

    IndexDescription getConf() {
        return indexDescription;
    }

    String getIndexCoreName() {
        return indexDescription.name;
    }

    void insert(JSONObject record) {
//        if (SRCH2Engine.isReady()) {
            InsertTask insertTask = new InsertTask(UrlBuilder.getInsertUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(), record);
            HttpTask.addToQueue(insertTask);
            SRCH2Engine.isChanged.set(true);
            isDirty.set(true);
//        }
    }


    void insert(JSONArray records) {
//        if (SRCH2Engine.isReady()) {
            URL url = UrlBuilder.getInsertUrl(SRCH2Engine.getConfig(),
                    indexDescription);

            InsertTask insertTask = new InsertTask(url, getIndexCoreName(),
                    records);
            HttpTask.addToQueue(insertTask);
            SRCH2Engine.isChanged.set(true);
            isDirty.set(true);
//        }
    }

    void update(JSONObject record) {
//        if (SRCH2Engine.isReady()) {
            UpdateTask updateTask = new UpdateTask(UrlBuilder.getUpdateUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(), record);
            HttpTask.addToQueue(updateTask);
            SRCH2Engine.isChanged.set(true);
            isDirty.set(true);
//        }
    }

    void update(JSONArray records) {
//        if (SRCH2Engine.isReady()) {
            UpdateTask updateTask = new UpdateTask(UrlBuilder.getUpdateUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(), records);
            HttpTask.addToQueue(updateTask);
            SRCH2Engine.isChanged.set(true);
            isDirty.set(true);
//        }
    }

    void delete(String id) {
//        if (SRCH2Engine.isReady()) {
            DeleteTask deleteTast = new DeleteTask(UrlBuilder.getDeleteUrl(
                    SRCH2Engine.getConfig(), indexDescription, id),
                    getIndexCoreName());
            HttpTask.addToQueue(deleteTast);
            SRCH2Engine.isChanged.set(true);
            isDirty.set(true);
//        }
    }

    void searchRawString(String rawString) {
        SRCH2Engine.lastQuery.set(new SRCH2Engine.IndexQueryPair(this, rawString));
  //      if (SRCH2Engine.isReady()) {
            if (currentSearchTask != null) {
                currentSearchTask.cancel();
            }
            currentSearchTask = new SearchTask(

                    UrlBuilder.getSearchUrl(SRCH2Engine.getConfig(), indexDescription,
                            rawString), getIndexCoreName(),
                    SRCH2Engine.getSearchResultsObserver());

            HttpTask.addToQueue(currentSearchTask);
//        }
    }

    void search(String searchInput) {
        if (searchInput == null || searchInput.trim().length() < 1) {
            throw new IllegalArgumentException("Invalid searchInput");
        }
        searchInput = formatDefaultQueryURL(searchInput);
        //if (this.indexDescription.isGeoIndex()) {
        ////searchInput += formatDefaultGeoCircle();
        //}
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
 //       if (SRCH2Engine.isReady()) {
            HttpTask.addToQueue(new GetRecordTask(UrlBuilder.getGetDocUrl(
                    SRCH2Engine.getConfig(), indexDescription, id), this.getIndexCoreName()));
 //       }
    }

}
