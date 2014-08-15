package com.srch2.android.http.service;

import com.srch2.android.http.service.HttpTask.SearchHttpTask;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicBoolean;

class SearchTask extends SearchHttpTask {

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

                    SRCH2Engine.conf.indexableMap.get(coreName).incrementSearchRequestCount();


                    JSONObject o = root.getJSONObject(coreNodes.getString(j));
                    JSONArray nodes = o.getJSONArray("results");

                    ArrayList<JSONObject> records = new ArrayList<JSONObject>();

                    for (int i = 0; i < nodes.length(); ++i) {
                        try {
                            JSONObject resultNodes = (JSONObject) nodes.get(i);
                            JSONObject record = resultNodes.getJSONObject("record");
                            records.add(record);
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                    resultMap.put(coreName, records);
                }
            } catch (JSONException ignore) {
                ignore.printStackTrace();
            }
        } else {
            SRCH2Engine.conf.indexableMap.get(targetCoreName).incrementSearchRequestCount();

            ArrayList<JSONObject> recordResults = new ArrayList<JSONObject>();
            try {
                JSONObject root = new JSONObject(json);
                JSONArray nodes = root.getJSONArray("results");

                for (int i = 0; i < nodes.length(); ++i) {
                    try {
                        JSONObject resultNodes = (JSONObject) nodes.get(i);
                        JSONObject record = resultNodes.getJSONObject("record");
                        recordResults.add(record);
                    } catch (Exception e) {
                        e.printStackTrace();
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
            networkProblem.printStackTrace();
            jsonResponse = networkProblem.getMessage();
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }
        if (!shouldHalt()) {
            onTaskComplete(responseCode, jsonResponse);
        }
    }

    @Override
    protected void onTaskComplete(int returnedResponseCode,
                                  String returnedResponseLiteral) {
        if (searchResultsListener != null) {
            HashMap<String, ArrayList<JSONObject>> resultMap;

            if (returnedResponseCode / 100 == 2) {
                resultMap = parseResponseForRecordResults(
                        returnedResponseLiteral, isMultiCoreSearch,
                        super.targetCoreName);
                searchResultsListener.onNewSearchResults(returnedResponseCode,
                        returnedResponseLiteral, resultMap);
            } else {
                searchResultsListener.onNewSearchResults(returnedResponseCode,
                        returnedResponseLiteral,
                        new HashMap<String, ArrayList<JSONObject>>(0));
            }
        }
    }
}
