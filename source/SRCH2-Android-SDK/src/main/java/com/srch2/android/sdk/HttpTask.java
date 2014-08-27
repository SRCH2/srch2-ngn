package com.srch2.android.sdk;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

abstract class HttpTask implements Runnable {

    static final String IRRECOVERABLE_NETWORK_ERROR_MESSAGE = "Connection failed. ";
    private static final String TAG = "HttpTask";

    static final class RESTfulResponseTags {
        static final int INVALID_JSON_RESPONSE = -1;
        final static String JSON_KEY_PRIMARY_KEY_INDICATOR = "rid";
        final static String JSON_KEY_LOG = "log";
        final static String JSON_KEY_DELETE = "delete";
        final static String JSON_KEY_INSERT = "insert";
        final static String JSON_KEY_UPDATE = "update";
        final static String JSON_VALUE_UPDATE_FAIL = "failed";
        final static String JSON_VALUE_UPDATED_EXISTS = "Existing record updated successfully";
        final static String JSON_VALUE_UPSERT_SUCCESS = "New record inserted successfully";
        final static String JSON_VALUE_SUCCESS = "success";
        public static final int FAILED_TO_CONNECT_RESPONSE_CODE = -1;
    }

    static private boolean isExecuting = false;
    static private ExecutorService controlTaskExecutor;
    static private ExecutorService searchTaskExecutor;
    static private ExecutorService clientCallbackTaskExecutor;

    private static final int TASK_ID_INSERT_UPDATE_DELETE_GETRECORD = 1;
    private static final int TASK_ID_SEARCH = 2;
    private static final int TASK_ID_CLIENT_CALLBACK = 3;

    protected SearchResultsListener searchResultsListener;



    static synchronized void onStart() {
        isExecuting = true;
        controlTaskExecutor = Executors.newFixedThreadPool(1);
        searchTaskExecutor = Executors.newFixedThreadPool(1);
        clientCallbackTaskExecutor = Executors.newFixedThreadPool(1);
    }

    static synchronized void onStop() {
        isExecuting = false;
        if (controlTaskExecutor != null) {
            controlTaskExecutor.shutdown();
        }
        if (searchTaskExecutor != null) {
            searchTaskExecutor.shutdown();
        }
        if (clientCallbackTaskExecutor != null) {
            clientCallbackTaskExecutor.shutdown();
        }
    }

    static synchronized void executeTask(HttpTask taskToExecute) {
        if (!isExecuting || taskToExecute == null) {
            return;
        }

        int taskId = -1;
        final Class originatingTaskClass = taskToExecute.getClass();
        if (originatingTaskClass == SearchTask.class || originatingTaskClass == CheckCoresLoadedTask.class) {
            taskId = TASK_ID_SEARCH;
        } else if (originatingTaskClass == GetRecordTask.class ||
                     originatingTaskClass == UpdateTask.class ||
                        originatingTaskClass == InsertTask.class ||
                            originatingTaskClass == DeleteTask.class) {
            taskId = TASK_ID_INSERT_UPDATE_DELETE_GETRECORD;
        } else if (originatingTaskClass == InsertResponse.class ||
                        originatingTaskClass == UpdateResponse.class ||
                            originatingTaskClass == DeleteResponse.class ||
                                originatingTaskClass == GetRecordResponse.class ||
                                    originatingTaskClass == IndexIsReadyResponse.class) {
            taskId = TASK_ID_CLIENT_CALLBACK;
        } else {
            throw new IllegalStateException("Meltdown imminent: taskToExecute does not match any assignable task executor");
        }

        if (taskId == TASK_ID_SEARCH) {
            if (searchTaskExecutor != null) {
                searchTaskExecutor.execute(taskToExecute);
            }
        } else if (taskId == TASK_ID_INSERT_UPDATE_DELETE_GETRECORD) {
            if (controlTaskExecutor != null) {
                controlTaskExecutor.execute(taskToExecute);
            }
        } else if (taskId == TASK_ID_CLIENT_CALLBACK) {
            if (clientCallbackTaskExecutor != null) {
                clientCallbackTaskExecutor.execute(taskToExecute);
            }
        }
    }

    static abstract class SingleCoreHttpTask extends HttpTask {
        final String targetCoreName;
        SingleCoreHttpTask(String theTargetCoreName) {
            targetCoreName = theTargetCoreName;
        }
    }

    static abstract class InsertUpdateDeleteTask extends SingleCoreHttpTask {
        final URL targetUrl;

        public InsertUpdateDeleteTask(final URL theTargetUrl, final String theTargetCoreName) {
            super(theTargetCoreName);
            targetUrl = theTargetUrl;
        }

