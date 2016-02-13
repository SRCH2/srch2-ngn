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
package com.srch2.android.http.app.demo.data;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.srch2.android.http.app.demo.data.incremental.IncrementalValuePair;

public class SourceDataRecords {
	public ArrayList<IncrementalValuePair> incrementalValuePairs;
	public HashMap<Long, JSONObject> jsonRecordIdMap;
	
	final static private JSONObject deletionJsonObject = new JSONObject();


	public SourceDataRecords(int estimatedSize) {
		incrementalValuePairs = new ArrayList<IncrementalValuePair>(estimatedSize);
		jsonRecordIdMap = new HashMap<Long, JSONObject>(estimatedSize);
	}

	public SourceDataRecords(HashSet<IncrementalValuePair> recordsToPrepareForDeletion) {
		this(recordsToPrepareForDeletion.size());
		
		for (IncrementalValuePair p : recordsToPrepareForDeletion) {
			incrementalValuePairs.add(p);
			jsonRecordIdMap.put(p.id, deletionJsonObject);
		}
	}
	
	public void add(long theId, JSONObject theJsonRecord, IncrementalValuePair theIncrementalValuePair) {
		incrementalValuePairs.add(theIncrementalValuePair);
		jsonRecordIdMap.put(theId, theJsonRecord);
	}
	
	public int size() {
		return incrementalValuePairs == null ? 0 : incrementalValuePairs.size();
	}
	
	public JSONArray getJsonArray() {
		if (jsonRecordIdMap != null && jsonRecordIdMap.size() > 0) {
			Collection<JSONObject> records = jsonRecordIdMap.values();
			JSONArray jarray = new JSONArray();
			for (JSONObject jo : records) {
				jarray.put(jo);
			} 
			return jarray;
		} else {
			return null;
		}
	}
	
	public int[] getDeleteIds() {
		int[] deleteIds = new int[jsonRecordIdMap.size()];
		int i = 0;
		for (Long id : jsonRecordIdMap.keySet()) {
			deleteIds[i] = Integer.valueOf(String.valueOf(id));
			++i;
		}
		return deleteIds;
	}
}
