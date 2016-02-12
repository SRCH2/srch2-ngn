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
package com.srch2.android.sdk;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.sql.Timestamp;
import java.util.Random;
import java.util.UUID;

public class TestCaseUtil {

    static final long offset = Timestamp.valueOf("2012-01-01 00:00:00").getTime();
    static final long end = Timestamp.valueOf("2013-01-01 00:00:00").getTime();
    static final long diff = end - offset + 1;
    private static TestCaseUtil instance;
    private static Random random = new Random();

    public static String generateRandomSearchableString(String seed) {
        return seed + " " + UUID.randomUUID().toString();
    }

    static String generateRandomIdentifier(String prefix) {
        return prefix + "-" + UUID.randomUUID().toString();
    }

    public static int generateRandomInteger(int min, int max) {
        return random.nextInt(max - min + 1) + min;
    }

    public static float generateRandomFloat(float min, float max) {
        return min + (float) Math.random() * (max - min + 1);
    }

    public static String generateRandomTime() {
        Timestamp rand = new Timestamp(offset + (long) (Math.random() * diff));
        return rand.toString();
    }

    public static double generateRandomGeo() {
        return (double) generateRandomFloat(-180, 180);
    }

    public Indexable createTestIndex() {
        return new TestIndex();
    }

    public static JSONObject generateRandomRecord(Indexable indexable) {
        JSONObject record = new JSONObject();
        try {
            for (Field f : indexable.getSchema().fields) {
                switch (f.type) {
                    case TEXT:

                        record.put(f.name, generateRandomIdentifier(f.name));
                        break;
                    case INTEGER:
                        record.put(f.name, generateRandomInteger(0, 1000));
                        break;
                    case FLOAT:
                        record.put(f.name, generateRandomFloat(0, 1000));
                        break;
                    case TIME:
                        record.put(f.name, generateRandomTime());
                        break;
                    case LOCATION_LONGITUDE:
                        record.put(f.name, generateRandomGeo());
                        break;
                    case LOCATION_LATITUDE:
                        record.put(f.name, generateRandomGeo());
                        break;
                }
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return record;
    }

    public static JSONArray generateRandomRecordArray(Indexable indexable, int count) throws JSONException {
        JSONArray array = new JSONArray();
        for (int i = 0; i < count; i++) {
            array.put(generateRandomRecord(indexable));
        }
        return array;
    }
}
