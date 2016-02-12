package com.srch2.android.http.app.demo.data.contacts;

import java.io.InputStream;
import java.util.HashMap;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.ContactsContract;
import android.widget.Toast;

import com.srch2.android.http.app.demo.ContextSingleton;
import com.srch2.android.http.app.demo.DefaultUiResources;
import com.srch2.android.http.app.demo.IndexType;
import com.srch2.android.http.app.demo.R;
import com.srch2.android.http.app.demo.data.ViewableResult;

public class ContactViewableResult extends ViewableResult {
	
	private static final String TAG = "ContactViewableResult";
	
	
	public static ContactsFactoryWrapper factory = new ContactsFactoryWrapper();
	public static ContactClickFunctor rowClickFunctor = new ContactClickFunctor();
	public static ContactRowButtonClickFunctor rowButtonClickFunctor = new ContactRowButtonClickFunctor();
	
	public String emailAddress;
	public String phoneNumber;

	public static class ContactsFactoryWrapper extends Wrapper {
		@Override
		public ViewableResult wrap(JSONObject jsonRecord) {
			String lookupKey = null;
			String title = null;
			try {
				lookupKey = String.valueOf(jsonRecord.get("id"));
				title = jsonRecord.getString("name");
			} catch (JSONException ignore) { }
			
			ContactViewableResult vr = new ContactViewableResult(lookupKey, title);
			vr.rowClickFunction = rowClickFunctor;
			vr.rowButtonClickFunction = rowButtonClickFunctor;
			
			Context c = ContextSingleton.getContext();
			
			String emailAddress = null;
			String phoneNumber = null;
			if (c != null) {
				emailAddress = getContactEmail(c, lookupKey);
				phoneNumber = getContactPhoneNumber(c, lookupKey);
			}
			
			vr.rowButtonClickMap = new HashMap<Integer, Bitmap>();
			
			if (emailAddress != null && !emailAddress.equals(ViewableResult.INVALID_VALUE)) {
				vr.emailAddress = emailAddress;
				vr.rowButtonClickMap.put(R.id.iv_row_special_button_3, DefaultUiResources.getContactRowButton(R.id.iv_row_special_button_3));
			}
			
			if (phoneNumber != null && !phoneNumber.equals(ViewableResult.INVALID_VALUE)) {
				vr.phoneNumber = phoneNumber;
				vr.isSwippable = true;
				vr.rowButtonClickMap.put(R.id.iv_row_special_button_1, DefaultUiResources.getContactRowButton(R.id.iv_row_special_button_1));
				vr.rowButtonClickMap.put(R.id.iv_row_special_button_2, DefaultUiResources.getContactRowButton(R.id.iv_row_special_button_2));
			}

			return vr;
		}
	}
	
	protected ContactViewableResult(String theLookupKey, String theTitle) {
		super(IndexType.Contacts, theLookupKey, theTitle);
	}

	@Override
	public Bitmap getIcon(Context context) {
		boolean cnull = context == null;

		Bitmap b = null;
		Uri uri = null;
		InputStream input = null;
		try {
			uri = ContentUris.withAppendedId(ContactsContract.Contacts.CONTENT_URI, Long.valueOf(lookupKey));
			/* ContactsContract.Contacts.openContactPhotoInputStream non-deterministically time consuming */
			input = ContactsContract.Contacts.openContactPhotoInputStream(context.getContentResolver(), uri);
			b = BitmapFactory.decodeStream(input);
		

		} catch (Exception e) {
			e.printStackTrace();
		}
		return b;
	}

	@Override
	public void onRowClick(Context context, ViewableResult clickedViewableResult) {
		super.onRowClick(context, clickedViewableResult);
		if (rowClickFunction != null) {
			rowClickFunction.onClick(context, clickedViewableResult);
		}
	}

	@Override
	public void onRowButtonClick(Context context, int rowButtonId, ViewableResult clickedViewableResult) {
		super.onRowButtonClick(context, rowButtonId, clickedViewableResult);
		if (rowButtonClickFunction != null) {
			rowButtonClickFunction.onClick(context, rowButtonId, clickedViewableResult);
		}
	}

