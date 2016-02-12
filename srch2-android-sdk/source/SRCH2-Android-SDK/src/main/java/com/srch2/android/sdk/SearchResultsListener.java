package com.srch2.android.sdk;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;

/**
 * This interface contains the callback method for the search RESTful requests of the SRCH2 search server:
 * upon completion of the search request, the method {@link #onNewSearchResults(int, String, java.util.HashMap)}
 * will be triggered
 * containing the results of the search. The RESTful response literal can be inspected by evaluating the
 * <code>String JSONResponse</code> to view the results of a search; in addition, this RESTful response
 * literal is parsed and the specific records of the search are parsed and put into the map
 * <code>HashMap resultRecordMap</code>. This map contains as its keys the names of the indexes mapped to an
 * <code>ArrayList</code>
 * of the parsed <code>JSONObject</code>
 * records.
 * <br><br>
 * This callback can be registered by calling {@link SRCH2Engine#setSearchResultsListener(SearchResultsListener)}
 * or passing it in when calling
 * {@link com.srch2.android.sdk.SRCH2Engine#onResume(android.content.Context, SearchResultsListener, boolean)}
 * and can be reset at any time. It is not necessary to register an implementation
 * of <code>SearchResultsListener</code> but it is the only way through this API to receive search results.
 * <br><br>
 * The method {@link #onNewSearchResults(int, String, java.util.HashMap)} will be executed <b>off of the Ui
 * thread</b> by default, but can be do so by passing <b>true</b> when setting the <code>SearchResultListener</code>
 * instance when calling
 * {@link com.srch2.android.sdk.SRCH2Engine#setSearchResultsListener(SearchResultsListener, boolean)}.
 */
public interface SearchResultsListener {

    /**
     * Called whenever the SRCH2 search server finishing processing a search request. The response of the
     * SRCH2 search server can be evaluated in two ways: the RESTful response literal can be inspected by accessing the
     * <code>String jsonResponse</code> to view the results of a search; in addition, this RESTful response
     * literal is parsed and the specific records of the search are parsed and put into the map
     * <code>resultRecordMap</code>.
     * <br><br>
     * This map contains as its keys the names of the indexes (as set by the return value
     * of
     * {@link Indexable#getIndexName()} and {@link SQLiteIndexable#getIndexName()}}) mapped to an <code>ArrayList</code>
     * of the parsed <code>JSONObject</code>
     * records. Each <code>JSONObject</code> <b>will always have one key by the name of 'record'</b> which will have
     * as its set of keys the
     * field names as they were defined in the schema returned
     * from {@link Indexable#getSchema()} or {@link SQLiteIndexable#getSchema()} in order to retrieve the corresponding
     * data values set. If any field
     * had highlighting enabled by calling {@link Field#enableHighlighting()}, the <code>JSONObject</code> will also
     * contain the key 'highlighted' which will contain the highlighted text for each field as a <code>JSONArray</code>.
     * The highlighted text will be formatted according to the configuration of the {@link com.srch2.android.sdk.Highlighter}
     * returned from {@link Indexable#getHighlighter()} or {@link SQLiteIndexable#getHighlighter()}.
     * <br><br>
     * If the search that was performed is specific to a single index, this map will only contain one key:
     * the name of searched index; otherwise the map will contain results for all the indexes (such as when
     * {@link SRCH2Engine#searchAllIndexes(String)} is called).
     * <br><br>
     * If there are no results for a given
     * search input, the map will be <b>non-null</b> but each <code>ArrayList&lt;JSONObject&gt;</code> will
     * be of size 0.
     * <br><br>
     * The method {@link #onNewSearchResults(int, String, java.util.HashMap)} will be executed <b>off of the Ui
     * thread</b> by default, but can be do so by passing <b>true</b> when setting the <code>SearchResultListener</code>
     * instance when calling
     * {@link com.srch2.android.sdk.SRCH2Engine#setSearchResultsListener(SearchResultsListener, boolean)}.
     *
     * @param HTTPResponseCode the HTTP response code as it was returned by the SRCH2 search server
     * @param JSONResponse the RESTful response as it was returned by the SRCH2 search server
     * @param resultMap a parsing of the <code>JSONResponse</code> that maps the names of indexes to the
     *                        sets of their results
     */
    void onNewSearchResults(int HTTPResponseCode, String JSONResponse, HashMap<String, ArrayList<JSONObject>> resultMap);
}
