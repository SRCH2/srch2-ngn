package com.srch2.android.http.app.demo.data.contacts;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

import org.json.JSONObject;

import android.content.ContentResolver;
import android.database.Cursor;

import com.srch2.android.http.app.demo.data.SourceDataRecords;
import com.srch2.android.http.app.demo.data.incremental.IncrementalValuePair;

public class IterateContactsDifference {
	private static final String TAG = "ContentSourceInspector";
	
	public static HashMap<Boolean, SourceDataRecords> inspectForRecordsToUpdate(ContentResolver cr, HashSet<IncrementalValuePair> latestIncrementalSnapshot) {
		HashSet<IncrementalValuePair> currentIncrementalData = getCurrentIncrementalDataValues(cr);
		
		HashMap<Boolean, HashSet<IncrementalValuePair>> results = 
				IncrementalValuePair.resolveIncrementalDifference(
						currentIncrementalData, latestIncrementalSnapshot); 
	
		HashSet<IncrementalValuePair> additions = results.get(true);
		HashSet<IncrementalValuePair> deletions = results.get(false);

		HashMap<Boolean, SourceDataRecords> updateRecordSet = new HashMap<Boolean, SourceDataRecords>();

		SourceDataRecords recordsToAdd = (additions != null && additions.size() > 0) ? retrieveAdditions(cr, additions) : new SourceDataRecords(0);
		SourceDataRecords recordsToDelete = (deletions != null && deletions.size() > 0) ? retrieveDeletions(deletions) : new SourceDataRecords(0);
		
		updateRecordSet.put(true, recordsToAdd);
		updateRecordSet.put(false, recordsToDelete);
	
		return updateRecordSet;
	}
	
	/** Returns the hashset of id-version pairs representing the current incremental data, to be used to diff against the latest incremental snapshot. */
	private static HashSet<IncrementalValuePair> getCurrentIncrementalDataValues(ContentResolver cr) {
		HashSet<IncrementalValuePair> currentIncrementalData = new HashSet<IncrementalValuePair>();
		
		Cursor c = null;
		try {
			c = IterateRawContacts.getCursor(cr);
			
			if (c.moveToFirst()) {
				final int cursorGetCount = c.getCount();
				currentIncrementalData = new HashSet<IncrementalValuePair>(cursorGetCount);
				
				//final int nameColumnIndex = c.getColumnIndex(ContactsContract.RawContacts.DISPLAY_NAME_PRIMARY);
				//CharArrayBuffer cab = new CharArrayBuffer(40);
				//StringBuilder name = new StringBuilder();
				
				long previousId = -1;
				
				do { // note: it was observed after starring a record, there was a new contact id for the same contact
					 // ie: duplicates must be pruned at some point based on an equality of this class before inserting
					
					final long currentId = c.getLong(0);
					if (currentId == 0) { continue; }
					
					/* test to see if can be omited: depends on if adding single will prevent if containing these lines...
					c.copyStringToBuffer(nameColumnIndex, cab);
					if (cab.sizeCopied != 0) {
						name.setLength(0);
						name.insert(0, cab.data, 0, cab.sizeCopied);
					} else {
						continue;
					}
					*/
					
					final int version = c.getInt(1);
					
					if (currentId != previousId) {
						currentIncrementalData.add(new IncrementalValuePair(currentId, version));
						previousId = currentId;
					}
				} while (c.moveToNext());	
			}
		} finally {
			if (c != null) {
				c.close();
			}
		}
		
		return currentIncrementalData;
	}

	

	/** Retrieves the set of source data records, used to do restful update, representing the new records that need to be added to the index. */ 
	public static SourceDataRecords retrieveAdditions(ContentResolver cr, HashSet<IncrementalValuePair> additions) {
		String[] selectIds = ContentProviderConstants.getSelectedIdArgs(additions);
		SourceDataRecords records = IterateRawContacts.iterate(IterateRawContacts.getCursor(cr, selectIds));
		IterateRawContactEntities.iterate(IterateRawContactEntities.getCursor(cr, selectIds), records);
		return records;
	}
	
	/** Retrieves the set of ids, used to do restful insert, representing the records that need to be deleted from the index. */
	public static SourceDataRecords retrieveDeletions(HashSet<IncrementalValuePair> deletions) {
		return new SourceDataRecords(deletions);
	}
}
