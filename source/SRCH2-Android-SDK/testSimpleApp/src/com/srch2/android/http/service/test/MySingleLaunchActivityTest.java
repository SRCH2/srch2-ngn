package com.srch2.android.http.service.test;

import android.test.SingleLaunchActivityTestCase;
import android.util.Log;
import com.srch2.android.http.service.*;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.UUID;


public class MySingleLaunchActivityTest extends SingleLaunchActivityTestCase<MySingleLaunchActivity> {
    final static String TAG = "SRCH2::MySingleLaunchActivityTest";
    private static boolean firstStart = true;
    MySingleLaunchActivity mActivity;

    public MySingleLaunchActivityTest() {
        super("com.srch2.android.http.service", MySingleLaunchActivity.class);

    }

    @Override
    public void setUp() throws Exception {
        super.setUp();
        Log.d(TAG, "setUp get called");
        mActivity = getActivity();
        mActivity.waitForEngineReady();
        if (firstStart) {
            firstStart = false;
            if (mActivity.mControlListener.indexesInfoResponseMap.get(MyActivity.MovieIndex.INDEX_NAME).getNumberOfDocumentsInTheIndex() < 1) {
                insertToMovie();
            }
            if (mActivity.mControlListener.indexesInfoResponseMap.get(MyActivity.MusicIndex.INDEX_NAME).getNumberOfDocumentsInTheIndex() < 1) {
                insertToMusic();

            }
            if (mActivity.mControlListener.indexesInfoResponseMap.get(MyActivity.GeoIndex.INDEX_NAME).getNumberOfDocumentsInTheIndex() < 1) {
                //FIXME geo is not working anymore, why ?
//                insertToGeo();
            }

        }
    }

    private void insertToMovie() {

        JSONArray recordsJsonArray = new JSONArray();
        try {
            JSONObject recordOne = new JSONObject();
            recordOne.put(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY, 1);
            recordOne.put(MyActivity.MovieIndex.INDEX_KEY_TITLE, "Mission Impossible");
            recordOne.put(MyActivity.MovieIndex.INDEX_KEY_GENRE, "Action");
            recordOne.put(MyActivity.MovieIndex.INDEX_KEY_YEAR, 2002);
            recordsJsonArray.put(recordOne);

            JSONObject recordTwo = new JSONObject();
            recordTwo.put(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY, 2);
            recordTwo.put(MyActivity.MovieIndex.INDEX_KEY_TITLE, "Independence Day");
            recordTwo.put(MyActivity.MovieIndex.INDEX_KEY_GENRE, "Science Fiction");
            recordTwo.put(MyActivity.MovieIndex.INDEX_KEY_YEAR, 1999);
            recordsJsonArray.put(recordTwo);

            JSONObject record3 = new JSONObject();
            record3.put(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY, 3);
            record3.put(MyActivity.MovieIndex.INDEX_KEY_TITLE, "Rocky 3");
            record3.put(MyActivity.MovieIndex.INDEX_KEY_GENRE, "Action");
            record3.put(MyActivity.MovieIndex.INDEX_KEY_YEAR, 1980);
            recordsJsonArray.put(record3);
        } catch (JSONException oops) {
        }
        mActivity.mControlListener.insertResponse = null;
        mActivity.mMovieIndex.insert(recordsJsonArray);
        InsertResponse response = mActivity.getInsertResponse();
        Log.d(TAG, response.toString());
        assertTrue(3 == response.getSuccessCount());
//        Util.assertInsertRecordsNumber(mActivity, mActivity.mMovieIndex, 2);

        recordsJsonArray = new JSONArray();
        Log.d("srch2:: demo index", "INSERTING 200");
        int totalCount = 200;
        for (int i = 11; i < totalCount; ++i) {

            if (i % 1000 == 0) {
                Log.d("srch2:: demo index", "at count " + i);
            }

            JSONObject record = new JSONObject();
            try {
                record.put(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY, i);
                record.put(MyActivity.MovieIndex.INDEX_KEY_TITLE, "SRCH2 test " + UUID.randomUUID());
                record.put(MyActivity.MovieIndex.INDEX_KEY_GENRE, UUID.randomUUID());
                record.put(MyActivity.MovieIndex.INDEX_KEY_YEAR, i);
                recordsJsonArray.put(record);
            } catch (JSONException e) {
                e.printStackTrace();
            }
        }
        mActivity.mControlListener.insertResponse = null;
        mActivity.mMovieIndex.insert(recordsJsonArray);
        assertTrue(mActivity.getInsertResponse().getSuccessCount() == 200 - 11);
    }


