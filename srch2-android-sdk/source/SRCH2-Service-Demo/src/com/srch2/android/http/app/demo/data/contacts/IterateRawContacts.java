package com.srch2.android.http.app.demo.data.contacts;

import java.util.ArrayList;

import org.json.JSONException;
import org.json.JSONObject;

import com.srch2.android.http.app.demo.data.SourceDataRecords;
import com.srch2.android.http.app.demo.data.incremental.IncrementalValuePair;

import android.content.ContentResolver;
import android.database.CharArrayBuffer;
import android.database.Cursor;
import android.net.Uri;
import android.provider.ContactsContract;
import android.util.Log;

public class IterateRawContacts {

	private final static String TAG = "IterateRawContacts";
	
	public static final int MAXIMUM_TIMES_CONTACTED_CAP = 100;
	
	final public static Cursor getCursor(final ContentResolver cr) {
		return getCursor(cr, null);
	}
	
	final public static Cursor getCursor(final ContentResolver cr, final String... recordsToSelect) {
		final Uri uri = ContactsContract.RawContacts.CONTENT_URI;
		final String[] proj = new String[] 
				{ 
					ContactsContract.RawContacts.CONTACT_ID, 
					ContactsContract.RawContacts.VERSION,
					ContactsContract.RawContacts.DISPLAY_NAME_PRIMARY,
					ContactsContract.RawContacts.TIMES_CONTACTED,
					ContactsContract.RawContacts.STARRED, 
				};
		
		final String order = ContactsContract.RawContacts.CONTACT_ID + " ASC, " + ContactsContract.RawContacts.VERSION + " ASC";
	
		String select = null;
		String[] selectIdArgs = null;

		if (recordsToSelect != null && (recordsToSelect.length < 999 && recordsToSelect.length > 0)) {
			StringBuilder sb = new StringBuilder();

			final int recordsSize = recordsToSelect.length;
			for (int i = 0; i < recordsSize - 1; ++i) {
				sb.append(ContactsContract.RawContacts.CONTACT_ID + " = ? OR ");
			}
			sb.append(ContactsContract.RawContacts.CONTACT_ID + " = ?");
			
			select = sb.toString();
			selectIdArgs = recordsToSelect;
			
		} else {
			select = ContactsContract.RawContacts.DELETED + " = 0";
		}
		
		return cr.query(uri, proj, select, selectIdArgs, order);
	}

	/** Iterates RawContacts to retrieve contacts according to cursor selection. */
	public static SourceDataRecords iterate(Cursor iterationCursor) {

		SourceDataRecords records = null;

		Cursor c = null;
		try {
			c = iterationCursor;
		
			if (c.moveToFirst()) {
			
				final int cursorGetCount = c.getCount();
				records = new SourceDataRecords(cursorGetCount);
				
				final int nameColumnIndex = c.getColumnIndex(ContactsContract.RawContacts.DISPLAY_NAME_PRIMARY);
				CharArrayBuffer cab = new CharArrayBuffer(40);
				StringBuilder name = new StringBuilder();
				
				long previousId = -1;
				
				do { // note: it was observed after starring a record, there was a new contact id for the same contact
					 // ie: duplicates must be pruned at some point based on an equality of this class before inserting
				
					final long currentId = c.getLong(0);
					if (currentId == 0) {
						continue;
					}
					
					c.copyStringToBuffer(nameColumnIndex, cab);
					if (cab.sizeCopied != 0) {
						name.setLength(0);
						name.insert(0, cab.data, 0, cab.sizeCopied);
					} else {
						continue;
					}
					 
					final int version = c.getInt(1);
					final long timesContacted = c.getLong(3);
					final int isStarred = c.getInt(4);
					
					if (currentId != previousId) {
						JSONObject r = new JSONObject();
						try {
							r.put("name", name.toString());
							r.put("score", computeScore(isStarred, timesContacted));
							r.put("id", String.valueOf(currentId));
						} catch (JSONException ignore) { }
						IncrementalValuePair ivp = new IncrementalValuePair(currentId, version);
						records.add(currentId, r, ivp);
						previousId = currentId;
					}
				} while (c.moveToNext());	
			}
		} finally {
			if (c != null) {
				c.close();
			}
		}
		
		return records;
	//prune this list by removing non-visible group from ContactsContract.Contacts
	}

	private static float computeScore(int isStarred, long timesContacted) {
		float score = 50 * isStarred;
		score += Math.max(1, (timesContacted / (float) (timesContacted + MAXIMUM_TIMES_CONTACTED_CAP)) * 50f);
		return score;
	}
	
	/*
	public static ArrayList<IdPrimaryDisplayNamePair> iterateForNonVisibleContacts(ContentResolver cr) {
		Pith.d(IndexActuator.TAG, "iterateRawContacts()");
		
		ArrayList<IdPrimaryDisplayNamePair> nonVisibleContacts = null;

		final Uri uri = ContactsContract.Contacts.CONTENT_URI;
		final String[] projection = new String[] 
				{ 
					ContactsContract.Contacts._ID, 
					ContactsContract.Contacts.DISPLAY_NAME_PRIMARY,
				};
		
		final String select = ContactsContract.Contacts.IN_VISIBLE_GROUP + " =0";
	
		Cursor c = null;
		try {
			c = cr.query(uri, projection, select, null, null);
		
			if (c.moveToFirst()) {
			
				final int cursorGetCount = c.getCount();
				nonVisibleContacts = new ArrayList<IdPrimaryDisplayNamePair>(cursorGetCount);
				
				final int nameColumnIndex = c.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME_PRIMARY);
				CharArrayBuffer cab = new CharArrayBuffer(40);
				StringBuilder name = new StringBuilder();
				
				long previousId = -1;
				
				do { // note: it was observed after starring a record, there was a new contact id for the same contact
					 // ie: duplicates must be pruned at some point based on an equality of this class before inserting
				
					final long currentId = c.getLong(0);
					if (currentId == 0) {
						continue;
					}
					
					c.copyStringToBuffer(nameColumnIndex, cab);
					if (cab.sizeCopied != 0) {
						name.setLength(0);
						name.insert(0, cab.data, 0, cab.sizeCopied);
					} else {
						continue;
					}
					 
					if (currentId != previousId) {
						nonVisibleContacts.add(new IdPrimaryDisplayNamePair(currentId, name));
						previousId = currentId;
					}
				} while (c.moveToNext());	
			}
		} finally {
			if (c != null) {
				c.close();
			}
		}
		
		if (nonVisibleContacts == null ) { nonVisibleContacts = new ArrayList<IdPrimaryDisplayNamePair>(0); }
		return nonVisibleContacts;
	}
	*/
	
	public static class IdPrimaryDisplayNamePair {
		long id;
		String name;
		
		public IdPrimaryDisplayNamePair(long theId, StringBuilder theName) {
			id = theId;
			name = new String(theName);
		}
		
		public boolean isEqual(long otherId, String theOtherName) {
			return (id == otherId) && (theOtherName != null && name != null && name.equals(theOtherName));
		}
	}
	
	
	
	// add one with observer // logger
}