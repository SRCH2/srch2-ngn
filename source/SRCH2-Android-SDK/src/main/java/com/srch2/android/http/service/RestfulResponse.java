package com.srch2.android.http.service;


// Note for code clarity, all subclass implementations of RestfulResponse are static nested classes. 

/**
 * An abstract class for mapping the parsed results of an http connection result
 * from the SRCH2 restful server to field members of the corresponding,
 * implementing subclass. Users of any subclass may always access the field
 * members <code>httpResponseCode</code> (to see the response code) and
 * <code>restfulResponseLiteral</code> (to see the raw result as it was supplied
 * by the SRCH2 restful server) and are encouraged to at least check the value
 * of <code>httpResponseCode</code> and make sure it is <b>not</b> equal to
 * <code>FAILED_TO_CONNECT_RESPONSE_CODE</code> or -1 before accessing addition
 * fields.
 * <p/>
 * See the subclass <code>InfoResponse</code> for an example of how this class
 * can be used and accessed.
 * <p/>
 */
abstract class RestfulResponse {

    /**
     * The value <code>httpResponseCode</code> will be set to when the http
     * connection, responsible for generating the specific
     * <code>RestfulResponse</code>, fails to connect.
     * <p/>
     * Has the <b>constant</b> value of -1.
     */
    public static final int FAILED_TO_CONNECT_RESPONSE_CODE = -1;


    public static final String IRRECOVERABLE_NETWORK_ERROR_MESSAGE = "Connection failed without known cause.";

    /**
     * Indicates the InfoResponse was unable to be formed from the JSON response from the SRCH2 server upon performing an info command. Has the <bold>constant</bold> value <code>-1</code>.
     */
    public static final int INVALID_COUNT = -1;


    /**
     * The response code as it was supplied by the SRCH2 http restful server
     * upon performing the http connection. If the http connection fails to
     * connect, <code>httpResponseCode</code> will always be set to the value of
     * <code>FAILED_TO_CONNECT_RESPONSE_CODE</code> which has the
     * <b>constant</b> value of -1.
     */
    public final int httpResponseCode;

    /**
     * The literal response as it was supplied by the SRCH2 http restful server
     * upon performing the http connection. If the http connection fails to
     * connect, <code>restfulResponseLiteral</code> will always be set to the
     * value of <code>FAILED_TO_CONNECT_RESPONSE_CODE</code> which has the
     * <b>constant</b> value of "invalid response".
     */
    public final String restfulResponseLiteral;

    RestfulResponse(int theHttpResponseCode, String theRestfulResponseLiteral) {
        httpResponseCode = theHttpResponseCode;

        if (theRestfulResponseLiteral != null && !theRestfulResponseLiteral.equals(IRRECOVERABLE_NETWORK_ERROR_MESSAGE)) {
            restfulResponseLiteral = theRestfulResponseLiteral;
        } else {
            restfulResponseLiteral = IRRECOVERABLE_NETWORK_ERROR_MESSAGE;
        }
    }
}
