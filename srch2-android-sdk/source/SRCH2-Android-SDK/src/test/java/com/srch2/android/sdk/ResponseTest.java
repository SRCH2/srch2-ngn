/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
