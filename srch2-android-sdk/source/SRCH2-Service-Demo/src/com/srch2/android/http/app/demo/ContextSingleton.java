package com.srch2.android.http.app.demo;

import java.lang.ref.WeakReference;

import android.content.Context;

import java.lang.ref.WeakReference;

import android.content.Context;

public class ContextSingleton {

	private static ContextSingleton instance;
	
	static WeakReference<Context> contextReference;

	private ContextSingleton(Context context) {
		contextReference = new WeakReference<Context>(context);
	}
	
	static public Context getContext() {
		if (contextReference != null) {
			Context c = contextReference.get();
			if (c != null) {
				return c;
			}
		}
		return null;
	}
	
	static public void link(Context context) {
		unlink();
		instance = new ContextSingleton(context.getApplicationContext());
	}
	
	static public void unlink() {
		if (instance != null) {
			contextReference.clear();
			instance = null;
		}
	}
	
}
