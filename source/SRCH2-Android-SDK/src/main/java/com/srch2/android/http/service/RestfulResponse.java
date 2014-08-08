package com.srch2.android.http.service;

abstract class RestfulResponse {
    /**
     * Indicates the SRCH2 search server was unavailable to process a request.
     * <br><br>
     * Has the <b>constant</b> value of <code>-1</code>.
     */
    public static final int FAILED_TO_CONNECT_RESPONSE_CODE = -1;

    /**
     * Indicates the SRCH2 search server was unavailable to process a request.
     * <br><br>
     * Has the <b>constant</b> value of <code>"Connection failed without known cause"</code>.
     */
    public static final String IRRECOVERABLE_NETWORK_ERROR_MESSAGE = "Connection failed without known cause.";

    private final int httpResponseStatusCode;
    /**
     * For each task executed by the SRCH2 search server, the server will return
     * a HTTP status code of the completed request. Calling this method returns
     * this status code.
     * @return the HTTP status code as returned by the SRCH2 search server
     */
    public final int getRESTfulHTTPStatusCode() { return httpResponseStatusCode; }

    private final String restfulResponseLiteral;
    /**
     * For each task executed by the SRCH2 search server, the server will return
     * a RESTful response in for the form a <code>JSONObject</code>. Calling this
     * method will return the literal of this <code>JSONObject</code>.
     * <br><br>
     * This method may return the value of <code>IRRECOVERABLE_NETWORK_ERROR_MESSAGE</code>
     * if the SRCH2 search server was unavailable.
     * @return the RESTful response literal as returned by the SRCH2 search server
     */
    public final String getRESTfulResponseLiteral() {
        return restfulResponseLiteral == null ? "no response found" : restfulResponseLiteral;
    }

    RestfulResponse(int theHttpResponseCode, String theRestfulResponseLiteral) {
        httpResponseStatusCode = theHttpResponseCode;

        if (theRestfulResponseLiteral != null && !theRestfulResponseLiteral.equals(IRRECOVERABLE_NETWORK_ERROR_MESSAGE)) {
            restfulResponseLiteral = theRestfulResponseLiteral;
        } else {
            restfulResponseLiteral = IRRECOVERABLE_NETWORK_ERROR_MESSAGE;
        }
    }
}
