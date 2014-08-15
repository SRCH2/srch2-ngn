package com.srch2.android.http.service;

/**
 * This interface contains the callback methods for the control related RESTful
 * requests of the SRCH2 search server: upon completion of the specific request,
 * the corresponding callback method will be called passing the corresponding subclass
 * of <code>RestfulResponse</code>. Each <code>RestfulResponse</code> subclass
 * will always contain the HTTP status code and RESTful response literal as
 * returned by the SRCH2 search server; these values can be returned by calling
 * {@link RestfulResponse#getRESTfulResponseLiteral()} and {@link RestfulResponse#getRESTfulHTTPStatusCode()}
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
 * {@link com.srch2.android.http.service.SRCH2Engine#setStateResponseListener(StateResponseListener)}.
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
     * when {@link Indexable#info()} is called.
     * @param indexName the name of the index that the information task was completed upon
     * @param response a representation of the returned index information
     */
    void onInfoRequestComplete(String indexName,
                               InfoResponse response);

    /**
     * Called after the SRCH2 search server completes an insert task, which occurs
     * when either {@link com.srch2.android.http.service.Indexable#insert(org.json.JSONArray)} or
     * {@link com.srch2.android.http.service.Indexable#insert(org.json.JSONObject)} is called.
     * @param indexName the name of the index that was inserted into
     * @param response a representation of the RESTful insert response
     */
    void onInsertRequestComplete(String indexName,
                                 InsertResponse response);

    /**
     * Called after the SRCH2 search server completes an update task, which occurs
     * when either {@link com.srch2.android.http.service.Indexable#update(org.json.JSONArray)} or
     * {@link com.srch2.android.http.service.Indexable#update(org.json.JSONObject)} is called.
     * @param indexName the name of the index that was updated
     * @param response a representation of the RESTful update response
     */
    void onUpdateRequestComplete(String indexName,
                                 UpdateResponse response);

    /**
     * Called after the SRCH2 search server comes online after the call to {@link com.srch2.android.http.service.SRCH2Engine#onStart(android.content.Context)}
     * is made. When this callback method is triggered, it indicates all indexes are accessible
     * and ready for searching. The <code>Indexable</code> of each index should also have its state
     * information (ie {@link Indexable#getRecordCount()} updated to latest known values.
    */
    void onSRCH2ServiceReady();

    /**
     * Called after the SRCH2 search server completes an deletion task, which occurs
     * when {@link com.srch2.android.http.service.Indexable#delete(String)} is called.
     * @param indexName the name of the index that had deletions
     * @param response a representation of the RESTful deletion response
     */
    void onDeleteRequestComplete(String indexName,
                                 DeleteResponse response);

    /**
     * Called after the SRCH2 search server completes an record retrieval task, which occurs
     * when {@link com.srch2.android.http.service.Indexable#getRecordbyID(String)} is called.
     * @param indexName the name of the index that the record requested was retrieved from
     * @param response a representation of the RESTful record retrieval response, including the record
     *                 retrieved if found
     */
    void onGetRecordByIDComplete(String indexName,
                                 GetRecordResponse response);

}
