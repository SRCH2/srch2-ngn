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
