package com.srch2.android.http.service;

import java.util.HashMap;

/**
 * This interface contains the callback methods for the control related RESTful
 * requests of the SRCH2 search server: upon completion of the specific request,
 * the corresponding callback method will be called passing the corresponding subclass
 * of <code>RestfulResponse</code>. Each <code>RestfulResponse</code> subclass
 * will always contain the HTTP status code and RESTful response literal as
 * returned by the SRCH2 search server; these values can be returned by calling
 * <code>getRESTfulHTTPStatusCode()</code> and <code>getRESTfulResponseLiteral()</code>
 * on any of the <code>RestfulResponse</code> subclasses.
 * <br><br>
 * Implementers of this class can either parse the RESTful response literal as
 * returned by <code>getRESTfulResponseLiteral()</code> of the specific
 * <code>RestfulResponse</code> subclass in order to manually inspect the
 * response from SRCH2 search server; however, this RESTful response literal
 * is also parsed in the construction of the each <code>RestfulResponse</code>
 * subclass and the relevant values of the RESTful response literal can be
 * accessed from the specific <code>RestfulResponse</code> subclass methods.
 * <br><br>
 * While it is not necessary to implement this interface, it is strongly
 * encouraged. Implementations of this interface can be registered by calling
 * <code>SRCH2Engine.setStateResponseListener(StateResponseListener
 * stateResultsObserver)</code>.
 * <br><br>
 * All of the callback methods in this interface are executed off the Ui thread,
 * so if any of the data is to be made visible to the user, it must be pushed to the
 * Ui thread first. Starting any intensive execution from the implementation of
 * these callbacks methods is highly discouraged as it could block subsequent
 * tasks performed by the <code>SRCH2Engine</code>.
 */
public interface StateResponseListener {

    /**
     * Called after the SRCH2 search server completes an information task, which occurs
     * when either <code>mIndexable.info()</code> or <code>SRCH2Engine.getIndexInfo(String
     * indexName)</code> is called.
     * @param indexName the name of the index that the information task was completed upon
     * @param response a representation of the returned index information
     */
    void onInfoRequestComplete(String indexName,
                               InfoResponse response);

    /**
     * Called after the SRCH2 search server completes an insert task, which occurs
     * when either <code>mIndexable.insert(...)</code> or
     * <code>SRCH2Engine.insertIntoIndex(...)</code> is called.
     * @param indexName the name of the index that was inserted into
     * @param response a representation of the RESTful insert response
     */
    void onInsertRequestComplete(String indexName,
                                 InsertResponse response);

    /**
     * Called after the SRCH2 search server completes an update task, which occurs
     * when either <code>mIndexable.update(...)</code> or
     * <code>SRCH2Engine.updateIndex(...)</code> is called.
     * @param indexName the name of the index that was updated
     * @param response a representation of the RESTful update response
     */
    void onUpdateRequestComplete(String indexName,
                                 UpdateResponse response);

    /**
     * Called after the SRCH2 search server comes online after the call to <code>SRCH2.onStart(Context
     * context)</code> is made. When this method executes, it will pass a map of indexes ready for
     * CRUD operations, defined by the <code>Indexable</code> implementations: this map contains the
     * names of the indexes (as they were defined in the return value of the <code>getIndexName()</code>
     * implementation of each <code>Indexable</code>) as
     * its keys mapping to the <code>InfoResponse</code> for each index.
     *
     * @param indexesToInfoResponseMap a mapping of indexes to their valid <code>InfoResponse</code>s
     */
    void onSRCH2ServiceReady(HashMap<String, InfoResponse> indexesToInfoResponseMap);

    /**
     * Called after the SRCH2 search server completes an deletion task, which occurs
     * when either <code>mIndexable.delete(...)</code> or
     * <code>SRCH2Engine.deleteFromIndex(...)</code> is called.
     * @param indexName the name of the index that had deletions
     * @param response a representation of the RESTful deletion response
     */
    void onDeleteRequestComplete(String indexName,
                                 DeleteResponse response);

    /**
     * Called after the SRCH2 search server completes an record retrieval task, which occurs
     * when either <code>mIndexable.getRecordbyID(...)</code> or
     * <code>SRCH2Engine.getRecordByIdFromIndex(...)</code> is called.
     * @param indexName the name of the index that the record requested was retreived from
     * @param response a representation of the RESTful record retrieval response, including the record
     *                 retrieved if found
     */
    void onGetRecordByIDComplete(String indexName,
                                 GetRecordResponse response);

}
