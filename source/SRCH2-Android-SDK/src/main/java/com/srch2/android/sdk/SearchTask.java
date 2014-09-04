package com.srch2.android.sdk;

import android.os.Handler;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.concurrent.atomic.AtomicBoolean;

class SearchTask extends HttpTask.SearchHttpTask {

    static final String TAG = "SearchTask";

    private final AtomicBoolean isCancelled = new AtomicBoolean(false);
    private boolean isMultiCoreSearch = false;

    SearchTask(URL url, SearchResultsListener searchResultsListener) {
        super(url, null, searchResultsListener);
        isMultiCoreSearch = true;
    }

    SearchTask(URL url, String nameOfTheSingleCoreToQuery,
               SearchResultsListener searchResultsListener) {
        super(url, nameOfTheSingleCoreToQuery, searchResultsListener);
        isMultiCoreSearch = false;
    }

    static HashMap<String, ArrayList<JSONObject>> parseResponseForRecordResults(
            String json, boolean isMultiCoreSearch, String targetCoreName) {
        HashMap<String, ArrayList<JSONObject>> resultMap = new HashMap<String, ArrayList<JSONObject>>();

        if (isMultiCoreSearch) {
            try {
                JSONObject root = new JSONObject(json);
                JSONArray coreNodes = root.names();

                for (int j = 0; j < coreNodes.length(); ++j) {

                    String coreName = coreNodes.getString(j);

                    Cat.d(TAG, "corename about to parse is " + coreName);

                    JSONObject o = root.getJSONObject(coreNodes.getString(j));
                    JSONArray nodes = o.getJSONArray("results");

                    ArrayList<JSONObject> records = new ArrayList<JSONObject>();

                    for (int i = 0; i < nodes.length(); ++i) {
                        try {
                            JSONObject resultNodes = (JSONObject) nodes.get(i);
                            JSONObject record = resultNodes.getJSONObject("record");
                            JSONObject newRecord = new JSONObject();
                            newRecord.put(Indexable.SEARCH_RESULT_JSON_KEY_RECORD, record);
                            if (resultNodes.has("snippet")) {
                                boolean highlightingNotEmpty = true;
                                JSONObject snippet = null;
                                try {
                                    snippet = resultNodes.getJSONObject("snippet");
                                } catch (JSONException eee) {
                                    highlightingNotEmpty = false;
                                }
                                if (highlightingNotEmpty && snippet != null && snippet.length() > 0) {
                                    Iterator<String> snippetKeys = snippet.keys();
                                    while (snippetKeys.hasNext()) {
                                        String key = snippetKeys.next();
                                        String highlight = null;

                                        try {
                                            highlight = snippet.getString(key);
                                        } catch (JSONException highlighterOops) {
                                            continue;
                                        }

                                        if (highlight != null) {
                                            highlight = highlight.replace("\\/", "/");
                                            highlight = highlight.replace("\\\"", "\"");
                                            highlight = highlight.substring(2, highlight.length() - 2);
                                            snippet.put(key, highlight);
                                        }
                                    }
                                    newRecord.put(Indexable.SEARCH_RESULT_JSON_KEY_HIGHLIGHTED, snippet);
                                }
                            }
                            records.add(newRecord);
                        } catch (Exception e) {
                            Cat.ex(TAG, "while parsing records General Exception", e);
                        }
                    }
                    resultMap.put(coreName, records);
                }
            } catch (JSONException ignore) {
                Cat.ex(TAG, "while parsing records JSONException", ignore);
            }
        } else {

            ArrayList<JSONObject> recordResults = new ArrayList<JSONObject>();
            try {
                JSONObject root = new JSONObject(json);
                JSONArray nodes = root.getJSONArray("results");

                for (int i = 0; i < nodes.length(); ++i) {
                    try {
                        JSONObject resultNodes = (JSONObject) nodes.get(i);
                        JSONObject record = resultNodes.getJSONObject("record");

                        JSONObject newRecord = new JSONObject();
                        newRecord.put(Indexable.SEARCH_RESULT_JSON_KEY_RECORD, record);

                        if (resultNodes.has("snippet")) {
                            boolean highlightingNotEmpty = true;
                            JSONObject snippet = null;
                            try {
                                snippet = resultNodes.getJSONObject("snippet");
                            } catch (JSONException eee) {
                                highlightingNotEmpty = false;
                            }

                            if (highlightingNotEmpty && snippet != null && snippet.length() > 0) {

                                Iterator<String> snippetKeys = snippet.keys();
                                while (snippetKeys.hasNext()) {
                                    String key = snippetKeys.next();
                                    String highlight = null;
                                    try {
                                        highlight = snippet.getString(key);
                                    } catch (JSONException highlighterOops) {
                                        continue;
                                    }

                                    if (highlight != null) {
                                        highlight = highlight.replace("\\/", "/");
                                        highlight = highlight.replace("\\\"", "\"");
                                        highlight = highlight.substring(2, highlight.length() - 2);
                                        snippet.put(key, highlight);
                                    }
                                }
                                newRecord.put(Indexable.SEARCH_RESULT_JSON_KEY_HIGHLIGHTED, snippet);
                            }
                        }
                        recordResults.add(newRecord);

                    } catch (Exception e) {
                        Cat.ex(TAG, "while parsing records", e);
                    }

                }
            } catch (JSONException ignore) {
            }
            resultMap.put(targetCoreName, recordResults);
        }
        return resultMap;
    }