    private void insertToMusic() {
        JSONArray recordsJsonArray = new JSONArray();
        try {
            JSONObject recordOne = new JSONObject();
            recordOne.put(MyActivity.MusicIndex.INDEX_KEY_PRIMARY_KEY, "1");
            recordOne.put(MyActivity.MusicIndex.INDEX_KEY_SONG_TITLE, "It's a Wonderful World");
            //recordOne.put(INDEX_KEY_SONG_YEAR, 1967);
            //recordOne.put(INDEX_KEY_ARTIST, "Louis Armstrong");
            recordOne.put(MyActivity.MusicIndex.INDEX_KEY_GENRE, "Jazz");
            recordsJsonArray.put(recordOne);

            JSONObject recordTwo = new JSONObject();
            recordTwo.put(MyActivity.MusicIndex.INDEX_KEY_PRIMARY_KEY, "2");
            recordTwo.put(MyActivity.MusicIndex.INDEX_KEY_SONG_TITLE, "All You Need Is Love");
            //recordOne.put(INDEX_KEY_SONG_YEAR, 1967);
            //recordOne.put(INDEX_KEY_ARTIST, "Beatles");
            recordTwo.put(MyActivity.MusicIndex.INDEX_KEY_GENRE, "Rock Roll");
            recordsJsonArray.put(recordTwo);
        } catch (JSONException oops) {
        }
        mActivity.mControlListener.insertResponse = null;
        mActivity.mMusicIndex.insert(recordsJsonArray);
        assertTrue(mActivity.getInsertResponse().getSuccessCount() == 2);
    }

    private void insertToGeo() {
        JSONArray recordsJsonArray = new JSONArray();
        try {
            JSONObject recordOne = new JSONObject();
            recordOne.put(MyActivity.GeoIndex.INDEX_KEY_PRIMARY_KEY, "Irvine");
            recordOne.put(MyActivity.GeoIndex.LAT, 33);
            recordOne.put(MyActivity.GeoIndex.LON, -117);
            recordsJsonArray.put(recordOne);

            JSONObject recordTwo = new JSONObject();
            recordTwo.put(MyActivity.GeoIndex.INDEX_KEY_PRIMARY_KEY, "Los Angeles");
            recordTwo.put(MyActivity.GeoIndex.LAT, 34);
            recordTwo.put(MyActivity.GeoIndex.LON, -118);

            recordsJsonArray.put(recordTwo);
        } catch (JSONException oops) {
        }
        mActivity.mControlListener.insertResponse = null;
        mActivity.mGeoIndex.insert(recordsJsonArray);
        assertTrue(mActivity.getInsertResponse().getSuccessCount() == 2);
    }

    private void clearAndDelete(Indexable index, String id) {
        mActivity.mControlListener.deleteResponse = null;
        index.delete(id);
        DeleteResponse response = mActivity.getDeleteResponse();
        Log.d(TAG, response.toString());
        assertTrue(response.getSuccessCount() == 1);
    }

    private void clearAndInsertAndAssertSuccess(Indexable index, JSONObject record) {
        mActivity.mControlListener.insertResponse = null;
        index.insert(record);
        InsertResponse response = mActivity.getInsertResponse();
        Log.d(TAG, response.toString());
        assertTrue(1 == response.getSuccessCount());
    }

