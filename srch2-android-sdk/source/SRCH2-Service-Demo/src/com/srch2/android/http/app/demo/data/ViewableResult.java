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
