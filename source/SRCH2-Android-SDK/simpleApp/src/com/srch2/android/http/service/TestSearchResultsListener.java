package com.srch2.android.http.service;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;

/**
 * Created by ashton on 7/29/2014.
 */
public class TestSearchResultsListener implements SearchResultsListener {

    public int httpResponseCode;
    public String jsonResultsLiteral;
    public HashMap<String, ArrayList<JSONObject>> resultRecordMap;

    public void reset() {
        httpResponseCode = 0;
        jsonResultsLiteral = null;
        resultRecordMap = null;
    }

    @Override
    public void onNewSearchResults(int httpResponseCode,
                                   String jsonResultsLiteral,
                                   HashMap<String, ArrayList<JSONObject>> resultRecordMap) {
        this.httpResponseCode = httpResponseCode;
        this.jsonResultsLiteral = jsonResultsLiteral;
        this.resultRecordMap = resultRecordMap;
    }

}
