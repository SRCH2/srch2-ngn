package com.srch2.android.sdk;


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
        InternalInfoResponse infoResponse = new InternalInfoResponse(200, literal);
        Assert.assertTrue(infoResponse.isValidInfoResponse);
        Assert.assertEquals("07/24/14 00:53:46", infoResponse.lastMergeTime);
        Assert.assertEquals(100, infoResponse.numberOfDocumentsInTheIndex);
        Assert.assertEquals(2, infoResponse.numberOfSearchRequests);
        Assert.assertEquals(100, infoResponse.numberOfWriteRequests);

        literal = "{\"engine_status\":{\"search_requests\":\"2\",\"write_requests\":\"100\",\"docs_in_index\":\"100\",\"last_merge\":\"07/24/14 00:53:46\",\"doc_count\":\"0\"}, \"version\":\"4.3.4\"}";
        infoResponse = new InternalInfoResponse(200, literal);
        Assert.assertTrue(infoResponse.isValidInfoResponse);
        Assert.assertEquals("07/24/14 00:53:46", infoResponse.lastMergeTime);
        Assert.assertEquals(100, infoResponse.numberOfDocumentsInTheIndex);
        Assert.assertEquals(2, infoResponse.numberOfSearchRequests);
        Assert.assertEquals(100, infoResponse.numberOfWriteRequests);
    }

    @Test
    public void InvalidInfoResponseTest() {
        String literal = "{\"engine_status\":{\"search_requests\":\"2\",\"write_requests\":\"100\",\"docs_in_index\":\"100\",\"last_merge\":\"07/24/14 00:53:46\",\"doc_count\":\"100\"}, \"version\":\"4.3.4\"}";
        InternalInfoResponse infoResponse = new InternalInfoResponse(-1, literal);
        Assert.assertFalse(infoResponse.isValidInfoResponse);
        Assert.assertEquals(InternalInfoResponse.INVALID_LAST_MERGE_TIME, infoResponse.lastMergeTime);
        Assert.assertEquals(-1, infoResponse.numberOfDocumentsInTheIndex);
        Assert.assertEquals(-1, infoResponse.numberOfSearchRequests);
        Assert.assertEquals(-1, infoResponse.numberOfWriteRequests);
    }

    @Test
    public void MalformedInfoResponse() {
        String literal = "{\"engine_status\":{\"search_req";
        InternalInfoResponse infoResponse = new InternalInfoResponse(-1, literal);
        Assert.assertFalse(infoResponse.isValidInfoResponse);
        Assert.assertEquals(InternalInfoResponse.INVALID_LAST_MERGE_TIME, infoResponse.lastMergeTime);
        Assert.assertEquals(-1, infoResponse.numberOfDocumentsInTheIndex);
        Assert.assertEquals(-1, infoResponse.numberOfSearchRequests);
        Assert.assertEquals(-1, infoResponse.numberOfWriteRequests);
    }

    @Test
    public void DeleteResponseTest() {

    }

    @Test
    public void UpdateResponseTest() {

    }

}