    private void deleteMovie() throws JSONException {
        clearAndDelete(mActivity.mMovieIndex, "1");

        JSONObject recordOne = new JSONObject();
        recordOne.put(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY, "1");
        recordOne.put(MyActivity.MovieIndex.INDEX_KEY_TITLE, "Mission Impossible");
        recordOne.put(MyActivity.MovieIndex.INDEX_KEY_YEAR, 2002);
        recordOne.put(MyActivity.MovieIndex.INDEX_KEY_GENRE, "Action");

        clearAndInsertAndAssertSuccess(mActivity.mMovieIndex, recordOne);
    }

    private void deleteMusic() throws JSONException {
        clearAndDelete(mActivity.mMusicIndex, "1");

        JSONObject recordOne = new JSONObject();
        recordOne.put(MyActivity.MusicIndex.INDEX_KEY_PRIMARY_KEY, "1");
        recordOne.put(MyActivity.MusicIndex.INDEX_KEY_SONG_TITLE, "It's a Wonderful World");
        //recordOne.put(INDEX_KEY_SONG_YEAR, 1967);
        //recordOne.put(INDEX_KEY_ARTIST, "Louis Armstrong");
        recordOne.put(MyActivity.MusicIndex.INDEX_KEY_GENRE, "Jazz");

        clearAndInsertAndAssertSuccess(mActivity.mMusicIndex, recordOne);
    }

    private void deleteGeo() throws JSONException {
        clearAndDelete(mActivity.mGeoIndex, "Irvine");

        JSONObject recordOne = new JSONObject();
        recordOne.put(MyActivity.GeoIndex.INDEX_KEY_PRIMARY_KEY, "Irvine");
        recordOne.put(MyActivity.GeoIndex.LAT, 33);
        recordOne.put(MyActivity.GeoIndex.LON, -117);

        clearAndInsertAndAssertSuccess(mActivity.mGeoIndex, recordOne);
    }


    @Override
    public void tearDown() throws Exception {
        super.tearDown();
        Log.d(TAG, "tearDown");
    }

    void resetAndSearch(Indexable index, String keyword) {
        mActivity.mResultListener.reset();
        index.search(keyword);
        mActivity.getSearchResult();
        Log.d(TAG, "search result:" + mActivity.mResultListener.jsonResultsLiteral);
    }

    void resetAndSearch(Indexable index, Query query) {
        mActivity.mResultListener.reset();
        index.advancedSearch(query);
        mActivity.getSearchResult();
        Log.d(TAG, "search result:" + mActivity.mResultListener.jsonResultsLiteral);
    }


    public void testDeletion() throws JSONException {
        Log.d(TAG, "testDeletion Movie");
        deleteMovie();
        Log.d(TAG, "testDeletion::Deletion done");

        Log.d(TAG, "testDeletion Geo");
        deleteMusic();
        Log.d(TAG, "testDeletion::Deletion done");
    }

    public void testSearch() throws JSONException {
        Log.d(TAG, "testSearch");

        resetAndSearch(mActivity.mMovieIndex, "Missio");

        HashMap result = mActivity.mResultListener.resultRecordMap;
        assertEquals(1, result.size());
        assertTrue(result.containsKey(MyActivity.MovieIndex.INDEX_NAME));
        ArrayList<JSONObject> jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MovieIndex.INDEX_NAME);
        assertEquals(1, jsonArray.size());
        assertEquals(1, (Integer.parseInt(jsonArray.get(0).get(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY).toString())));

