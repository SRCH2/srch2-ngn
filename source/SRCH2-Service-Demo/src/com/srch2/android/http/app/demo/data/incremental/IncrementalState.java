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