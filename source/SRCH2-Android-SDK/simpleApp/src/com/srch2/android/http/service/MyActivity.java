package com.srch2.android.http.service;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import junit.framework.Assert;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static junit.framework.Assert.assertTrue;

public class MyActivity extends Activity {
    public static final String TAG = "srch2:: MyActivity";

    public TestSearchResultsListener mResultListener = new TestSearchResultsListener();
    public TestControlResponseListener mControlListener = new TestControlResponseListener();

    public TestableIndex mIndex1 = new TestIndex();
    public TestableIndex mIndex2 = new TestIndex2();
    public TestGeoIndex mIndexGeo = new TestGeoIndex();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle(TAG);

        Log.d(TAG, "onCreate");
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.d(TAG, "onStart");
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.d(TAG, "onStop");
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        Log.d(TAG, "onRestart");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
    }

    public void initializeSRCH2EngineAndCallStart() {
        initializeSRCH2Engine();
        try {
            Thread.currentThread().sleep(300);
        } catch (InterruptedException e) {

        }
        callSRCH2EngineStart();
    }

    public void initializeSRCH2Engine() {
        SRCH2Engine.initialize(mIndex1, mIndex2, mIndexGeo);
        SRCH2Engine.setSearchResultsListener(mResultListener);
        SRCH2Engine.setStateResponseListener(mControlListener);
        SRCH2Engine.setTestAndDebugMode(true);
    }

    public void callSRCH2EngineStart() {
        SRCH2Engine.onStart(this);
    }

    public void callSRCH2EngineStop() {
        SRCH2Engine.onStop(this);
    }

    private void reset() {
        mControlListener.reset();
        mResultListener.reset();
    }

    public InfoResponse getInfoResponse(){
        Util.waitForResponse(mControlListener, InfoResponse.class);
        return mControlListener.infoResponse;
    }

    public InsertResponse getInsertResponse() {
        Util.waitForResponse(mControlListener, InsertResponse.class);
        return mControlListener.insertResponse;
    }

    public DeleteResponse getDeleteResponse() {
        Util.waitForResponse(mControlListener, DeleteResponse.class);
        return mControlListener.deleteResponse;
    }

    public UpdateResponse getUpdateResponse() {
        Util.waitForResponse(mControlListener, UpdateResponse.class);
        return mControlListener.updateResponse;
    }

    public GetRecordResponse getRecordResponse() {
        Util.waitForResponse(mControlListener, GetRecordResponse.class);
        return mControlListener.recordResponse;
    }

    public SearchResultsListener getSearchResult() {
        Util.waitForResultResponse(mResultListener);
        return mResultListener;
    }

    public void waitForEngineReady() {
        Util.waitSRCH2EngineIsReady();
    }

    public void testAll(){
        testStartEngine(mIndex1, mIndex2);
        try {
            for(TestableIndex index : new TestableIndex[] {mIndex1, mIndex2} ) {
                testOneRecordCRUD(index);
                testBatchRecordCRUD(index);
            }
        } catch (JSONException e) {
            Assert.fail();
        }
    }

    public void testStartEngine(TestableIndex ... indexes){
        waitForEngineReady();
        assertTrue(mControlListener.indexesInfoResponseMap.size() == indexes.length);
        for(TestableIndex index : indexes){
            assertTrue(mControlListener.indexesInfoResponseMap.get(index.getIndexDescription().getIndexName()).getNumberOfDocumentsInTheIndex()== 0);
        }
    }


    public void testOneRecordCRUD(TestableIndex index) throws JSONException {
        JSONObject record = index.getSucceedToInsertRecord();

        testInsertShouldSuccess(index, record);

        testInsertShouldFail(index, index.getFailToInsertRecord());

        testGetRecordIdShouldSuccess(index, new JSONArray(Arrays.asList(record)));

        testSearchStringShouldSuccess(index, index.getSucceedToSearchString(new JSONArray(Arrays.asList(record))));

        testSearchStringShouldFail(index, index.getFailToSearchString(new JSONArray(Arrays.asList(record))));

        testSearchQueryShouldSuccess(index, index.getSucceedToSearchQuery(new JSONArray(Arrays.asList(record))));

        testSearchQueryShouldFail(index, index.getFailToSearchQuery(new JSONArray(Arrays.asList(record))));

        testUpdateExistShouldSuccess(index, index.getSucceedToUpdateExistRecord());

        testUpdateNewShouldSuccess(index, index.getSucceedToUpsertRecord());

        testUpdateShouldFail(index, index.getFailToUpdateRecord());

        testDeleteShouldSuccess(index, Arrays.asList(record.getString(index.getPrimaryKeyFieldName())));

        testDeleteShouldFail(index, index.getFailToDeleteRecord());
    }


    public void testBatchRecordCRUD(TestableIndex index) throws JSONException {
        JSONArray records = index.getSucceedToInsertBatchRecords();

        testBatchInsertShouldSuccess(index, records);

        testGetRecordIdShouldSuccess(index, records);

        testBatchInsertShouldFail(index, index.getFailToInsertBatchRecord());

        testSearchStringShouldSuccess(index, index.getSucceedToSearchString(records));

        testSearchStringShouldFail(index, index.getFailToSearchString(records));

        testSearchQueryShouldSuccess(index, index.getSucceedToSearchQuery(records));

        testSearchQueryShouldFail(index, index.getFailToSearchQuery(records));

        testBatchUpdateShouldSuccess(index, index.getSucceedToUpdateBatchRecords());

        testBatchUpdateShouldFail(index, index.getFailToUpdateBatchRecords());

        ArrayList<String> ids = new ArrayList<String> ();
        for(int i = 0; i < records.length(); ++i){
            ids.add(records.getJSONObject(i).getString(index.getPrimaryKeyFieldName()));
        }

        testDeleteShouldSuccess(index, ids);

        testDeleteShouldFail(index, ids);

    }

    private void testGetRecordIdShouldSuccess(TestableIndex index, JSONArray records) throws JSONException {
        for(int i = 0; i < records.length(); i++){
            mControlListener.recordResponse = null;
            index.getRecordbyID(records.getJSONObject(i).getString(index.getPrimaryKeyFieldName()));
            getRecordResponse();
            assertTrue(mControlListener.recordResponse.record.toString().equals(records.getJSONObject(i).toString()));
        }
    }

    public void testMultiCoreSearch(TestableIndex index1, TestableIndex ... restIndex){
        //TODO
    }


    public void testInsertShouldSuccess(TestableIndex index, JSONObject record) {
        mControlListener.insertResponse = null;
        index.insert(record);
        getInsertResponse();
        assertTrue(mControlListener.insertResponse.getSuccessCount() == 1);
        assertTrue(mControlListener.insertResponse.getFailureCount() == 0);
    }

    public void testInsertShouldFail(TestableIndex index, JSONObject record){
        mControlListener.insertResponse = null;
        index.insert(record);
        getInsertResponse();
        assertTrue(mControlListener.insertResponse.getSuccessCount() == 0);
        assertTrue(mControlListener.insertResponse.getFailureCount() == 1);
    }

    public void testBatchInsertShouldSuccess(TestableIndex index, JSONArray array){
        mControlListener.insertResponse = null;
        index.insert(array);
        getInsertResponse();
        assertTrue(mControlListener.insertResponse.getSuccessCount()== array.length());
        assertTrue(mControlListener.insertResponse.getFailureCount()== 0);
    }

    public void testBatchInsertShouldFail(TestableIndex index, JSONArray array){
        mControlListener.insertResponse = null;
        index.insert(array);
        getInsertResponse();
        assertTrue(mControlListener.insertResponse.getFailureCount()== array.length());
        assertTrue(mControlListener.insertResponse.getSuccessCount()== 0);
    }

    public void testSearchStringShouldSuccess(TestableIndex index, List<String> queries){
        for(String query : queries){
            mResultListener.reset();
            index.search(query);
            getSearchResult();
            assertTrue(mResultListener.resultRecordMap.size() == 1);
            assertTrue(mResultListener.resultRecordMap.get(index.getIndexDescription().getIndexName()) != null);
            assertTrue(index.verifyResult(query, mResultListener.resultRecordMap.get(index.getIndexDescription().getIndexName())));
        }
    }

    public void testSearchStringShouldFail(TestableIndex index, List<String> queries){
        for(String query : queries){
            mResultListener.reset();
            index.search(query);
            getSearchResult();
            assertTrue(mResultListener.resultRecordMap.size() == 1);
            assertTrue(mResultListener.resultRecordMap.get(index.getIndexDescription().getIndexName()).size() == 0);
        }
    }

    public void testSearchQueryShouldSuccess(TestableIndex index, List<Query> queries){
        for(Query query: queries){
            mResultListener.reset();
            index.advancedSearch(query);
            getSearchResult();
            assertTrue(mResultListener.resultRecordMap.size() == 1);
            assertTrue(mResultListener.resultRecordMap.get(index.getIndexDescription().getIndexName()) != null);
            assertTrue(index.verifyResult(query, mResultListener.resultRecordMap.get(index.getIndexDescription().getIndexName())));
        }
    }

    public void testSearchQueryShouldFail(TestableIndex index, List<Query> queries){
        for(Query query : queries){
            mResultListener.reset();
            index.advancedSearch(query);
            getSearchResult();
            assertTrue(mResultListener.resultRecordMap.size() == 1);
            assertTrue(mResultListener.resultRecordMap.get(index.getIndexDescription().getIndexName()).size() == 0);
        }
    }

    public void testUpdateExistShouldSuccess(TestableIndex index, JSONObject record){
        mControlListener.updateResponse = null;
        index.update(record);
        getUpdateResponse();
        assertTrue(mControlListener.updateResponse.getExistRecordUpdatedSuccessCount()== 1);
        assertTrue(mControlListener.updateResponse.getNewRecordInsertedSuccessCount()== 0);
        assertTrue( mControlListener.updateResponse.getFailureCount()== 0);
    }

    public void testUpdateNewShouldSuccess(TestableIndex index, JSONObject record){
        mControlListener.updateResponse = null;
        index.update(record);
        getUpdateResponse();
        assertTrue(mControlListener.updateResponse.getExistRecordUpdatedSuccessCount()== 0);
        assertTrue(mControlListener.updateResponse.getNewRecordInsertedSuccessCount()== 1);
        assertTrue(mControlListener.updateResponse.getFailureCount()== 0);
    }

    public void testUpdateShouldFail(TestableIndex index, JSONObject record){
        mControlListener.updateResponse = null;
        index.update(record);
        getUpdateResponse();
        assertTrue(mControlListener.updateResponse.getSuccessCount()== 0);
        assertTrue(mControlListener.updateResponse.getFailureCount()== 1);
    }

    public void testBatchUpdateShouldSuccess(TestableIndex index, JSONArray array){
        mControlListener.updateResponse = null;
        index.update(array);
        getUpdateResponse();
        assertTrue(mControlListener.updateResponse.getSuccessCount() == array.length());
        assertTrue(mControlListener.updateResponse.getFailureCount()== 0);
    }

    public void testBatchUpdateShouldFail(TestableIndex index, JSONArray array){
        mControlListener.updateResponse = null;
        index.update(array);
        getUpdateResponse();
        assertTrue(mControlListener.updateResponse.getSuccessCount() == 0);
        assertTrue(mControlListener.updateResponse.getFailureCount()== array.length());
    }

    public void testDeleteShouldSuccess(TestableIndex index, List<String> ids){
        for( String id : ids) {
            mControlListener.deleteResponse = null;
            index.delete(id);
            getDeleteResponse();
            assertTrue( mControlListener.deleteResponse.getSuccessCount() == 1);
        }
    }

    public void testDeleteShouldFail(TestableIndex index, List<String> ids){
        for(String id: ids){
            mControlListener.deleteResponse = null;
            index.delete(id);
            getDeleteResponse();
            assertTrue( mControlListener.deleteResponse.getFailureCount() == 1);
        }
    }

}

