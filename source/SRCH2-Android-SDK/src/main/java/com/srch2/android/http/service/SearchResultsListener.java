package com.srch2.android.http.service;

import java.util.ArrayList;
import java.util.HashMap;

import org.json.JSONObject;

public interface SearchResultsListener {
    void onNewSearchResults(int httpResponseCode, String jsonResultsLiteral, HashMap<String, ArrayList<JSONObject>> resultRecordMap);
}
