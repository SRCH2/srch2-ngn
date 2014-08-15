package com.srch2.android.sdk;


import com.srch2.android.sdk.InfoResponse;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;


@Config(emulateSdk = 18)
@RunWith(RobolectricTestRunner.class)
public class ResponseTest {

    @Test
    public void InfoResponseTest() {
        String literal = "{\"engine_status\":{\"search_requests\":\"2\",\"write_requests\":\"100\",\"docs_in_index\":\"100\",\"last_merge\":\"07/24/14 00:53:46\",\"doc_count\":\"100\"}, \"version\":\"4.3.4\"}";
        InfoResponse infoResponse = new InfoResponse(200, literal);
        Assert.assertTrue(infoResponse.isValidInfoResponse());
        Assert.assertEquals("07/24/14 00:53:46", infoResponse.getLastMergeTime());
        Assert.assertEquals(100, infoResponse.getNumberOfDocumentsInTheIndex());
        Assert.assertEquals(2, infoResponse.getNumberOfSearchRequests());
        Assert.assertEquals(100, infoResponse.getNumberOfWriteRequests());
        Assert.assertEquals(200, infoResponse.getRESTfulHTTPStatusCode());

        literal = "{\"engine_status\":{\"search_requests\":\"2\",\"write_requests\":\"100\",\"docs_in_index\":\"100\",\"last_merge\":\"07/24/14 00:53:46\",\"doc_count\":\"0\"}, \"version\":\"4.3.4\"}";
        infoResponse = new InfoResponse(200, literal);
        Assert.assertTrue(infoResponse.isValidInfoResponse());
        Assert.assertEquals("07/24/14 00:53:46", infoResponse.getLastMergeTime());
        Assert.assertEquals(100, infoResponse.getNumberOfDocumentsInTheIndex());
        Assert.assertEquals(2, infoResponse.getNumberOfSearchRequests());
        Assert.assertEquals(100, infoResponse.getNumberOfWriteRequests());
        Assert.assertEquals(200, infoResponse.getRESTfulHTTPStatusCode());
    }

    @Test
    public void InvalidInfoResponseTest() {
        String literal = "{\"engine_status\":{\"search_requests\":\"2\",\"write_requests\":\"100\",\"docs_in_index\":\"100\",\"last_merge\":\"07/24/14 00:53:46\",\"doc_count\":\"100\"}, \"version\":\"4.3.4\"}";
        InfoResponse infoResponse = new InfoResponse(-1, literal);
        Assert.assertFalse(infoResponse.isValidInfoResponse());
        Assert.assertEquals(InfoResponse.INVALID_LAST_MERGE_TIME, infoResponse.getLastMergeTime());
        Assert.assertEquals(-1, infoResponse.getNumberOfDocumentsInTheIndex());
        Assert.assertEquals(-1, infoResponse.getNumberOfSearchRequests());
        Assert.assertEquals(-1, infoResponse.getNumberOfWriteRequests());
    }

    @Test
    public void MalformedInfoResponse() {
        String literal = "{\"engine_status\":{\"search_req";
        InfoResponse infoResponse = new InfoResponse(-1, literal);
        Assert.assertFalse(infoResponse.isValidInfoResponse());
        Assert.assertEquals(InfoResponse.INVALID_LAST_MERGE_TIME, infoResponse.getLastMergeTime());
        Assert.assertEquals(-1, infoResponse.getNumberOfDocumentsInTheIndex());
        Assert.assertEquals(-1, infoResponse.getNumberOfSearchRequests());
        Assert.assertEquals(-1, infoResponse.getNumberOfWriteRequests());
    }

    @Test
    public void DeleteResponseTest() {

    }

    @Test
    public void UpdateResponseTest() {

    }

}
