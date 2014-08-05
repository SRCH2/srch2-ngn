package com.srch2.android.http.service;

import android.net.Uri;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

import java.io.UnsupportedEncodingException;

@RunWith(RobolectricTestRunner.class)
public class URLTest {

    static {
        PrepareEngine.prepareEngine();
        System.out.println("OAUTH KEY::::" + SRCH2Engine.conf.getAuthorizationKey());
    }

    static final String QUERY_KEY_WORDS = "android servi";
    static final String EXPECTED_QUERY_RAWSTRING = "android~ AND servi*~";
    static final String EXPECTED_QUERY_FORMAT = Uri.encode(EXPECTED_QUERY_RAWSTRING);
    static final IndexInternal TEST_CORE = SRCH2Engine.getConfig().indexesMap.values().iterator().next();
    static final String DEFAULT_CORE_NAME = TEST_CORE.getIndexCoreName();


    static final String ONE_DOC_ID = "id1";
    static final String[] MULTI_DOC_IDS = {"id2", "id3", "id4"};

    static final String HOST_URL = "http://" + SRCH2Configuration.HOSTNAME
            + ":" + SRCH2Engine.getConfig().getPort() + "/";
    static final String EXPECTED_DELETE_ONE = HOST_URL + DEFAULT_CORE_NAME
            + "/docs?OAuth=" + PrepareEngine.OAUTH + "&" + TEST_CORE.getConf().schema.uniqueKey + "=" + ONE_DOC_ID;

    static final String EXPECTED_INFO = HOST_URL + DEFAULT_CORE_NAME + "/info?OAuth=" + PrepareEngine.OAUTH;
    static final String EXPECTED_SAVE = HOST_URL + DEFAULT_CORE_NAME + "/save?OAuth=" + PrepareEngine.OAUTH;
    static final String EXPECTED_SEARCH_ALL = HOST_URL + "_all/search?OAuth=" + PrepareEngine.OAUTH + "&q="
            + EXPECTED_QUERY_FORMAT;
    static final String EXPECTED_SEARCH_ONE = HOST_URL + DEFAULT_CORE_NAME
            + "/search?OAuth=" + PrepareEngine.OAUTH + "&q=" + EXPECTED_QUERY_FORMAT;
    static final String EXPECTED_SHUTDOWN = HOST_URL + "_all/shutdown?OAuth=" + PrepareEngine.OAUTH;
    static final String EXPECTED_UPDATE = HOST_URL + DEFAULT_CORE_NAME
            + "/update?OAuth=" + PrepareEngine.OAUTH;
    static final String EXPECTED_INSERT = HOST_URL + DEFAULT_CORE_NAME
            + "/docs?OAuth=" + PrepareEngine.OAUTH;

    static final String EXPECTED_GETDOC = HOST_URL + DEFAULT_CORE_NAME
            + "/search?OAuth=" + PrepareEngine.OAUTH + "&docid=" + ONE_DOC_ID;


    @Test
    public void testDefaultQueryFormat() throws UnsupportedEncodingException {
        Assert.assertEquals(EXPECTED_QUERY_RAWSTRING,
                IndexInternal.formatDefaultQueryURL(QUERY_KEY_WORDS));
    }

    @Test
    public void testDeleteURL() {
        Assert.assertEquals(
                EXPECTED_DELETE_ONE,
                UrlBuilder.getDeleteUrl(SRCH2Engine.getConfig(),
                        TEST_CORE.getConf(), ONE_DOC_ID)
                        .toString());
    }

    @Test
    public void testInfoURL() {
        Assert.assertEquals(
                EXPECTED_INFO,
                UrlBuilder.getInfoUrl(SRCH2Engine.getConfig(),
                        TEST_CORE.getConf()).toString());
    }

    @Test
    public void testSaveURL() {
        Assert.assertEquals(
                EXPECTED_SAVE,
                UrlBuilder.getSaveUrl(SRCH2Engine.getConfig(),
                        TEST_CORE.getConf()).toString());
    }

    @Test
    public void testSearchAll() throws UnsupportedEncodingException {
        Assert.assertEquals(
                EXPECTED_SEARCH_ALL,
                UrlBuilder.getSearchUrl(SRCH2Engine.getConfig(), null,
                        IndexInternal.formatDefaultQueryURL(QUERY_KEY_WORDS))
                        .toString());
    }

    @Test
    public void testSearchOne() throws UnsupportedEncodingException {
        Assert.assertEquals(
                EXPECTED_SEARCH_ONE,
                UrlBuilder.getSearchUrl(SRCH2Engine.getConfig(),
                        TEST_CORE.getConf(),
                        IndexInternal.formatDefaultQueryURL(QUERY_KEY_WORDS))
                        .toString());
    }

    @Test
    public void testShutDown() {
        Assert.assertEquals(EXPECTED_SHUTDOWN,
                UrlBuilder.getShutDownUrl(SRCH2Engine.getConfig())
                        .toString());
    }

    @Test
    public void testUpdate() {
        Assert.assertEquals(
                EXPECTED_UPDATE,
                UrlBuilder.getUpdateUrl(SRCH2Engine.getConfig(),
                        TEST_CORE.getConf()).toString());
    }

    @Test
    public void testInsert() {
        Assert.assertEquals(
                EXPECTED_INSERT,
                UrlBuilder.getInsertUrl(SRCH2Engine.getConfig(),
                        TEST_CORE.getConf()).toString());
    }

    @Test
    public void testGetDocId() {
        Assert.assertEquals(
                EXPECTED_GETDOC,
                UrlBuilder.getGetDocUrl(SRCH2Engine.getConfig(),
                        TEST_CORE.getConf(), ONE_DOC_ID)
                        .toString());
    }
}
