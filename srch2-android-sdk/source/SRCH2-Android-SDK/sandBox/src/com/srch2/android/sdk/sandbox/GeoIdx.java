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
package com.srch2.android.sdk.sandbox;

import android.util.Log;
import com.srch2.android.sdk.Field;
import com.srch2.android.sdk.Indexable;
import com.srch2.android.sdk.PrimaryKeyField;
import com.srch2.android.sdk.Schema;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

/**
 * Created by ashton on 8/22/2014.
 */
public class GeoIdx extends Indexable {

    @Override
    public void onIndexReady() {
        super.onIndexReady();

        if (getRecordCount() == 0) {
            insert(getRecords());
        }
    }

    public JSONArray getRecords() {
        JSONArray recordArray = new JSONArray();
        try {

            JSONObject jo = new JSONObject();
            jo.put(INDEX_FIELD_PK, 1);
            jo.put(INDEX_FIELD_LATITUDE, 10);
            jo.put(INDEX_FIELD_LONGITUDE, 10);
            jo.put(INDEX_FIELD_NAME, "name one");
            recordArray.put(jo);

            jo = new JSONObject();
            jo.put(INDEX_FIELD_PK, 2);
            jo.put(INDEX_FIELD_LATITUDE, 20);
            jo.put(INDEX_FIELD_LONGITUDE, 20);
            jo.put(INDEX_FIELD_NAME, "name two");
            recordArray.put(jo);

            jo = new JSONObject();
            jo.put(INDEX_FIELD_PK, 3);
            jo.put(INDEX_FIELD_LATITUDE, 30);
            jo.put(INDEX_FIELD_LONGITUDE, 30);
            jo.put(INDEX_FIELD_NAME, "name three");
            recordArray.put(jo);

        } catch (JSONException e) {
        }
        return recordArray;
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

    public static final String INDEX_NAME = "geo";
    public static final String INDEX_FIELD_PK = "id";
    public static final String INDEX_FIELD_NAME = "name";
    public static final String INDEX_FIELD_LATITUDE = "lat";
    public static final String INDEX_FIELD_LONGITUDE = "long";

    @Override
    public String getIndexName() {
        return INDEX_NAME;
    }

    @Override
    public Schema getSchema() {
        PrimaryKeyField pk = Field.createDefaultPrimaryKeyField(INDEX_FIELD_PK);
        Field f = Field.createSearchableField(INDEX_FIELD_NAME);
        return Schema.createGeoSchema(pk, INDEX_FIELD_LATITUDE, INDEX_FIELD_LONGITUDE, f);
    }

    static public ArrayList<SearchResultsAdapter.SearchResultItem> wrap(ArrayList<JSONObject> jsonResultsToWrap) {
        ArrayList<SearchResultsAdapter.SearchResultItem> newResults = new ArrayList<SearchResultsAdapter.SearchResultItem>();
        for (JSONObject jsonObject : jsonResultsToWrap) {
            Log.d("SEARCH RESULT OBJECT", jsonObject.toString());
            SearchResultsAdapter.SearchResultItem searchResult = null;
            try {

                JSONObject originalRecord = jsonObject.getJSONObject(Indexable.SEARCH_RESULT_JSON_KEY_RECORD);

                JSONObject highlightRecord = jsonObject.getJSONObject(Indexable.SEARCH_RESULT_JSON_KEY_HIGHLIGHTED);

                String title = highlightRecord.getString(INDEX_NAME);

                Log.d("Highlight", "title is " + title);

                if (title == null) {
                    title =  "null title";
                }

                searchResult = new SearchResultsAdapter.SearchResultItem(title, " ", " ");
            } catch (JSONException oops) {
                oops.printStackTrace();
                continue;
            }

            if (searchResult != null) {
                newResults.add(searchResult);
            }
        }
        return newResults;
    }
}
