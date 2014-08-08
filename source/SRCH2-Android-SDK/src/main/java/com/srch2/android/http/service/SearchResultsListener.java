package com.srch2.android.http.service;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;

import org.json.JSONObject;

/**
 * This interface contains the callback method for the search RESTful requests of the SRCH2 search server:
 * upon completion of the search request, the method <code>onNewSearchResults(int httpResponseCode,
 * String jsonResultsLiteral, HashMap<String, ArrayList<JSONObject>> resultRecordMap</code> will be triggered
 * containing the results of the search. The RESTful response literal can be inspected by evaluating the
 * <code>String jsonResultsLiteral</code> to view the results of a search; in addition, this RESTful response
 * literal is parsed and the specific records of the search are parsed and put into the map
 * <code>resultRecordMap</code>. This map contains as its keys the names of the index (as set by the
 * <code>IndexDescription</code> returned from the <code>Indexable</code> implementation of
 * <code>getIndexDescription()</code>) mapped to an <code>ArrayList</code> of the parsed <code>JSONObject</code>
 * records. If the search that was performed is specific to a single index, this map will only contain one key--
 * the name of that index searched; otherwise the map will contain results for all the indexes (such as when
 * <code>SRCH2Engine.searchAllIndexes(String searchInput)</code> is called). If there are no results for a given
 * search input, the map will be <b>non-null</b> but contain no values for each index name key.
 * <br><br>
 * This callback can be registered by calling <code>SRCH2Engine.setSearchResultsListener(SearchResultsListener
 * searchResultListener)</code> and can be reset at any time. It is not necessary to register an implementation
 * of <code>SearchResultsListener</code> but it is the only way through this API to recieve search results.
 * <br><br>
 * Whenever <code>onNewSearchResults(...)</code> is executed, it will always occur off of the Ui thread so
 * for results to be displayed to the user they must be pushed to the Ui thread first. Implementing this
 * interface on subclass of <code>android.os.Handler</code> is one way to do this. Starting any intensive
 * execution from the implementation of this callback's method is highly discouraged as it could block
 * subsequent search tasks performed by the SRCH2Engine; however simple post-processing should not cause
 * any blocking.
 */
public interface SearchResultsListener {
    /**
     * Called whenever the SRCH2 search server finishing processing a search request. The response of the
     * SRCH2 search server can be evaluated in two ways: the RESTful response literal can be inspected by accessing the
     * <code>String jsonResultsLiteral</code> to view the results of a search; in addition, this RESTful response
     * literal is parsed and the specific records of the search are parsed and put into the map
     * <code>resultRecordMap</code>. This map contains as its keys the names of the index (as set by the
     * <code>IndexDescription</code> returned from the <code>Indexable</code> implementation of
     * <code>getIndexDescription()</code>) mapped to an <code>ArrayList</code> of the parsed <code>JSONObject</code>
     * records. If the search that was performed is specific to a single index, this map will only contain one key--
     * the name of that index searched; otherwise the map will contain results for all the indexes (such as when
     * <code>SRCH2Engine.searchAllIndexes(String searchInput)</code> is called). If there are no results for a given
     * search input, the map will be <b>non-null</b> but contain no values for each index name key.
     * @param httpResponseCode the HTTP response code as it was returned by the SRCH2 search server
     * @param jsonResultsLiteral the RESTful response as it was returned by the SRCH2 search server
     * @param resultRecordMap a parsing of the <code>jsonResultsLiteral</code> that maps the names of indexes to the
     *                        sets of their results
     */
    void onNewSearchResults(int httpResponseCode, String jsonResultsLiteral, HashMap<String, ArrayList<JSONObject>> resultRecordMap);
}
