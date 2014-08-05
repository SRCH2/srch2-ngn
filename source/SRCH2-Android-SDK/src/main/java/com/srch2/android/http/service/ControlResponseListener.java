package com.srch2.android.http.service;

import java.util.HashMap;

/**
 * This interface contains the call-back methods for the control related restful
 * requests of the SRCH2 http server: upon completion of the specific request,
 * the corresponding call-back will be called passing the corresponding subclass
 * of <code>RestfulResponse</code>. Each <code>RestfulResponse</code> subclass
 * will always contain the <code>httpResponseCode</code> and
 * <code>restfulResponseLiteral</code> as it is returned by the SRCH2 server.
 * Implementors of this class can either parse the field member
 * <code>restfulResponseLiteral</code> of the specific subclass passed, or can
 * review the specific field members of each subclass and access them directly
 * to avoid having to parse the response literal. Implementors are encouraged to
 * check that the field member <code>httpResponseCode</code> is not equal to
 * <code>RestfulResponse.FAILED_TO_CONNECT_RESPONSE_CODE</code> (equal to -1)
 * before parsing or accessing any other field members.
 * <p/>
 * Users of this class <b>should</b> register their implementation of this
 * interface to the appropriate <code>IndexInternal</code> object by calling
 * <code>setControlResponseListener(ControlResponseListener observer)</code>,
 * although is unnecessary to do so if users are not interested in receiving
 * control response call-backs for their <code>IndexInternal</code> object.
 */
public interface ControlResponseListener {

    /**
     * Will be called when an /info request is completed on an index.
     *
     * @param response an object wrapping the info response literal
     */
    void onInfoRequestComplete(String targetIndexName,
                               InfoResponse response);

    /**
     * Will be called when an insert record(s) request is completed on an index.
     *
     * @param response an object wrapping the insert response literal
     */
    void onInsertRequestComplete(String targetIndexName,
                                 InsertResponse response);

    /**
     * Will be called when an update record(s) request is completed on an index.
     *
     * @param response an object wrapping the update response literal
     */
    void onUpdateRequestComplete(String targetIndexName,
                                 UpdateResponse response);

    /**
     * Will be called when a delete record(s) request is completed on an index.
     *
     */
    void onSRCH2ServiceReady(HashMap<String, InfoResponse> indexesToInfoResponseMap);

    /**
     * Will be called when an delete record(s) request is completed on an index.
     *
     * @param targetIndexName
     * @param response an object wrapping the record literal
     */
    void onDeleteRequestComplete(String targetIndexName,
                                 DeleteResponse response);

    /**
     * Will be called when a get one record by id request is completed on an
     * index.
     *
     * @param targetIndexName
     * @param response an object wrapping the record literal
     */
    void onGetRecordByIDComplete(String targetIndexName,
                                 GetRecordResponse response);

}
