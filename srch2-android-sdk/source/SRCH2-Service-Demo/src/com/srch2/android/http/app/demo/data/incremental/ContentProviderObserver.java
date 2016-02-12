package com.srch2.android.http.app.demo.data.incremental;

import android.content.Context;
import android.net.Uri;

public class ContentProviderObserver extends IncrementalObserver {

	private Uri contentUri;
	
	public ContentProviderObserver(Object dataContentListenerHandle, IncrementalUpdateObserver contentChangedObserver) {
		super(contentChangedObserver);
		try {
			contentUri = (Uri) dataContentListenerHandle;
		} catch (ClassCastException cce) {
			contentUri = null;
		}
	}

	@Override
	protected void registerObserver(final Context context) {
		if (!isRegistered) {
			isRegistered = true;
			context.getContentResolver().registerContentObserver(contentUri, true, this);
		}
	}

	@Override
	protected void unregisterObserver(final Context context) {
		if (isRegistered) {
			context.getContentResolver().unregisterContentObserver(this);
			isRegistered = false;
		}
	}
}