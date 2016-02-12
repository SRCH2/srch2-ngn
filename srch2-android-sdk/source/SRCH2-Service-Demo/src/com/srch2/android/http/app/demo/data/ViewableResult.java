package com.srch2.android.http.app.demo.data;

import java.util.HashMap;

import org.json.JSONObject;

import com.srch2.android.http.app.demo.DefaultUiResources;
import com.srch2.android.http.app.demo.IndexType;

import android.content.Context;
import android.graphics.Bitmap;

public abstract class ViewableResult {


	public RowClickFunctor rowClickFunction = null;
	public void onRowClick(Context context, ViewableResult clickedViewableResult) { } 

	public RowButtonClickFunctor rowButtonClickFunction = null;
	public HashMap<Integer, Bitmap> rowButtonClickMap = null;
	public void onRowButtonClick(Context context, int rowButtonId, ViewableResult clickedViewableResult) { }
	
	public boolean isSwippable = false;
	public void onSwipeLeft(Context context, ViewableResult clickedViewableResult) { }
	public void onSwipeRight(Context context, ViewableResult clickedViewableResult) { }
	
	
	
	public abstract Bitmap getIcon(Context context);
	
	public static final String INVALID_VALUE = "[INVALID_VALUE]";
	
	public String title;
	public int indicatorColor;
	public Bitmap icon;
	
	public String lookupKey;
	
	public IndexType indexType;
	
	
	
	
	protected ViewableResult(IndexType theIndexType, String theLookupKey, String theTitle) {
		indexType = theIndexType;
		title = theTitle != null ? theTitle : INVALID_VALUE;
		lookupKey = theLookupKey != null ? theLookupKey : INVALID_VALUE;
		
		indicatorColor = DefaultUiResources.getDefaultIndicatorColor(indexType);
		icon = DefaultUiResources.getDefaultIcon(indexType);
		
		
		
	}
	
	
	
	
	
	
	
	@Override
	public String toString() {
		return title;
	}
	
	
	
	
	
	
	public static abstract class Wrapper {
		public abstract ViewableResult wrap(JSONObject jsonRecord);
	}
	
	public static abstract class RowClickFunctor {
		public abstract void onClick(Context context, ViewableResult clickedViewableResult);
	}
	
	public static abstract class RowButtonClickFunctor {
		public abstract void onClick(Context context, int rowButtonId, ViewableResult clickedViewableResult);
	}
}