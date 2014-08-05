package com.srch2.android.http.app.demo.data.contacts;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;

import org.json.JSONException;
import org.json.JSONObject;

import com.srch2.android.http.app.demo.data.SourceDataRecords;

import android.content.ContentResolver;
import android.database.CharArrayBuffer;
import android.database.Cursor;
import android.net.Uri;
import android.provider.ContactsContract;
import android.util.Log;

public class IterateRawContactEntities {
	private static final String TAG = "IterateRawContactEntities";
	
	/** Mime-types of data to exclude from search. */
	private static final String[] MIME_LABEL_EXCLUSION_VALUES = 
		{ 
			"vnd.android.cursor.item/organization_svoice_dmetaphone_primary_encoding",
			"vnd.android.cursor.item/name",
			"vnd.android.cursor.item/photo",
			"vnd.android.cursor.item/group_membership",
			"vnd.android.cursor.item/identity"
		};
	/** Listing of mime-types of data to exclude from contributing to matching keywords of full text literal. */
	private static final HashSet<String> reversibleLookUpMimeLabelExclusion = new HashSet<String>(Arrays.asList(MIME_LABEL_EXCLUSION_VALUES));

	/** Gets a cursor to the RawContactEntities table returning all contact rows selecting against contacts marked deleted, and
	 * sorted by CONTACT_ID and DATA_VERSION. */
	final public static Cursor getCursor(final ContentResolver cr) {
		return getCursor(cr, null);
	}
	
	/** Gets a cursor to the RawContactEntities table returning selected contact rows selecting for ids matching those in recordsToSelect, and
	 * sorted by CONTACT_ID and DATA_VERSION. */
	final public static Cursor getCursor(final ContentResolver cr, final String... recordsToSelect) {
		final Uri uri = ContactsContract.RawContactsEntity.CONTENT_URI;
		final String[] proj = new String[] 
				{ 
					ContactsContract.RawContactsEntity.CONTACT_ID, 
					ContactsContract.RawContactsEntity.DATA_ID, 
					ContactsContract.RawContactsEntity.MIMETYPE,
					ContactsContract.RawContactsEntity.DATA1  
				};
		
		final String order = ContactsContract.RawContactsEntity.CONTACT_ID + " ASC, " + ContactsContract.RawContactsEntity.DATA_VERSION + " ASC";
		
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
	
	/** Records must already contain contact id, display name, and should contain boost data. Full text search relevance will be set for each record. */
	static public void iterate(Cursor iteratationCursor, SourceDataRecords records) {
		
		
		Cursor c = null;
		try {
			c = iteratationCursor;
	
			if (c.moveToFirst()) {
				final int BLOB_MIME_TYPE = Cursor.FIELD_TYPE_BLOB;
				final int dataIdColumnIndex = c.getColumnIndex(ContactsContract.RawContactsEntity.DATA_ID);
				final int mimeDataColumnIndex = c.getColumnIndex(ContactsContract.RawContactsEntity.DATA1);
				
				CharArrayBuffer cab = new CharArrayBuffer(128);
				
				StringBuilder mimeData_stateHolder = new StringBuilder();
				long contactId_stateHolder = 0;
				long previousId = 0;

				// the first while loops forces the cursor to the first valid raw contact entity row
				boolean notNullId = false;
				boolean notBlobMimeType = false;
				boolean notNullDataId = false;
				boolean notExcludedMimeType = false;
				while (!c.isAfterLast()) {
					contactId_stateHolder = c.getLong(0);
					if (contactId_stateHolder != 0) {
						notNullId = true;	
					}
					
					final String mimeLabel = c.getString(2);
					if (mimeLabel != null && !reversibleLookUpMimeLabelExclusion.contains(mimeLabel)) {
						notExcludedMimeType = true;
					}
					
					c.copyStringToBuffer(dataIdColumnIndex, cab);
					if (cab.sizeCopied != 0) {
						notNullDataId = true;
					}
					
					if (c.getType(3) != BLOB_MIME_TYPE) {
						notBlobMimeType = true;
					}
		
					if (notNullId && notBlobMimeType && notNullDataId && notExcludedMimeType) {
						break;
					} else {
						notNullId = notBlobMimeType = notNullDataId = notExcludedMimeType = false;
						c.moveToNext();
					}
				}
				StringBuilder fullTextLiteralHolder = new StringBuilder();
				previousId = contactId_stateHolder;
				
				do {
					contactId_stateHolder = c.getLong(0);
					if (contactId_stateHolder == 0) {
						continue;
					}
				
					// if dataId is null then there is no data to be retrieved
					c.copyStringToBuffer(dataIdColumnIndex, cab);
					if (cab.sizeCopied == 0) {
						continue;
					}

					// filter mime data against blob (pictures) and excluded types (eg, phonetic name spellings that give nonsensical false positives for search results) 
					final String mimeLabel = c.getString(2);
					if (mimeLabel == null || reversibleLookUpMimeLabelExclusion.contains(mimeLabel) || c.getType(3) == BLOB_MIME_TYPE) {
						continue;
					}

					boolean hasRealData = false;
					c.copyStringToBuffer(mimeDataColumnIndex, cab);
					if (cab.sizeCopied != 0) {
						hasRealData = true;
						mimeData_stateHolder.setLength(0);
						mimeData_stateHolder.insert(0, cab.data, 0, cab.sizeCopied);
					}
					
					if (hasRealData) {
						if (contactId_stateHolder == previousId) {
							// if one the same record, append the relevant datum
							fullTextLiteralHolder.append(mimeData_stateHolder);
							fullTextLiteralHolder.append(" ");
						} else {
							// row belongs to new contact id, add previous one which will contain all data since all it's rows have been iterated 
							JSONObject record = records.jsonRecordIdMap.get(previousId);
							if (record != null) {
								try {
									record.put("fulltext", fullTextLiteralHolder.toString());
								} catch (JSONException ignore) {} 
							}
							
							fullTextLiteralHolder.setLength(0);
							previousId = contactId_stateHolder;
						}
					}
				} while (c.moveToNext());
				// add the last record
				JSONObject record = records.jsonRecordIdMap.get(previousId);
				if (record != null) {
					try {
						record.put("fulltext", fullTextLiteralHolder.toString());
					} catch (JSONException ignore) {} 
				}
			} 
		} finally {
			if (c != null) {
				c.close();
			}
		}
	}
	
}
