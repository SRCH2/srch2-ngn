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
package com.srch2.android.http.app.demo.data.incremental;

import java.util.ArrayList;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteStatement;
import android.util.Log;

public class IncrementalDatabase extends SQLiteOpenHelper {

	private static final String databaseName = "incstate";
	private static final int databaseVersion = 1;
	
	private static final String PRIMARY_KEY_NAME = "_id";
	private static final String VERSION_KEY_NAME = "ver";
	
	static private final String getCreateTableString(final String tableName) {
		return "CREATE TABLE IF NOT EXISTS " + tableName + 
				"(" +
				PRIMARY_KEY_NAME + " INTEGER, " + 
				VERSION_KEY_NAME + " INTEGER" + 
				")";
	}
	
	public IncrementalDatabase(Context context) {
		super(context, databaseName, null, databaseVersion);
	}
	
	public void serializeIncrementalState(String tableNameOfIndex, ArrayList<IncrementalValuePair> incrementalValuePairs) {
		if (incrementalValuePairs == null || incrementalValuePairs.size() == 0) {
			Log.d("srch2:: IncrementalDatabase", "SERIALZIE FAILURE no ivp's");
			
			return;
		}
		Log.d("srch2:: IncrementalDatabase", "SERIALZIE start");
		
		SQLiteDatabase db = getWritableDatabase();
		db.beginTransaction();
		db.execSQL("DROP TABLE IF EXISTS " + tableNameOfIndex + " ");
		db.execSQL(getCreateTableString(tableNameOfIndex));
		final SQLiteStatement statement = db.compileStatement("INSERT INTO " + tableNameOfIndex + " VALUES( ?, ? )");
		for (IncrementalValuePair ivp : incrementalValuePairs) {
			statement.bindLong(1, ivp.id);
			statement.bindLong(2, ivp.version);
			statement.executeInsert();
		}
		
		Log.d("srch2:: IncrementalDatabase", "SERIALZIE SUCCESS of size " + incrementalValuePairs.size());
		
		db.setTransactionSuccessful();
		db.endTransaction();
		db.close();
	}
	
	public ArrayList<IncrementalValuePair> deserializeIncrementalState(String tableNameOfIndex) {
		ArrayList<IncrementalValuePair> pairs = new ArrayList<IncrementalValuePair>();
		Log.d("srch2:: IncrementalDatabase", "DESERIALZIE start");
		
		final SQLiteDatabase db = getReadableDatabase();
		Cursor c = null;
		db.execSQL(getCreateTableString(tableNameOfIndex));
		try {
			c = db.rawQuery("SELECT * FROM " + tableNameOfIndex + " ORDER BY " + PRIMARY_KEY_NAME + " ASC, " + VERSION_KEY_NAME + " ASC", null);
			if (c.moveToFirst()) {
				do {
					IncrementalValuePair p = new IncrementalValuePair(c.getInt(0), c.getInt(1));
					pairs.add(p);
				} while (c.moveToNext());
			}
		} catch (Exception e) { Log.d("srch2:: IncrementalDatabase", "error during DESERIALIZATION ......................."); e.printStackTrace();
		} finally {
			if (c != null) {
				c.close();
			}
		}
		
		Log.d("srch2:: IncrementalDatabase", "DESERIALZIE finished return pair size " + pairs.size());
		
		
		return pairs;
	}
	
	@Override
	public void onCreate(SQLiteDatabase db) { }

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) { }
}
