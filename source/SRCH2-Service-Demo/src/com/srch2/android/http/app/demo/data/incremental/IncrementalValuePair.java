package com.srch2.android.http.app.demo.data.incremental;

import java.util.HashMap;
import java.util.HashSet;

public class IncrementalValuePair {
	public static final String TAG = "ivp-super";

	public long id;
	public int version;

	
	public IncrementalValuePair(long theId, int theVersion) {
		id = theId;
		version = theVersion;
	}

	/** Resolves the incremental differences by adding all of one set and then removing all of the other. <br>
	 * Additions start with currentIncrementalData, and deletions start with latestIncrementalSnapshot.  
	 * @return the differences to effect: the key is true for all additions, false for all deletions */
	public static HashMap<Boolean, HashSet<IncrementalValuePair>> resolveIncrementalDifference(
														HashSet<IncrementalValuePair> currentIncrementalData,
																		HashSet<IncrementalValuePair> latestIncrementalSnapshot) {
		
		HashSet<IncrementalValuePair> additions = new HashSet<IncrementalValuePair>();
		additions.addAll(currentIncrementalData);
		additions.removeAll(latestIncrementalSnapshot);
		
		HashSet<IncrementalValuePair> deletions = new HashSet<IncrementalValuePair>();
		deletions.addAll(latestIncrementalSnapshot);
		deletions.removeAll(currentIncrementalData);
		
		HashMap<Boolean, HashSet<IncrementalValuePair>> results = new HashMap<Boolean, HashSet<IncrementalValuePair>>();
		results.put(true, additions);
		results.put(false, deletions);
		
		return results;
	}
	
	@Override
	public boolean equals(Object o) {
		if (o != null && o.getClass() == IncrementalValuePair.class) {
			IncrementalValuePair other = (IncrementalValuePair) o;
			if (other.id == id && other.version == version) {
				return true;
			}
		} 
		return false;
	}

	@Override
	public int hashCode() { // the left hand side of the + operator is the canonical hashcode function for longs in java  
		return (int) ((int) (id ^ (id >>> 32)) + version);
	}
	
	@Override
	public String toString() {
		return "IVP: id:[ " + id + " ] version:[ " + version + " ]"; 
	}
}