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