        protected void onTaskComplete(int returnedResponseCode,
                                      String returnedResponseLiteral) {
            updateIndexableIndexInformation(SRCH2Engine.conf.indexableMap.get(targetCoreName));
            SRCH2Engine.resetHeartBeatPing();
            if (SRCH2Engine.isChanged.get()) {
                SRCH2Engine.reQueryLastOne();
            }
        }
    }

    void updateIndexableIndexInformation(Indexable indexableToUpdate) {
        if (indexableToUpdate != null) {
            InternalInfoTask iit = new InternalInfoTask(UrlBuilder
                    .getInfoUrl(
                            SRCH2Engine.conf,
                            indexableToUpdate.indexInternal.indexDescription));
            indexableToUpdate.setRecordCount(iit.getInfo().numberOfDocumentsInTheIndex);
        }
    }

    static class DeleteResponse extends SingleCoreHttpTask {
        final int success;
        final int failed;
        final String jsonResponse;

        DeleteResponse(String theTargetCoreName, int successfulDeletions, int failedDeletions, String jsonResponseLiteral) {
            super(theTargetCoreName);
            success = successfulDeletions;
            failed = failedDeletions;
            jsonResponse = jsonResponseLiteral;
        }

        @Override
        public void run() {
            Indexable idx = SRCH2Engine.conf.indexableMap.get(targetCoreName);
            if (idx != null) {
                idx.onDeleteComplete(success, failed, jsonResponse);
            }
        }
    }

    static class InsertResponse extends SingleCoreHttpTask {
        final int success;
        final int failed;
        final String jsonResponse;

        InsertResponse(String theTargetCoreName, int successfulDeletions, int failedDeletions, String jsonResponseLiteral) {
            super(theTargetCoreName);
            success = successfulDeletions;
            failed = failedDeletions;
            jsonResponse = jsonResponseLiteral;
        }

        @Override
        public void run() {
            Indexable idx = SRCH2Engine.conf.indexableMap.get(targetCoreName);
            if (idx != null) {
                idx.onInsertComplete(success, failed, jsonResponse);
            }
        }
    }

    static class UpdateResponse extends SingleCoreHttpTask {
        final int success;
        final int upserts;
        final int failed;
        final String jsonResponse;

        UpdateResponse(String theTargetCoreName, int successfulDeletions, int successfulUpserts, int failedDeletions, String jsonResponseLiteral) {
            super(theTargetCoreName);
            success = successfulDeletions;
            upserts = successfulUpserts;
            failed = failedDeletions;
            jsonResponse = jsonResponseLiteral;
        }

        @Override
        public void run() {
            Indexable idx = SRCH2Engine.conf.indexableMap.get(targetCoreName);
            if (idx != null) {
                idx.onUpdateComplete(success, upserts, failed, jsonResponse);
            }
        }
    }

    static class GetRecordResponse extends SingleCoreHttpTask {
        final boolean success;
        final JSONObject retrievedRecord;
        final String jsonResponse;

        GetRecordResponse(String theTargetCoreName, boolean wasRecordRetrieved, JSONObject theRetrievedRecord, String jsonResponseLiteral) {
            super(theTargetCoreName);
            success = wasRecordRetrieved;
            if (success) {
                retrievedRecord = theRetrievedRecord;
            } else {
                retrievedRecord = new JSONObject();
            }
            jsonResponse = jsonResponseLiteral;
        }

        @Override
        public void run() {
            Indexable idx = SRCH2Engine.conf.indexableMap.get(targetCoreName);
            if (idx != null) {
                idx.onGetRecordComplete(success, retrievedRecord, jsonResponse);
            }
        }
    }

    static class IndexIsReadyResponse extends SingleCoreHttpTask {
        IndexIsReadyResponse(String theTargetCoreName) {
            super(theTargetCoreName);
        }

        @Override
        public void run() {
            Indexable idx = SRCH2Engine.conf.indexableMap.get(targetCoreName);
            if (idx != null) {
                idx.onIndexReady();
            }
        }
    }

    static abstract class SearchHttpTask extends SingleCoreHttpTask {
        final URL targetUrl;

        SearchHttpTask(final URL theTargetUrl, final String theTargetCoreName, final SearchResultsListener theSearchResultsListener) {
            super(theTargetCoreName);
            targetUrl = theTargetUrl;
            searchResultsListener = theSearchResultsListener;
        }

        abstract protected void onTaskComplete(int returnedResponseCode,
                                               String returnedResponseLiteral);
    }