	@Override
	public void onSwipeLeft(Context context, ViewableResult clickedViewableResult) {
		super.onSwipeLeft(context, clickedViewableResult);
		
	
		boolean success = true;
		try { context.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("smsto:" + ((ContactViewableResult) clickedViewableResult).phoneNumber))); } 
		catch (Exception e) { success = false; }
		if (!success) { Toast.makeText(context, "Sorry, Android seems to be having trouble handling this request!", Toast.LENGTH_LONG).show(); }
	}

	@Override
	public void onSwipeRight(Context context, ViewableResult clickedViewableResult) {
		super.onSwipeRight(context, clickedViewableResult);
		

		String phoneNumber = ((ContactViewableResult) clickedViewableResult).phoneNumber;
		

		boolean success = true;
		try { context.startActivity(new Intent(Intent.ACTION_CALL, Uri.parse("tel:" + ((ContactViewableResult) clickedViewableResult).phoneNumber))); } 
		catch (Exception e) { success = false; e.printStackTrace(); }
		if (!success) { Toast.makeText(context, "Sorry, Android seems to be having trouble handling this request!", Toast.LENGTH_LONG).show(); }
	}

	public static class ContactClickFunctor extends RowClickFunctor {
		@Override
		public void onClick(Context context, ViewableResult clickedViewableResult) {
			boolean success = true;
			
			if (!clickedViewableResult.lookupKey.equals(ViewableResult.INVALID_VALUE)) {
				try {
					Intent intent = new Intent(Intent.ACTION_VIEW, Uri.withAppendedPath(ContactsContract.Contacts.CONTENT_URI, clickedViewableResult.lookupKey));
					context.startActivity(intent); 
				} catch (Exception e) {
					success = false;
				}
			} else { 
				success = false;
			}

			if (!success) { Toast.makeText(context, "Sorry, Android seems to be having trouble handling this request!", Toast.LENGTH_LONG).show(); } 
		}
	}	
	
	public static class ContactRowButtonClickFunctor extends RowButtonClickFunctor {
		@Override
		public void onClick(Context context, int rowButtonId, ViewableResult clickedViewableResult) {
			boolean success = true;
			Intent i = null;
			
			switch (rowButtonId) {
				case R.id.iv_row_special_button_3:
					if (((ContactViewableResult) clickedViewableResult).emailAddress.equals(ViewableResult.INVALID_VALUE)) {
						success = false;
						break;
					}
					
					try {
						i = new Intent(Intent.ACTION_SEND);
						i.setType("text/html");
						i.putExtra(Intent.EXTRA_EMAIL, new String[] { ((ContactViewableResult) clickedViewableResult).emailAddress });
						context.startActivity(i);
					} catch (Exception e) {
						success = false;
					}
					break;
				case R.id.iv_row_special_button_1:
					if (((ContactViewableResult) clickedViewableResult).phoneNumber.equals(ViewableResult.INVALID_VALUE)) {
						success = false;
						break;
					}
					
					try {
						i = new Intent(Intent.ACTION_DIAL, Uri.parse("tel:" + ((ContactViewableResult) clickedViewableResult).phoneNumber)); 
						context.startActivity(i);
					} catch (Exception e) {
						success = false;
					}
					break;
				case R.id.iv_row_special_button_2:
					if (((ContactViewableResult) clickedViewableResult).phoneNumber.equals(ViewableResult.INVALID_VALUE)) {
						success = false;
						break;
					}
					
					try {
						i = new Intent(Intent.ACTION_VIEW, Uri.parse("smsto:" + ((ContactViewableResult) clickedViewableResult).phoneNumber));         
						context.startActivity(i);
					} catch (Exception e) {
						success = false;
					}
					break;
			}	
			if (!success) { Toast.makeText(context, "Sorry, Android seems to be having trouble handling this request!", Toast.LENGTH_LONG).show(); }
		}
	}
	
	public static final String getContactEmail(Context context, String contactId) {
		String s = null;
        Cursor c = null;
        try {
        	c = context.getContentResolver().query(
												 ContactsContract.CommonDataKinds.Email.CONTENT_URI,
							                     new String[] { ContactsContract.CommonDataKinds.Email.DATA }, 
							                     ContactsContract.CommonDataKinds.Email.CONTACT_ID + " = ?",
							                     new String[] {contactId}, 
							                     null);

        	if(c.moveToFirst()) {
        		s = c.getString(0);
			}
        } finally {
        	c.close();
        }
        if (s == null) {
        	return ViewableResult.INVALID_VALUE;
        } else if (s.length() < 1) {
        	return ViewableResult.INVALID_VALUE;
        } else {
        	return s;
        }
	}
	
	public static final String getContactPhoneNumber(Context context, String contactId) {
		String s = null;
        Cursor c = null;
        try {
        	c = context.getContentResolver().query(
												 ContactsContract.CommonDataKinds.Phone.CONTENT_URI,
							                     new String[] { ContactsContract.CommonDataKinds.Phone.DATA }, 
							                     ContactsContract.CommonDataKinds.Phone.CONTACT_ID + " = ?",
							                     new String[] {contactId}, 
							                     null);

        	if(c.moveToFirst()) {
        		s = c.getString(0);
			}
        } finally {
        	c.close();
        }
        if (s == null) {
        	return ViewableResult.INVALID_VALUE;
        } else if (s.length() < 1) {
        	return ViewableResult.INVALID_VALUE;
        } else {
        	return s;
        }
	}
}
