package com.srch2.android.http.app.demo.data.contacts;

import java.util.HashSet;

import com.srch2.android.http.app.demo.data.incremental.IncrementalValuePair;

public class ContentProviderConstants {

	public static final String[] NO_SELECTION_ARGS = new String[] { };
	
	final public static String[] getSelectedIdArgs(final HashSet<IncrementalValuePair> recordIdsToSelect) {
		String[] selectionArgs = new String[recordIdsToSelect.size()];
		int i = 0;
		for (IncrementalValuePair p : recordIdsToSelect) {
			selectionArgs[i] = String.valueOf(p.id);
			++i;
		}
		return selectionArgs;
	}
}
