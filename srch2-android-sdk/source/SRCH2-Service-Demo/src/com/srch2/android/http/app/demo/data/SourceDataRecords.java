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