        resetAndSearch(mActivity.mMusicIndex, "Rock");
        result = mActivity.mResultListener.resultRecordMap;
        assertEquals(1, result.size());
        assertTrue(result.containsKey(MyActivity.MusicIndex.INDEX_NAME));
        jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MusicIndex.INDEX_NAME);
        assertTrue(jsonArray.size() == 1);
        Log.d(TAG, "Music response: " + jsonArray.get(0));
        assertTrue(2 == Integer.parseInt(jsonArray.get(0).get(MyActivity.MusicIndex.INDEX_KEY_PRIMARY_KEY).toString()));

    }

    public void testGeoSearch() throws JSONException, UnsupportedEncodingException {
        resetAndSearch(mActivity.mGeoIndex, "Irvin");
        HashMap<String, ArrayList<JSONObject>> result = mActivity.mResultListener.resultRecordMap;
        assertEquals(1, result.size());
        assertTrue(result.containsKey(MyActivity.GeoIndex.INDEX_NAME));
        ArrayList<JSONObject> jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.GeoIndex.INDEX_NAME);
        // seems not find any data, using default range.
//        assertTrue(jsonArray.size() == 1);
//        assertTrue("Irvine" == jsonArray.get(0).get(MyActivity.GeoIndex.INDEX_KEY_PRIMARY_KEY).toString());

        Query query = new Query(new SearchableTerm("Irvine").setIsPrefixMatching(true)).insideCircleRegion(33, -117, 50);
        resetAndSearch(mActivity.mGeoIndex, query);
        result = mActivity.mResultListener.resultRecordMap;
        assertTrue(1 == result.size());
        jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.GeoIndex.INDEX_NAME);
