package com.srch2.android.http.app.demo;

import java.util.HashMap;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.DisplayMetrics;
import android.util.TypedValue;


public class DefaultUiResources {
	
	private static int srch2IndicatorColorCode = -12345;
	private static Bitmap srch2Icon = null;
	private static Bitmap transparentSquare = null;
	
	private static HashMap<IndexType, Bitmap> defaultIcon = null;
	private static HashMap<IndexType, Integer> defaultIndicatorColor = null;
	private static HashMap<Integer, Bitmap> contactRowButton = null;
	
	public static void init(Context context) {
		if (defaultIndicatorColor == null) {
			defaultIndicatorColor = new HashMap<IndexType, Integer>();
			defaultIndicatorColor.put(IndexType.Contacts, context.getResources().getColor(R.color.row_indicator_contacts));
		}
		
		if (defaultIcon == null) {
			defaultIcon = new HashMap<IndexType, Bitmap>();
			defaultIcon.put(IndexType.Contacts, BitmapFactory.decodeResource(context.getResources(), R.drawable.row_icon_default_contacts_grey));
		}
		
		if (contactRowButton == null) {
			contactRowButton = new HashMap<Integer, Bitmap>(3);
			contactRowButton.put(R.id.iv_row_special_button_1, BitmapFactory.decodeResource(context.getResources(), R.drawable.action_call_icon));
			contactRowButton.put(R.id.iv_row_special_button_2, BitmapFactory.decodeResource(context.getResources(), R.drawable.action_sms_icon));
			contactRowButton.put(R.id.iv_row_special_button_3, BitmapFactory.decodeResource(context.getResources(), R.drawable.action_mail_icon));
		}
		
		if (srch2IndicatorColorCode == -12345) {
			srch2IndicatorColorCode = context.getResources().getColor(R.color.row_indicator_default);
		}
		
		if (srch2Icon == null) {
			srch2Icon = BitmapFactory.decodeResource(context.getResources(), R.drawable.srch2_logo);
		}
		
		if (transparentSquare == null) {
			transparentSquare = BitmapFactory.decodeResource(context.getResources(), R.drawable.transparent_square_pixel);
		}
	}
	
	public static Bitmap getDefaultIcon(IndexType whichIndexType) {
		Bitmap result = null;
		try { result = defaultIcon.get(whichIndexType); } catch (NullPointerException ignore) { }
		if (result == null) { result = srch2Icon; }
		return result;
	}
	
	public static int getDefaultIndicatorColor(IndexType whichIndexType) {
		try {
			return defaultIndicatorColor.get(whichIndexType);
		} catch (NullPointerException npe) {
			return srch2IndicatorColorCode;
		}
	}
	
	public static Bitmap getContactRowButton(Integer whichRowButtonId) {
		try {
			return contactRowButton.get(whichRowButtonId);
		} catch (NullPointerException npe) {
			return transparentSquare;
		}
	}

	public static float convertPixelsToDp(Context context, float px){
	    Resources resources = context.getResources();
	    DisplayMetrics metrics = resources.getDisplayMetrics();
	    float dp = px / (metrics.densityDpi / 160f);
	    return dp;
	}
	
	public static float convertDpToPixels(Context context,float dipValue) {
	    DisplayMetrics metrics = context.getResources().getDisplayMetrics();
	    return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dipValue, metrics);
	}
}