	static String handleStreams(HttpURLConnection connection, String internalClassLogcatTag) throws IOException {
        String response = null;

        if (connection != null) {
            boolean hasValidInputStreamData = true;

            if (connection.getInputStream() != null) {
                response = readInputStream(connection.getInputStream());

                if (response == null || (response != null && response.trim().length() < 1)) {
                    Cat.d(internalClassLogcatTag, "INPUT STREAM data is null");
                    hasValidInputStreamData = false;
                    response = null;
                } else {
                    Cat.d(internalClassLogcatTag, "INPUT STREAM is _" + response + "_");
                }
            }else {
                Cat.d(internalClassLogcatTag, "INPUT STREAM NULL");
            }

            if (!hasValidInputStreamData) {
                if (connection.getErrorStream() != null) {
                    response = readInputStream(connection.getErrorStream());
                    if (response == null || (response != null && response.trim().length() < 1)) {
                        Cat.d(internalClassLogcatTag, "ERROR STREAM data is null");
                        response = null;
                    } else {
                        Cat.d(internalClassLogcatTag, "ERROR STREAM is _" + response + "_");
                    }
                } else {
                    Cat.d(internalClassLogcatTag, "ERROR STREAM NULL");
                }
            }
        }
        if (response == null) {
            response = prepareIOExceptionMessageForCallback();
        }
        return response;
    }

    static String handleIOExceptionMessagePassing(IOException ioException, String response, String internalClassLogcatTag) {
        String errorResponse = null;
        if (ioException != null) {
            errorResponse = ioException.getMessage();
            if (errorResponse == null || (errorResponse != null && errorResponse.trim().length() < 1)) {
                errorResponse = null;
                Cat.d(internalClassLogcatTag, "IOEXCEPTION message is NULL");
            } else {
                Cat.d(internalClassLogcatTag, "IOEXCEPTION message is " + errorResponse);
            }
            Cat.ex(TAG, "httptask handleIOexception", ioException);
        }
        if (errorResponse == null) {
            response = prepareIOExceptionMessageForCallback();
        } else {
            response = prepareIOExceptionMessageForCallback(errorResponse);
        }
        return response;
    }

    static String prepareIOExceptionMessageForCallback(String message) {
        return IRRECOVERABLE_NETWORK_ERROR_MESSAGE + " IOException message: " + message;
    }

    static String prepareIOExceptionMessageForCallback() {
        return IRRECOVERABLE_NETWORK_ERROR_MESSAGE + " IOException message: irrecoverable error";
    }

    /**
     * Reads from an input stream, returning the data read as a string encoded
     * as UTF-8. If the <code>InputStream source</code> is null, the returned
     * string will contain "NULL SOURCE INPUT STREAM".
     */
    static String readInputStream(InputStream source) throws IOException {
        if (source == null) {
            return "NULL SOURCE INPUT STREAM";
        }
        StringBuilder sb = new StringBuilder();
        String line;
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new InputStreamReader(source, "UTF-8"));
            while ((line = reader.readLine()) != null) {
                sb.append(line);
            }
        } finally {
            if (reader != null) {
                reader.close();
            }
        }
        return sb.toString();
    }
	/*
	 * 
	 * SAVE for if we want to not use static, single instance of the thread
	 * pools in the future: ie the user wants multiple engine / client objects
	 * for some reason
	 * 
	 * // Wrapper class for controlling execution of tasks to be executed. Users
	 * of this class <b>must</b> call <code>onStart()</code> and
	 * <code>onStop()</code>. Users should // also obtain an instance of this
	 * class by calling <code>HttpTask.getHttpTaskExecutor()</code>. static
	 * class HttpTaskExecutor { private boolean isExecuting = false; private
	 * ExecutorService controlTaskExecutor; private ExecutorService
	 * searchTaskExecutor;
	 * 
	 * synchronized void onStart() { isExecuting = true; controlTaskExecutor =
	 * Executors.newFixedThreadPool(1); searchTaskExecutor =
	 * Executors.newFixedThreadPool(1); }
	 * 
	 * synchronized void onStop() { isExecuting = false;
	 * controlTaskExecutor.shutdown(); searchTaskExecutor.shutdown(); }
	 * 
	 * synchronized protected void executeTask(HttpTask taskToExecte) { if
	 * (!isExecuting || taskToExecte == null) { return; }
	 * 
	 * boolean isSearchTask = taskToExecte.getClass() == SearchTask.class;
	 * 
	 * if (isSearchTask && searchTaskExecutor != null) { try {
	 * searchTaskExecutor.execute(taskToExecte); } catch
	 * (RejectedExecutionException ignore) { } } else if (!isSearchTask &&
	 * controlTaskExecutor != null) { try {
	 * controlTaskExecutor.execute(taskToExecte); } catch
	 * (RejectedExecutionException ignore) { } } } }
	 * 
	 * static HttpTaskExecutor getHttpTaskExecutor() { return new
	 * HttpTaskExecutor(); }
	 */
}
