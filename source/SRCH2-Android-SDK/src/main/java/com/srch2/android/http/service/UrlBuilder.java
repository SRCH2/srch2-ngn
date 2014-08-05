package com.srch2.android.http.service;

import android.net.Uri;

import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLEncoder;

class UrlBuilder {

    private static final String URL_QUERY_PATH_PARAMETER = "/search?OAuth="; // q=";
    private static final String URL_GETDOC_PATH_PARAMETER = "/search?OAuth="; // docid=";
    private static final String URL_DOCTS_PATH_PARAMETER = "/docs?OAuth=";
    private static final String URL_INFO_PATH_PARAMETER = "/info?OAuth=";
    private static final String URL_SAVE_PATH_PARAMETER = "/save?OAuth=";
    private static final String URL_UPDATE_PATH_PARAMETER = "/update?OAuth=";
    private static final String URL_SEARCH_ALL_PATH_PARAMETER = "_all/search?OAuth=";// q=";
    private static final String URL_SHUTDOWN_PATH_PARAMETER = "_all/shutdown?OAuth=";

    static URL getInfoUrl(SRCH2Configuration engineConf,
                          IndexDescription indexConf) {
        URL url = null;
        try {
            url = new URL(engineConf.getUrlString() + indexConf.getIndexName()
                    + URL_INFO_PATH_PARAMETER
                    + engineConf.getAuthorizationKey());
        } catch (MalformedURLException ignore) {
        }
        return url;
    }

    /**
     * Create the URL for the query string. The <code>formatedQueryString</code>
     * is
     *
     * @param engineConf
     * @param indexConf
     * @param formatedSearchInput "&fuzzy=true&rows=5"
     * @return
     */
    static URL getSearchUrl(SRCH2Configuration engineConf,
                            IndexDescription indexConf, String formatedSearchInput) {
        URL url = null;
        try {
            formatedSearchInput = getURLEncodedString(formatedSearchInput);
            if (indexConf == null) {
                url = new URL(engineConf.getUrlString()
                        + URL_SEARCH_ALL_PATH_PARAMETER + engineConf.getAuthorizationKey() + "&q=" + formatedSearchInput);

            } else {
                url = new URL(engineConf.getUrlString()
                        + indexConf.getIndexName() + URL_QUERY_PATH_PARAMETER + engineConf.getAuthorizationKey() + "&q="
                        + formatedSearchInput);
            }
        } catch (MalformedURLException ignore) {
        }
        return url;
    }

    static URL getGetDocUrl(SRCH2Configuration engineConf,
                            IndexDescription indexConf, String id) {
        URL url = null;
        id = getURLEncodedUTF8String(id);
        try {
            url = new URL(engineConf.getUrlString() + indexConf.getIndexName()
                    + URL_GETDOC_PATH_PARAMETER
                    + engineConf.getAuthorizationKey() + "&docid=" + id);
        } catch (MalformedURLException ignore) {
        }
        return url;
    }

    static URL getSaveUrl(SRCH2Configuration engineConf,
                          IndexDescription indexConf) {
        URL url = null;
        try {
            url = new URL(engineConf.getUrlString() + indexConf.getIndexName()
                    + URL_SAVE_PATH_PARAMETER
                    + engineConf.getAuthorizationKey());
        } catch (MalformedURLException ignore) {
        }
        return url;
    }

    static URL getUpdateUrl(SRCH2Configuration engineConf,
                            IndexDescription indexConf) {
        URL url = null;
        try {
            url = new URL(engineConf.getUrlString() + indexConf.getIndexName()
                    + URL_UPDATE_PATH_PARAMETER
                    + engineConf.getAuthorizationKey());
        } catch (MalformedURLException ignore) {
        }
        return url;
    }

    static URL getDeleteUrl(SRCH2Configuration engineConf,
                            IndexDescription indexConf, String id1, String... restIds) {
        URL url = null;
        try {
            StringBuilder ids = new StringBuilder(getURLEncodedUTF8String(id1));
            if (restIds != null && restIds.length > 0) {
                ids = new StringBuilder().append('{').append(getURLEncodedUTF8String(id1));
                for (String id : restIds) {
                    ids.append(',').append(getURLEncodedUTF8String(id));
                }
                ids.append('}');

            }
            String urlIds = ids.toString();

            url = new URL(engineConf.getUrlString() + indexConf.getIndexName()
                    + URL_DOCTS_PATH_PARAMETER
                    + engineConf.getAuthorizationKey() + "&" + indexConf.schema.uniqueKey + "=" + urlIds);
        } catch (MalformedURLException ignore) {
            ignore.printStackTrace();
        }
        return url;
    }

    static URL getInsertUrl(SRCH2Configuration engineConf,
                            IndexDescription indexConf) {
        URL url = null;
        try {
            url = new URL(engineConf.getUrlString() + indexConf.getIndexName()
                    + URL_DOCTS_PATH_PARAMETER
                    + engineConf.getAuthorizationKey());
        } catch (MalformedURLException ignore) {
        }
        return url;
    }

    static URL getShutDownUrl(SRCH2Configuration engineConf) {
        URL url = null;
        try {
            url = new URL(engineConf.getUrlString()
                    + URL_SHUTDOWN_PATH_PARAMETER
                    + engineConf.getAuthorizationKey());
        } catch (MalformedURLException ignore) {
        }
        return url;
    }

    static String getURLEncodedUTF8String(String utf8) {
        try {
            return URLEncoder.encode(utf8, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            throw new IllegalArgumentException("Unbelievable！ UTF-8 encoding is not supported! A" +
                    "re you in android platform?");
        }
    }

    static String getURLEncodedString(String sentence){
        return Uri.encode(sentence, "&=:*$.~^");
    }
}