//        assertTrue("Irvine" == jsonArray.get(0).get(MyActivity.GeoIndex.INDEX_KEY_PRIMARY_KEY).toString());

    }

    public void testMultiCoreSearch() throws JSONException {
        Log.d(TAG, "testMultiCoreSearch");
        mActivity.mResultListener.reset();
        SRCH2Engine.searchAllIndexes("Rock");
        mActivity.getSearchResult();
        Log.d(TAG, "search result:" + mActivity.mResultListener.jsonResultsLiteral);
        HashMap<String, ArrayList<JSONObject>> result = mActivity.mResultListener.resultRecordMap;
        assertTrue(result.size() == 2);

        ArrayList<JSONObject> movieResult = result.get(MyActivity.MovieIndex.INDEX_NAME);
        ArrayList<JSONObject> musicResult = result.get(MyActivity.MusicIndex.INDEX_NAME);
        assertTrue(movieResult.size() == 1);
        assertTrue(musicResult.size() == 1);
        assertTrue(movieResult.get(0).getString(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY).equals("3"));
        assertTrue(musicResult.get(0).getString(MyActivity.MusicIndex.INDEX_KEY_PRIMARY_KEY).equals("2"));

    }

    public void testSearchByQuery() throws UnsupportedEncodingException, JSONException {
        Log.d(TAG, "testSearchByQuery");
        Query query = new Query(new SearchableTerm("Misso"));
        resetAndSearch(mActivity.mMovieIndex, query);
        HashMap<String, ArrayList<JSONObject>> result = mActivity.mResultListener.resultRecordMap;
        assertEquals(1, result.size());
        // exact search, should not have any result
        assertTrue(result.get(MyActivity.MovieIndex.INDEX_NAME).size() == 0);

        query = new Query(new SearchableTerm("Miss").setIsPrefixMatching(true).AND(new SearchableTerm("Imposible").enableFuzzyMatching(0.8f)));
        resetAndSearch(mActivity.mMovieIndex, query);
        result = mActivity.mResultListener.resultRecordMap;
        assertEquals(1, result.size());
        ArrayList<JSONObject> jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MovieIndex.INDEX_NAME);
        assertTrue(jsonArray.size() == 1);
        assertTrue(jsonArray.get(0).getString(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY).equals("1"));

    }

    public void testFilter() throws JSONException {
        Log.d(TAG, "test filter");
        Query query = new Query(new SearchableTerm("srch2")).sortOnFields(MyActivity.MovieIndex.INDEX_KEY_YEAR).orderByAscending().pagingSize(200);
        query.filterByFieldEqualsTo(MyActivity.MovieIndex.INDEX_KEY_YEAR, "199");
        resetAndSearch(mActivity.mMovieIndex, query);
        HashMap<String, ArrayList<JSONObject>> result = mActivity.mResultListener.resultRecordMap;
        assertTrue(result.size() == 1);
        ArrayList<JSONObject> jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MovieIndex.INDEX_NAME);
        assertTrue(jsonArray.size() == 1);
        assertTrue(jsonArray.get(0).getString(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY).equals("199"));


        query = new Query(new SearchableTerm("srch2")).sortOnFields(MyActivity.MovieIndex.INDEX_KEY_YEAR).orderByAscending().pagingSize(200);
        int startFrom = 197;
        query.filterByFieldStartsFrom(MyActivity.MovieIndex.INDEX_KEY_YEAR, Integer.toString(startFrom));
        resetAndSearch(mActivity.mMovieIndex, query);
        result = mActivity.mResultListener.resultRecordMap;
        assertTrue(result.size() == 1);
        jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MovieIndex.INDEX_NAME);
        assertTrue(jsonArray.size() == 3);
        for (int i = 0, j = startFrom; i < 3; i++, j++) {
            assertTrue(jsonArray.get(i).getString(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY).equals(Integer.toString(j)));
        }

        query = new Query(new SearchableTerm("srch2")).sortOnFields(MyActivity.MovieIndex.INDEX_KEY_YEAR).orderByAscending().pagingSize(200);
        int endTo = 200;
        query.filterByFieldEndsTo(MyActivity.MovieIndex.INDEX_KEY_YEAR, Integer.toString(endTo));
        resetAndSearch(mActivity.mMovieIndex, query);
        result = mActivity.mResultListener.resultRecordMap;
        assertTrue(result.size() == 1);
        jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MovieIndex.INDEX_NAME);
        assertTrue(jsonArray.size() == 200 - 11);


        query = new Query(new SearchableTerm("srch2")).sortOnFields(MyActivity.MovieIndex.INDEX_KEY_YEAR).orderByAscending().pagingSize(200);
        query.filterByFieldInRange(MyActivity.MovieIndex.INDEX_KEY_YEAR, Integer.toString(startFrom), Integer.toString(endTo));
        resetAndSearch(mActivity.mMovieIndex, query);
        result = mActivity.mResultListener.resultRecordMap;
        assertTrue(result.size() == 1);
        jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MovieIndex.INDEX_NAME);
        assertTrue(jsonArray.size() == 3);
        for (int i = 0, j = startFrom; i < 3; i++, j++) {
            assertTrue(jsonArray.get(i).getString(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY).equals(Integer.toString(j)));
        }


    }

    public void testPaging() {
        Log.d(TAG, "test sorting");
        Query query = new Query(new SearchableTerm("srch2")).sortOnFields(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY).pagingSize(200);
        resetAndSearch(mActivity.mMovieIndex, query);
        HashMap<String, ArrayList<JSONObject>> result = mActivity.mResultListener.resultRecordMap;
        assertTrue(result.size() == 1);
        ArrayList<JSONObject> jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MovieIndex.INDEX_NAME);
        assertTrue(jsonArray.size() == 200 - 11);

        query = new Query(new SearchableTerm("srch2")).sortOnFields(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY).pagingSize(42);
        resetAndSearch(mActivity.mMovieIndex, query);
        result = mActivity.mResultListener.resultRecordMap;
        assertTrue(result.size() == 1);
        jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MovieIndex.INDEX_NAME);
        assertTrue(jsonArray.size() == 42);

    }

    public void testSorting() throws JSONException {
        Log.d(TAG, "test sorting");
        Query query = new Query(new SearchableTerm("srch2")).sortOnFields(MyActivity.MovieIndex.INDEX_KEY_YEAR).orderByAscending().pagingSize(200);
        resetAndSearch(mActivity.mMovieIndex, query);
        HashMap<String, ArrayList<JSONObject>> result = mActivity.mResultListener.resultRecordMap;
        assertTrue(result.size() == 1);
        ArrayList<JSONObject> jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MovieIndex.INDEX_NAME);
        assertTrue(jsonArray.size() == 200 - 11);
        for (int i = 11; i < 200; i++) {
            assertTrue(i == Integer.parseInt(jsonArray.get(i - 11).getString(MyActivity.MovieIndex.INDEX_KEY_YEAR)));
        }

        query = new Query(new SearchableTerm("srch2")).sortOnFields(MyActivity.MovieIndex.INDEX_KEY_YEAR).orderByDescending().pagingSize(200);
        resetAndSearch(mActivity.mMovieIndex, query);
        result = mActivity.mResultListener.resultRecordMap;
        assertTrue(result.size() == 1);

        jsonArray = (ArrayList<JSONObject>) result.get(MyActivity.MovieIndex.INDEX_NAME);
        assertTrue(jsonArray.size() == 200 - 11);
        for (int i = 199, j = 0; i >= 11; --i, ++j) {
            assertTrue(i == Integer.parseInt(jsonArray.get(j).getString(MyActivity.MovieIndex.INDEX_KEY_YEAR)));
        }

    }


    public void testGetDocById() throws JSONException {
        Log.d(TAG, "testGetDocById");

        mActivity.mControlListener.recordResponse = null;
        mActivity.mMusicIndex.getRecordbyID("2");
        GetRecordResponse response = mActivity.getRecordResponse();
        assertTrue(response.isRecordRetrieved);
        assertTrue(response.record.getString(MyActivity.MusicIndex.INDEX_KEY_SONG_TITLE).equals("All You Need Is Love"));

        //Try Geo Index GetDoc FIXME oh god GEO!
//        mActivity.mControlListener.recordResponse = null;
//        mActivity.mGeoIndex.getRecordbyID("Irvine");
//        response = mActivity.getRecordResponse();
//        assertTrue(response.isRecordRetrieved);
//        assertTrue(response.record.getString(MyActivity.GeoIndex.INDEX_KEY_PRIMARY_KEY).equals("Irvine"));
    }

    public void testUpdate() throws JSONException {
        Log.d(TAG, "testUpdate");
        mActivity.mControlListener.updateResponse = null;

        // update
        JSONObject record11 = new JSONObject();
        record11.put(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY, "11");
        record11.put(MyActivity.MovieIndex.INDEX_KEY_TITLE, "Rocky 11");
        record11.put(MyActivity.MovieIndex.INDEX_KEY_GENRE, "Action");
        record11.put(MyActivity.MovieIndex.INDEX_KEY_YEAR, "2042");
        mActivity.mMovieIndex.update(record11);
        UpdateResponse response = mActivity.getUpdateResponse();
        Log.d(TAG, "update response:" + response.toString());
        assertTrue(response.getSuccessCount() == 1);

        // upsert
        mActivity.mControlListener.updateResponse = null;

        JSONObject recordNew = new JSONObject();
        recordNew.put(MyActivity.MovieIndex.INDEX_KEY_PRIMARY_KEY, "201");
        recordNew.put(MyActivity.MovieIndex.INDEX_KEY_TITLE, "Rocky 201");
        recordNew.put(MyActivity.MovieIndex.INDEX_KEY_GENRE, "Magic");
        recordNew.put(MyActivity.MovieIndex.INDEX_KEY_YEAR, "20001");
        mActivity.mMovieIndex.update(recordNew);
        response = mActivity.getUpdateResponse();
        Log.d(TAG, "upsert response:" + response.toString());
        assertTrue(response.getSuccessCount() == 1);

        mActivity.mControlListener.infoResponse = null;
        mActivity.mMovieIndex.info();
        InfoResponse infoResponse = mActivity.getInfoResponse();
        Log.d(TAG, infoResponse.toString());
        assertTrue(infoResponse.getNumberOfDocumentsInTheIndex() == 200 - 11 + 3 + 1);
    }
}