    void cancel() {
        isCancelled.set(true);
    }

    private boolean shouldHalt() {
        return Thread.currentThread().isInterrupted() || isCancelled.get();
    }

    @Override
    public void run() {
        doSearch();
    }

    private void doSearch() {
        HttpURLConnection connection = null;
        String jsonResponse = null;
        int responseCode = -1;
        try {
            Cat.d("SEARCH_TASK:", targetUrl.toString());
            connection = (HttpURLConnection) targetUrl.openConnection();
            connection.setReadTimeout(1000);
            connection.setConnectTimeout(1000);
            connection.setRequestMethod("GET");
            connection.setDoInput(true);
            connection.connect();

            responseCode = connection.getResponseCode();
            if (responseCode / 100 == 2) {
                jsonResponse = readInputStream(connection.getInputStream());
            } else {
                jsonResponse = readInputStream(connection.getErrorStream());
            }
        } catch (IOException networkProblem) {
            jsonResponse = handleIOExceptionMessagePassing(networkProblem, jsonResponse, "SearchTask");
            responseCode = HttpURLConnection.HTTP_INTERNAL_ERROR;
            if (jsonResponse.contains(SRCH2Engine.ExceptionMessages.IO_EXCEPTION_ECONNREFUSED_CONNECTION_REFUSED)) {
                Cat.d(TAG, "search task crashed server contained message");
                onTaskCrashedSRCH2SearchServer();
            } else {
                Cat.d(TAG, "search task DID NOT crashed server message was " + jsonResponse);
            }
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }

        if (jsonResponse == null) {
            jsonResponse = prepareIOExceptionMessageForCallback();
            responseCode = HttpURLConnection.HTTP_INTERNAL_ERROR;
        }

        if (!shouldHalt()) {
            onExecutionCompleted(TASK_ID_SEARCH);
            onTaskComplete(responseCode, jsonResponse);
        }
    }

    @Override
    protected void onTaskComplete(final int returnedResponseCode,
                                  final String returnedResponseLiteral) {
        pushSearchResultsToCallback(returnedResponseCode, returnedResponseLiteral);
    }

    void pushSearchResultsToCallback(final int returnedResponseCode, final String returnedResponseLiteral) {
        if (searchResultsListener != null) {
            final HashMap<String, ArrayList<JSONObject>> resultMap;
            if (returnedResponseCode / 100 == 2) {
                resultMap = parseResponseForRecordResults(
                        returnedResponseLiteral, isMultiCoreSearch,
                        targetCoreName);
            } else {
                resultMap = new HashMap<String, ArrayList<JSONObject>>(0);
            }
            if (SRCH2Engine.searchResultsPublishedToUiThread) {
                Handler uiHandler = SRCH2Engine.getSearchResultsUiCallbackHandler();
                if (uiHandler != null) {
                    uiHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            searchResultsListener.onNewSearchResults(returnedResponseCode,
                                    returnedResponseLiteral, resultMap);
                        }
                    });
                }
            } else {
                searchResultsListener.onNewSearchResults(returnedResponseCode,
                        returnedResponseLiteral, resultMap);
            }
        }
    }

    static HashMap<String, ArrayList<JSONObject>> getEmptyResultSet(Collection<Indexable> indexables) {
        HashMap<String, ArrayList<JSONObject>> emptyResultSet = new HashMap<String, ArrayList<JSONObject>>(0);
        for (Indexable idx : indexables) {
            emptyResultSet.put(idx.getIndexName(), new ArrayList<JSONObject>(0));
        }
        return emptyResultSet;
    }

    private final static String getEmptyResultSetPerCoreJSONResponse(String indexName, boolean isLast) {
        if (isLast) {
            return "\"" + indexName + "\":" +
                    "{\"estimated_number_of_results\":0," +
                    "\"fuzzy\":1," +
                    "\"limit\":0," +
                    "\"message\":\"NOTICE : topK query\"," +
                    "\"offset\":0,\"payload_access_time\":0," +
                    "\"query_keywords\":[\"\"]," +
                    "\"query_keywords_complete\":[false]," +
                    "\"results\":[]," +
                    "\"results_found\":0," +
                    "\"searcher_time\":0," +
                    "\"type\":0}";
        } else {
            return "\"" + indexName + "\":" +
                    "{\"estimated_number_of_results\":0," +
                    "\"fuzzy\":1," +
                    "\"limit\":0," +
                    "\"message\":\"NOTICE : topK query\"," +
                    "\"offset\":0,\"payload_access_time\":0," +
                    "\"query_keywords\":[\"\"]," +
                    "\"query_keywords_complete\":[false]," +
                    "\"results\":[]," +
                    "\"results_found\":0," +
                    "\"searcher_time\":0," +
                    "\"type\":0},";
            }
    }

    static String getEmptyResultJSONResponse(Collection<Indexable> indexables) {
        StringBuilder sb = new StringBuilder("{");
        Iterator<Indexable> idxs = indexables.iterator();
        while (idxs.hasNext()) {
            Indexable idx = idxs.next();
            sb.append(getEmptyResultSetPerCoreJSONResponse(idx.getIndexName(), !idxs.hasNext()));
        }
        sb.append("}");
        return sb.toString();
    }
}
