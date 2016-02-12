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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

import android.content.Context;

public class IncrementalState implements IncrementalUpdateObserver {
	
	private static final String TAG = "IncrementalState";
	
	private HashMap<Long, IncrementalValuePair> latestIncrementalSnapshot;
	
	private IncrementalObserver sourceContentChangedObserver;
	
	private IncrementalStateFunctor updateIndexFunctor;
	
	public IncrementalState(Object contentSourceKey, IncrementalStateFunctor theUpdateFunctor) {
		updateIndexFunctor = theUpdateFunctor;
		sourceContentChangedObserver = new ContentProviderObserver(contentSourceKey, this);
		latestIncrementalSnapshot = new HashMap<Long, IncrementalValuePair>(0);
	}
	
	public void registerObserver(final Context context) {
		if (sourceContentChangedObserver != null) { sourceContentChangedObserver.registerObserver(context); }
	}
	
	public void unregisterObserver(final Context context) {
		if (sourceContentChangedObserver != null) { sourceContentChangedObserver.unregisterObserver(context); }
	}
	
	public void getAndSetLatestSnapShot(ArrayList<IncrementalValuePair> incrementalValuePairs) {
		latestIncrementalSnapshot = new HashMap<Long, IncrementalValuePair>();
		for (IncrementalValuePair p : incrementalValuePairs) {
			latestIncrementalSnapshot.put(p.id, p);
		}
	}
	
	public HashSet<IncrementalValuePair> getSnapShotAsHashSet() {
		return new HashSet<IncrementalValuePair>(latestIncrementalSnapshot.values());
	}
	
	public ArrayList<IncrementalValuePair> getSnapShotAsArrayList() {
		return new ArrayList<IncrementalValuePair>(latestIncrementalSnapshot.values());
	}
	
	public void removeSingle(long idToRemove) {
		if (latestIncrementalSnapshot != null) {
			latestIncrementalSnapshot.remove(idToRemove);
		}
	}
	
	public void addSingle(long idToAdd, int versionOfIdToAdd) {
		if (latestIncrementalSnapshot != null) {
			latestIncrementalSnapshot.put(idToAdd, new IncrementalValuePair(idToAdd, versionOfIdToAdd));
		}
	}
	
	public void addSingle(IncrementalValuePair newPair) {
		if (latestIncrementalSnapshot != null && newPair != null) {
			latestIncrementalSnapshot.put(newPair.id, newPair);
		}
	}
	
	public int size() {
		return latestIncrementalSnapshot == null ? 0 : latestIncrementalSnapshot.size();
	}

	@Override
	public void onIncrementalDifferenceDetected() {
		updateIndexFunctor.updateIndex();
	}
}
