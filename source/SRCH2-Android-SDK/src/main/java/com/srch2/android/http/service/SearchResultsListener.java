package com.srch2.android.http.service;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;

/**
 * User need to implement this interface to deal with the search result.
 */
public interface SearchResultsListener {
    /**
     * Whenever the search result is ready, the engine will call this function to send user the search results.
     *
     * @param httpResponseCode the httpResponseCode get from the engine. It consistent with <a href=http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html>w3c standard</a>
     * @param jsonResultsLiteral the original http response literal from the engine.
     * @param resultRecordMap the search result. It is a HashMap to associate the JSON result with each of the Index by {@link IndexDescription#getIndexName()}
     */
    void onNewSearchResults(int httpResponseCode, String jsonResultsLiteral, HashMap<String, ArrayList<JSONObject>> resultRecordMap);
}
