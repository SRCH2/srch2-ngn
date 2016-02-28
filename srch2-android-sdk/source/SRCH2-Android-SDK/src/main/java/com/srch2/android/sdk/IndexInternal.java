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


import org.json.JSONArray;
import org.json.JSONObject;

import java.net.URL;
import java.util.concurrent.atomic.AtomicBoolean;

final class IndexInternal {
    private static final String TAG = "IndexInternal";

    final IndexDescription indexDescription;
    private SearchTask currentSearchTask = null;

    AtomicBoolean isDirty;

    IndexInternal(IndexDescription description) {
        isDirty = new AtomicBoolean(false);
        this.indexDescription = description;
    }

    static String formatDefaultQueryURL(String queryString) {
        String[] tokens = queryString.split("\\s+");
        StringBuilder rawSearchInput = new StringBuilder("q=");
        for (int i = 0; i < tokens.length - 1; i++) {
            tokens[i] = UrlBuilder.getURLEncodedUTF8String(tokens[i]);
            rawSearchInput.append(tokens[i]).append("~").append(" AND ");
        }
        rawSearchInput.append(tokens[tokens.length - 1]).append("*~");
        return rawSearchInput.toString();
    }

    IndexDescription getConf() {
        return indexDescription;
    }

    String getIndexCoreName() {
        return indexDescription.name;
    }

    void insert(JSONObject record) {
            InsertTask insertTask = new InsertTask(UrlBuilder.getInsertUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(), record);
            HttpTask.addToQueue(insertTask);
            SRCH2Engine.isChanged.set(true);
            isDirty.set(true);
    }

    void insert(JSONArray records) {
            URL url = UrlBuilder.getInsertUrl(SRCH2Engine.getConfig(),
                    indexDescription);

            InsertTask insertTask = new InsertTask(url, getIndexCoreName(),
                    records);
            HttpTask.addToQueue(insertTask);
            SRCH2Engine.isChanged.set(true);
            isDirty.set(true);
    }

    void update(JSONObject record) {
            UpdateTask updateTask = new UpdateTask(UrlBuilder.getUpdateUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(), record);
            HttpTask.addToQueue(updateTask);
            SRCH2Engine.isChanged.set(true);
            isDirty.set(true);
    }

    void update(JSONArray records) {
            UpdateTask updateTask = new UpdateTask(UrlBuilder.getUpdateUrl(
                    SRCH2Engine.getConfig(), indexDescription),
                    getIndexCoreName(), records);
            HttpTask.addToQueue(updateTask);
            SRCH2Engine.isChanged.set(true);
            isDirty.set(true);
    }

    void delete(String id) {
            DeleteTask deleteTast = new DeleteTask(UrlBuilder.getDeleteUrl(
                    SRCH2Engine.getConfig(), indexDescription, id),
                    getIndexCoreName());
            HttpTask.addToQueue(deleteTast);
            SRCH2Engine.isChanged.set(true);
            isDirty.set(true);
    }

    void searchRawString(String rawString) {
        SRCH2Engine.lastQuery.set(new SRCH2Engine.IndexQueryPair(indexDescription.getIndexName(),
                rawString));
        if (currentSearchTask != null) {
            currentSearchTask.cancel();
        }
        currentSearchTask = new SearchTask(

                UrlBuilder.getSearchUrl(SRCH2Engine.getConfig(), indexDescription,
                        rawString), getIndexCoreName(),
                SRCH2Engine.getSearchResultsObserver());

        HttpTask.addToQueue(currentSearchTask);
    }

    void search(String searchInput) {
        if (SRCH2Engine.validateSearchInput(searchInput)) {
            searchRawString(formatDefaultQueryURL(searchInput));
        }
    }

    void advancedSearch(Query query) {
        if (query == null || SRCH2Engine.getConfig() == null) {
            return;
        }
        searchRawString(query.toString());
    }

    void getRecordbyID(String id) {
        if (id == null || id.length() < 1) {
            return;
        }
        HttpTask.addToQueue(new GetRecordTask(UrlBuilder.getGetDocUrl(
                SRCH2Engine.getConfig(), indexDescription, id), this
                .getIndexCoreName()));
    }

}
