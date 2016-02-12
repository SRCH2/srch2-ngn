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

import org.json.JSONException;
import org.json.JSONObject;

public class DumbIndex extends Indexable {
    public static final String INDEX_NAME = "dumb-index";
    public static final String INDEX_FIELD_NAME_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_NAME_TITLE = "title";

    @Override
    public String getIndexName() {
        return INDEX_NAME;
    }

    @Override
    public Schema getSchema() {
        return Schema.createSchema(
                Field.createDefaultPrimaryKeyField(INDEX_FIELD_NAME_PRIMARY_KEY),
                Field.createSearchableField(INDEX_FIELD_NAME_TITLE));
    }

    @Override
    public void onInsertComplete(int success, int failed, String JSONResponse) {
        super.onInsertComplete(success, failed, JSONResponse);
    }

    @Override
    public void onUpdateComplete(int success, int upserts, int failed, String JSONResponse) {
        super.onUpdateComplete(success, upserts, failed, JSONResponse);
    }

    @Override
    public void onDeleteComplete(int success, int failed, String JSONResponse) {
        super.onDeleteComplete(success, failed, JSONResponse);
    }

    @Override
    public void onGetRecordComplete(boolean success, JSONObject record, String JSONResponse) {
        super.onGetRecordComplete(success, record, JSONResponse);
    }

    @Override
    public void onIndexReady() {
        super.onIndexReady();
    }

    static JSONObject getRecord(String id, String title) {
        JSONObject jo = new JSONObject();
        try {
            jo.put(INDEX_FIELD_NAME_PRIMARY_KEY, id);
            jo.put(INDEX_FIELD_NAME_TITLE, title);
        } catch (JSONException e) {
        }
        return jo;
    }
}
