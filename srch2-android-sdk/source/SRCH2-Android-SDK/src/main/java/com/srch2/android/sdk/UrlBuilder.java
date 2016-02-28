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


    static URL getSearchUrl(SRCH2Configuration engineConf,
                            IndexDescription indexConf, String formatedSearchInput) {
        URL url = null;
        try {
            formatedSearchInput = getURLEncodedString(formatedSearchInput);
            if (indexConf == null) {
                url = new URL(engineConf.getUrlString()
                        + URL_SEARCH_ALL_PATH_PARAMETER + engineConf.getAuthorizationKey() + "&" + formatedSearchInput);

            } else {
                url = new URL(engineConf.getUrlString()
                        + indexConf.getIndexName() + URL_QUERY_PATH_PARAMETER + engineConf.getAuthorizationKey() + "&"
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
            Cat.ex("UrlBuilder", "gettingDeleteURL", ignore);
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
        return Uri.encode(sentence, "&=:*$.~");
    }
}
