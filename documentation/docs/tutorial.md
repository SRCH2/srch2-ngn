Hello and welcome to the Hello SRCH2 Search Tutorial for the SRCH2 Android SDK.

###Introduction
 
This tutorial will show you how to:

1. Incorporate the SRCH2 Android SDK into your Android Studio project
2. Create indexes and insert records
3. Enable powerful search in your app and publish the search results to the UI or main thread
 
To get started, this tutorial assumes you are familiar with Android Studio and can create, or have already created, Android Studio projects. For instructions how to set up the SRCH2 Android SDK on eclipse, see [Advanced Topics](advanced-topics.md). 

You can download the [Hello SRCH2 Search Demo project](../download/hello-srch2-search-demo-files/hello-srch2-search-demo-project.zip) and import it into Android Studio and read the tutorial to understand the code. Or, if you want to follow along with this tutorial, create a project from scratch and refer to the source code by browsing the project at our [GitHub repository](http://www.endoftheinternet.com/). 

###Installing the SDK
 
The first step is to obtain the SRCH2 Android SDK ``.aar`` file. An `.aar` file extension is an expansion of the JAR file format to include Android native resources, which the SRCH2 Android SDK requires to get the SRCH2 search engine running. For more information on the `.aar` file format, you can refer to [AAR Format](http://tools.android.com/tech-docs/new-build-system/aar-format) at the Android Tools Project Site.

You can download the `SRCH2-Android-SDK.aar` file by clicking [here](../download/latest-releases/SRCH2-Android-SDK.aar).
 
*Quick Steps*:

1. Open up Android Studio.
2. Goto File->New Module and select 'Import JAR / AAR file' from the list of options.
3. Select the `SRCH2-Android-SDK.aar` file from its download location.
4. Click 'Finish'.
5. Under ProjectName->app, open build.gradle and add `compile project(':SRCH2-Android-SDK')` to the dependencies node.
 
*Interactive Steps*:

1. Open up Android Studio.
2. Create a new Android project called `HelloSRCH2SearchDemo`, declared with the package name `com.srch2.android.demo.hellosrch2` and 14 set as the minimum API version in this example.
3. Choose to create the project with a blank activity, named `MyActivity` in this example.
4. You should now have a new project open in Android Studio. To incorporate the SRCH2 Android SDK, you will first need to add the `SRCH2-Android-SDK.aar` file to the project as a new module. To do this, open up your project's 'Module Settings' by right clicking into your 'Project' view and select 'Module Settings':<br>![Module Settings - How to open 'Module Settings'][tutorial-000]
5. Move the cursor to the upper-left hand of the dialogue box, and click the green addition symbol. This will add a new module to your project. This will look something like:<br>![Module Settings - How to add a new module][tutorial-001]
6. From the list of options, select 'Import .JAR or .AAR Package' and click Next.<br>![Module Settings - Creating a new module from an .AAR file][tutorial-002]
7. Select `SRCH2-Android-SDK.aar` from its download location, you can leave value of the field 'Subproject Name' the same, and click Finish.<br>![Module Settings - Importing the SRCH2-Android-SDK.aar file as the new module][tutorial-003]
8. Now that you've added the `SRCH2-Android-SDK.aar` as a new module in your project, it's time to add it as a dependency to your app. From the 'Module Settings' dialog, click on your app from the 'Modules' column (on the left-hand side, SRCH2-Android-SDK.aar should also be in the list)
9. Select the 'Dependencies' tab, move the cursor to the right side of the dialog. Click the green addition symbol, and from the drop down list select 'Module dependency'. This should look something like this:<br>![Module Settings - Adding the SRCH2-Android-SDK module as a dependency][tutorial-004]
10. Select `SRCH2-Android-SDK` from the list of modules. Click OK.<br>![Module Settings - Selecting the SRCH2-Android-SDK module][tutorial-005]
11. Now can click Apply and OK, or just OK to finish this process and close 'Module Settings'.<br>![Module Settings - Verifying SRCH2-Android-SDK module as been added as a compiled dependency][tutorial-006]
12. For those curious, this process added the `SRCH2-Android-SDK.aar` as a new module, then altered the build.gradle file in your app's subdirectory to include the `SRCH2-Android-SDK` module to be compiled as a dependency; which you can verify by navigating to this file, and it will look something like:![Installation Complete - Confirming gradle has included the SRCH2 Android SDK as a dependency in your project][tutorial-007]
13. Now you are ready to start accessing the API

###Including Default Project Files

This tutorial covers how the set-up and use the SRCH2 Android SDK in an example project you have just created. Certain class files will not be explained in this tutorial as they are beyond the scope of this tutorial. Before starting to add the code relating to the SRCH2 Android SDK, you need to copy the following classes and files into your project. Refer to [Hello SRCH2 Search Demo GitHub repository](http://www.endoftheinternet.com/) for the source code and resource files you will need to copy. 

1. Copy the set of [resource drawable files `instant_search_edit_text_clear_input_action.png`](../download/hello-srch2-search-demo-files/instant_search_edit_text_clear_input_action_icon.zip) into each of the corresponding container folders (`res-drawable\hpdi`; `res-drawable\mdpi`; `res-drawable\xhdpi`; `res-drawable\xxhdpi`).
2. Create a class called `InstantSearchEditText` in the same package as your `MyActivity` class. Copy the contents of its [source code](../download/hello-srch2-search-demo-files/InstantSearchEditText.txt) implementation. This is a subclass of the standard `EditText` with the interface observer `InstantSearchEditText.SearchInputEnteredObserver`. By implementing this interface, any time a user enters a new character the callback `public void onNewSearchInput(String newSearchText)` will be triggered; or, if the user clears the input by pressing the clear input icon or deleting all of the characters, the callback `public void onNewSearchInputIsBlank()` will be triggered. 
3. Create and copy the [resource layout file `activity_my.xml`](../download/hello-srch2-search-demo-files/activity_my.txt) into your project's `res/layout` folder. Make sure the package name matches your package name in the fully qualified name declaration of the `InstantSearchEditText`. For example in this demo it is declared as:
>`<com.srch2.android.demo.hellosrch2.InstantSearchEditText ... />`
4. Create the `MovieSearchResult` class and copy its [source code](../download/hello-srch2-search-demo-files/MovieSearchResult.txt). This class will be used as the model for the search results as they are displayed in a `ListView` and used the the adapter backing this list view. 
5. Create and copy the [resource layout file `listview_search_result_row.xml`](../download/hello-srch2-search-demo-files/listview_search_result_row.txt). This layout file will be how each row view is inflated for the list view.
6. The last class to add is the the `SearchResultsAdapter` class. Create a class with this name and copy from its [source code](../download/hello-srch2-search-demo-files/SearchResultsAdapter.txt) *everything but* the nested `SearchResultsUiHandler` class because implementing this handler will be explained later in this tutorial.  

Your project's file hierarchy should now look something like:<br>![Default Project Files - Project hierarchy after adding all the files][tutorial-009]
 

###Creating an Index

The first step to searching an index is creating an index. In this tutorial, we'll create an index to search on the title and genre of some movies. 

Create the index for movies by creating a new class that extends `com.srch2.android.http.service.Indexable`. In this example this class will be named `MovieIndex`. An index is defined by a schema, which describes the fields in the index such as their type and default value; the fields of the schema are comparable to the columns of an SQLITE table. To define the schema for the `MovieIndex`, you will need to override the `Indexable` abstract method `public IndexDescription getIndexDescription()`. Since we'll reuse the name of the index and the names of the schema fields later to prepare the results of a search, declare and define the following constant field members for this class:

```
public class MovieIndex extends Indexable {

	public static final String INDEX_NAME = "movies";
	public static final String INDEX_FIELD_PRIMARY_KEY = "id";
	public static final String INDEX_FIELD_TITLE = "title";
	public static final String INDEX_FIELD_YEAR = "year";
	public static final String INDEX_FIELD_GENRE = "genre";
	
}
```

And then the method `public IndexDescription getIndexDescription()` can be implemented as follows:

```
public class MovieIndex extends Indexable {

	...
	
    @Override
    public IndexDescription getIndexDescription() {
        Field primaryKey = Field.getRefiningField(INDEX_FIELD_PRIMARY_KEY,
                Field.Type.INTEGER);
        Field title = Field.getSearchableField(INDEX_FIELD_TITLE, 3);
        Field year = Field.getSearchableAndRefiningField(INDEX_FIELD_YEAR,
                Field.Type.INTEGER);
        Field genre = Field.getSearchableField(INDEX_FIELD_GENRE);
        return new IndexDescription(INDEX_NAME, primaryKey, title, year, genre);
    }
}
```

`Field` instances can be obtained by calling the static factory methods of the class `com.srch2.android.http.service.Field`. A searchable field has textual data that will be searched during a query; a refining field is for storing data values that can be used for query filtering and post-processing. Both refining and searchable field data can be retrieved from the raw search results returned by the SRCH2 search server. An additional parameter can be passed which will set the field's relative ranking (or relevance) to other searchable fields--the default value is one. Here, the relevance of the `Field title` is set to be three times more important than the fields `genre` and `year`.

**An index always requires one field to be defined as the primary key**: each record should have a unique value for its primary key. It can be searchable, and/or refining. The primary key is the handle by which you can verify records were inserted correctly, deleted and/or retrieved by later.

When returning the `new IndexDescription(INDEX_NAME, primaryKey, title, year, genre)` the first parameter is **always** the name of index, the second parameter **should always** be the primary key, and each subsequent parameter are the rest of the fields you define, in no particular order. 

The `SRCH2Engine` will use this `IndexDescription` to automatically create the configuration for this index. While we're in this class, let's create a method for inserting some records we can search on. Add a method `public static JSONArray getAFewRecordsToInsert()` implemented as follows:

```
    public static JSONArray getAFewRecordsToInsert() {
        JSONArray jsonRecordsToInsert = new JSONArray();
        try {
            JSONObject record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 1);
            record.put(INDEX_FIELD_TITLE, "The Good, the Bad And the Ugly");
            record.put(INDEX_FIELD_YEAR, 1966);
            record.put(INDEX_FIELD_GENRE, "Western Adventure");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 2);
            record.put(INDEX_FIELD_TITLE, "Citizen Kane");
            record.put(INDEX_FIELD_YEAR, 1941);
            record.put(INDEX_FIELD_GENRE, "Mystery Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 3);
            record.put(INDEX_FIELD_TITLE, "大红灯笼高高挂 (Raise the Red Lantern)");
            record.put(INDEX_FIELD_YEAR, 1991);
            record.put(INDEX_FIELD_GENRE, "Drama");
            jsonRecordsToInsert.put(record);
        } catch (JSONException oops) {
            // We know there are no errors
        }
        return jsonRecordsToInsert;
    }
```

The `SRCH2Engine` accepts a `JSONObject` or a `JSONArray` of `JSONObject`s when inserting or updating records. Insertions (and updates) are invoked by calling `insert(...)` on your `Indexable` implementation, or by specifying the index name and calling `SRCH2Engine.insertIntoIndex(String indexName, ...)`. Before actually calling this method it is time to set up the two asynchronous callbacks so that you can get index information and search results from the SRCH2 search server.

###Getting Search Results

The SRCH2 search server passes information back to the `SRCH2Engine` through two asynchronous callbacks: `com.srch2.android.http.service.SearchResultsListener` and `com.srch2.android.http.service.StateResponseListener`. The first passes back search results from queries performed while the second passes back information about the indexes and engine (covered in the next section). These two interfaces *should be* implemented by you--in particular, the `SearchResultsListener` is the way in which you will receive search results. 

For the demonstration of this tutorial it is assumed you are familiar with how a [`android.widget.BaseAdapter`](http://developer.android.com/reference/android/widget/BaseAdapter.html) works to populate a [`android.widget.ListView`](http://developer.android.com/reference/android/widget/ListView.html). Familiarity with how to communicate with the UI thread via a [`android.os.Handler`](http://developer.android.com/reference/android/os/Handler.html) is also helpful, but you can read more about this on the Android developer website, [Communicating with the UI Thread](https://developer.android.com/training/multiple-threads/communicate-ui.html). 

These two callbacks will execute in background threads, so in order to update the user interface of your application the search results must be passed to the UI thread. In this tutorial we will use subclass of the `android.os.Handler` class to implement the `SearchResultsListener` interface; however, other means of propagating the search results to the UI thread can be implemented, such as `runOnUiThread()` can be used. 

In the source code of this tutorial there is the class `SearchResultsAdapter` which extends `BaseAdapter`; here we will cover how to connect this adapter to a handler implementing `SearchResultsListener`. Create the class `SearchResultsAdapter` and copy and paste the `BaseAdapter`'s implementation, skipping the nested `Handler` class. At the end of this class declare the following nested subclass of `Handler`:

```
public class SearchResultsAdapter extends BaseAdapter {

	...

    private static class SearchResultsUiHandler extends Handler implements
            SearchResultsListener {
	}
}
```

In this class, two constant `int` fields that will be used as the message keys for this handler:

```
public class SearchResultsAdapter extends BaseAdapter {

	...
	
    private static class SearchResultsUiHandler extends Handler implements
            SearchResultsListener {
			
		private static final int MESSAGE_WHAT_PUBLISH_NEW_RESULTS = 001;
		private static final int MESSAGE_WHAT_PUBLISH_NO_NEW_RESULTS = 002;

	}
}
```

Also declare a `WeakReference<T>` of type `SearchResultsAdapter`. The `WeakReference<SearchResultsAdapter>` is necessary to prevent leaking the context (you can read more about why this is necessary [here](http://www.androiddesignpatterns.com/2013/01/inner-class-handler-memory-leak.html)) and can be defined in the constructor such as:

```
public class SearchResultsAdapter extends BaseAdapter {

	...
	
    private static class SearchResultsUiHandler extends Handler implements
            SearchResultsListener {
			
		... 
		
		private final WeakReference<SearchResultsAdapter> mSearchResultsAdapterWeakReference;
		
        public SearchResultsUiHandler(SearchResultsAdapter searchResultsAdapter) {
            mSearchResultsAdapterWeakReference = new WeakReference<SearchResultsAdapter>(
                    searchResultsAdapter);
        }
	}
}
```

Since this handler implements `SearchResultsListener`, override its interface method `public void onNewSearchResults(int httpResponseCode, String jsonResultsLiteral, HashMap<String, ArrayList<JSONObject>>)` in the following way:

```
public class SearchResultsAdapter extends BaseAdapter {

	...
	
    private static class SearchResultsUiHandler extends Handler implements
            SearchResultsListener {
			
		... 
		
        @Override
        public void onNewSearchResults(int httpResponseCode,
                                       String jsonResultsLiteral,
                                       HashMap<String, ArrayList<JSONObject>> resultRecordMap) {
            if (httpResponseCode == HttpURLConnection.HTTP_OK) {
                ArrayList<MovieSearchResult> newResults = new ArrayList<MovieSearchResult>();

                ArrayList<JSONObject> movieResults = resultRecordMap
                        .get(MovieIndex.INDEX_NAME);
                if (movieResults != null && movieResults.size() > 0) {
                    for (JSONObject jsonObject : movieResults) {
                        MovieSearchResult searchResult = null;
                        try {
                            searchResult = new MovieSearchResult(
                                    jsonObject
                                            .getString(MovieIndex.INDEX_FIELD_TITLE),
                                    jsonObject
                                            .getString(MovieIndex.INDEX_FIELD_GENRE),
                                    jsonObject
                                            .getInt(MovieIndex.INDEX_FIELD_YEAR));
                        } catch (JSONException oops) {
                            continue;
                        }

                        if (searchResult != null) {
                            newResults.add(searchResult);
                        }
                    }
                }
                sendMessage(Message
                        .obtain(this,
                                newResults.size() > 0 ? MESSAGE_WHAT_PUBLISH_NEW_RESULTS
                                        : MESSAGE_WHAT_PUBLISH_NO_NEW_RESULTS,
                                newResults));
            }
        }
	}
}
```

Any time one of the search methods of the API is called (such as `MovieIndex.search(String searchInput)`), `onNewSearchResults(...)` will be triggered when the search results are returned from the SRCH2 search server. Its parameters are:

1. `int httpResponseCode` which indicates how the RESTful action was handled
2. `String jsonResultsLiteral` which is the raw JSON literal as returned by the SRCH2 search server containing the full set of search result data
3. `HashMap<String, ArrayList<JSONObject>> resultRecordMap` which is a mapping of index names to their corresponding results parsed from the `jsonResultsLiteral` literal represented by how you defined them in the `Indexable` method `getIndexDescription`

The `resultRecordMap` will never be null: if there were no results for any of the indexes you've defined, the corresponding values for `ArrayList<JSONObject>` will be of size zero. 

Here we verify that `httpResponseCode` returned the status code OK, then iterate over the `ArrayList` of `JSONObject` instances corresponding to our `MovieIndex`. If present, these are added to the `newResults ArrayList` which will be passed to the handler via a `Message` object. `Message.obtain(...)` is used to obtain a message from our handler, and an overloaded signature of this method is used to set the `msg.what` (to indicate no new results or zero new results depending on the size of the `newResults ArrayList`) and the `msg.obj` as the `newResults ArrayList`. If you wanted to do further post-processing, since this callback is triggered off the UI-thread, you can do any other background operations before pushing the search results to the UI thread via the handler.

Finally, by overriding the handler's `public void handleMessage(Message msg)` superclass method we can push the results to the UI thread, since this handler is created when the `SearchResultsAdapter` is initialized in the `protected void onCreate(Bundle savedInstanceState)` method of `MyActivity`. Overriding this method to do this looks like:

```
	private static class SearchResultsUiHandler extends Handler implements SearchResultsListener {
	
		... 
		
        @Override
        public void handleMessage(Message msg) {

            SearchResultsAdapter searchResultAdapter = mSearchResultsAdapterWeakReference
                    .get();

            if (searchResultAdapter != null) {
                switch (msg.what) {
                    case MESSAGE_WHAT_PUBLISH_NEW_RESULTS:
                        ArrayList<MovieSearchResult> newResults = null;
                        try {
                            newResults = (ArrayList<MovieSearchResult>) msg.obj;
                        } catch (ClassCastException oops) {
                        }

                        if (newResults != null) {
                            searchResultAdapter
                                    .updateDisplayedSearchResults(newResults);
                        }
                        return;
                    case MESSAGE_WHAT_PUBLISH_NO_NEW_RESULTS:
                        searchResultAdapter.clearDisplayedSearchResults();
                        return;
                }
            }
        }
		
		...
		
	}
```

The two methods called on the `searchResultAdapter` (`updateDisplayedSearchResults(...)` and 'clearDisplayedSearchResults()') should be self-explanatory, but you can view how they are implemented in the source code.

###Getting Index and Engine Information

The other asynchronous callback that can be implemented is the `StateResponseListener`. For this tutorial, a new nested class is defined inside of `MyActivity` declared as `private static class SRCH2ControlListener implements StateResponseListener`. By overriding its methods you can get status results of operations performed on indexes, specific records when requested by their primary key, and know when the SRCH2Engine is ready to start searching. For this class, we'll keep a reference to our `MyActivity` to display toasts for each of the method callbacks. As long as the method `SRCH2Engine.stop(...)` is called in the `protected void onPause()` method of `MyActivity`, it is safe to keep a reference to `MyActivity` in this class: however, you could chose to keep a `WeakReference<MyActivity>` (such as with the `SearchResultListener`) if you chose not (or forget) to call `SRCH2Engine.stop(...)` from `onPause()` or you could also incorporate an event bus system like [Otto](http://square.github.io/otto/) or [EventBus](https://github.com/greenrobot/EventBus) if you have more complicated application architecture. For the purposes of this tutorial, we'll keep it simple. 

```
public class MyActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {

	...
	
    private static class SRCH2StateResponseListener implements
            StateResponseListener {
			
		private MyActivity mActivity;
	
        public SRCH2StateResponseListener(MyActivity myActivity) {
            mActivity = myActivity;
        }
	}
}
```

In detail, here are the methods:

```
public class MyActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {

	...
		
    private static class SRCH2StateResponseListener implements
            StateResponseListener {
	
		...
	
        @Override
        public void onInsertRequestComplete(final String indexName,
                                            final InsertResponse response) {
            Log.d(TAG, "Insert for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }
		
		@Override
        public void onUpdateRequestComplete(final String indexName,
                                            final UpdateResponse response) {
            Log.d(TAG, "Update for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }
		
	    @Override
        public void onDeleteRequestComplete(final String indexName,
                                            final DeleteResponse response) {
            Log.d(TAG, "Delete for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }
	}
}
```

These three methods will be triggered whenever its corresponding action is complete: note that like the `SearchResultsListener` these callbacks occur off the of the UI thread so in order to display the toasts, the method `runOnUiThread(...)` is used to push the returned information to the UI thread.

For these callbacks, the first parameter indicates which `Indexable` the following parameter is referring too, and the second parameter is a `response` which is a subclass of `RestfulResponse`. These classes wrap the raw JSON response as returned by the SRCH2 search server for each control-type task so that the data contained can be accessed from Java without parsing. These three `response`s contain the raw RESTful JSON response, as well as the success and failure counts of the insert, update or delete task performed. For example, after the `MovieIndex`'s method `insert(getAFewRecordsToInsert())` is called and the SRCH2 search server finishes inserting the three records, the `SRCH2Engine` will parse the restful JSON response and trigger this callback which will contain a `successCount` of three. It is useful to note that each subclass of `RestfulResponse' has its `toString()` method overridden, as well as the convenience method `toToastString()` which contains line breaks for each field.  

Moving on, the method:

```
public class MyActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {
	
	...
	
    private static class SRCH2StateResponseListener implements
            StateResponseListener {
			
		...

        @Override
        public void onGetRecordByIDComplete(final String indexName,
                                            final GetRecordResponse response) {
            Log.d(TAG, "Got record for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }
	}
}
```

will pass the `response GetRecordResponse` which will contain the record requested, as the original `JSONObject` as it was defined when it was inserted. 

There is also a callback for obtaining information on the indexes themselves, which occurs when either the `info()` of an `Indexable` or `SRCH2Engine.getIndexInfo(String indexName)` is called which will trigger the method:

```
public class MyActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {

	...

    private static class SRCH2StateResponseListener implements
            StateResponseListener {
			
		...

        @Override
        public void onInfoRequestComplete(final String indexName,
                                          final InfoResponse response) {
            Log.d(TAG, "Info for index: " + indexName + ". Printing info:\n"
                    + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }
	}
}
```

to be called. The `InfoResponse` object contains the fields that describe the state of an index:

```
	public boolean isValidInfoResponse();
	public int getNumberOfSearchRequests();
	public int getNumberOfWriteRequests();
	public int getNumberOfDocumentsInTheIndex();
	public String getLastMergeTime();
```

The method `boolean isValidInfoResponse()` will be return **true** if the JSON response from the SRCH2 search server was able to be parsed: if it is **false** it means the index is not available. The method `int getNumberOfDocumentsInTheIndex()` can be used to determine whether records need to be inserted and can generally be used for a first pass referential integrity check. 

`InfoResponse` objects are also passed back in what is perhaps the most useful of these control callback methods, `public void onSRCH2ServiceReady(...)` which is triggered as soon as the SRCH2 search server is up and running: 

Note also that each of the RestfulResponse subclasses also contain two methods, `public int getRESTfulHTTPStatusCode()` and `public String getRESTfulResponseLiteral()`, that can be used to manually inspect the response from the SRCH2 search server.

```
public class MyActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {

	...
		
    private static class SRCH2StateResponseListener implements
            StateResponseListener {
			
		...

        @Override
        public void onSRCH2ServiceReady(
                final HashMap<String, InfoResponse> indexesToInfoResponseMap) {
            Log.d(TAG, "SRCH2 Search Service is ready for action!");
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity,
                                "SRCH2 Search Service is ready for action!",
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }
	}
}
```

In the next section we'll see the call to initialize, start and stop the SRCH2 search server, but before that let's use this callback method to determine if the `MovieIndex` needs to be created--that is, whether the initial set of records should be inserted. Since the `InfoResponse` object value of the key 'movies' in the map `indexesToInfoResponse` will contain the method `getNumberOfDocumentsInTheIndex()` we can implement the following logic in this callback:

```
public class MyActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {

	...	
		
    private static class SRCH2StateResponseListener implements
            StateResponseListener {
			
		...

        @Override
        public void onSRCH2ServiceReady(
                final HashMap<String, InfoResponse> indexesToInfoResponseMap) {
				
			...
		
			InfoResponse movieIndexInfoResponse = indexesToInfoResponseMap.get(MovieIndex.INDEX_NAME);
			if (movieIndexInfoResponse != null && movieIndexInfoResponse.isValidInfoResponse()) {
				if (movieIndexInfoResponse.getNumberOfDocumentsInTheIndex() == 0) {
					SRCH2Engine.insertIntoIndex(MovieIndex.INDEX_NAME, MovieIndex.getAFewRecordsToInsert());
				} else {
					// Records already existed in the index, maybe time to check if the index need to be updated
				}
			}
		}
	}
}
```
  
Now when the SRCH2 search server starts, it'll trigger the `SRCH2Engine` to callback this method. The first time this method gets called, our `MovieIndex` will contain no records. The `InfoResponse` `getNumberOfDocumentsInTheIndex()` will reflect this, returning a value of zero, which in cause the outer if statement to evaluate true, causing the method `SRCH2Engine.insertIntoIndex(MovieIndex.INDEX_NAME, MovieIndex.getAFewRecordsToInsert())` to execute. Shortly afterward, the method `onInsertRequestComplete(...)` will be triggered, containing an `InsertResponse` for the `movies` index: calling `getSuccessCount()` on this `InsertResponse` will return the value of three, since that is how many records we inserted. 

At this point, you could also call `MovieIndex.info()` and the `InfoResponse` returned would, instead of returning zero, return three for the method `getNumberOfDocumentsInTheIndex()`. 

Also at this point, if the application was quit from and started again, the method `onSRCH2ServiceReady(...)` would not enter the inner if statement, since `movieIndexInfoResponse.getNumberOfDocumentsInTheIndex()` would return three. This might be the time to do a referential integrity check and update the index...

###Putting it All Together

To recap, so far we have created a `MovieIndex` as our implementation of `Indexable` representing a index with data about movies, defining its schema and creating a couple of records to insert; we have implemented the asynchronous callback for getting search results, `SearchResultsListener`, and integrated it with a `BaseAdapter` and `Handler` to update a list-view on the UI thread showing the results of a search performed in the background; we have also implemented the asynchronous callback for index and engine-server state information, the `StateResponseListener`, and integrated it in our `MyActivity` to log and display a `Toast` showing the results of each of its method callbacks, which in turn are called when the corresponding action is performed by the `SRCH2Engine`. 

If you've imported the tutorial project or copy and pasted the code, `MyActivity` should implement the `InstantSearchEditText.SearchInputEnteredObserver` which is a subclass of `EditText` to capture character-by-character input: feel free to reuse this code in your own projects.

We are now ready to initialize the `SRCH2Engine` in our `MyActivity` class. Initializing the `SRCH2Engine` is simple:

1. From `onCreate(...)` or wherever a new instance of your application is created, call `SRCH2Engine.initialize(...)` passing in each `Indexable`
2. Register the `SearchResultsListener` and `StateResponseListener`, you may reset it for instance if you are inserting from a `Service`, but in this example we'll also make these calls from `MyActivity`'s `onCreate(...)`
3. Start and stop the `SRCH2Engine` by calling `SRCH2Engine.start(Context context)` and `SRCH2Engine.stop(Context context)` where needed. For an activity requiring search, the ideal and *recommended* place is to call these from `onResume()' and `onPause()` respectively

Make these calls in `MyActivity` as follows:

```
public class MyActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {

	... 
	
	private SearchResultsAdapter mSearchResultsAdapter;
    private MovieIndex mMovieIndex;

	...

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
	
		...
		mSearchResultsAdapter = new SearchResultsAdapter(this);
		...
		
        setupSRCH2Engine();
    }

    private void setupSRCH2Engine() {
        mMovieIndex = new MovieIndex();
        SRCH2Engine.initialize(mMovieIndex);
	
        SRCH2Engine.setSearchResultsListener(mSearchResultsAdapter
                .getSearchResultsListener());

        mSRCH2StateResponseListener = new SRCH2StateResponseListener(this);
        SRCH2Engine.setStateResponseListener(mSRCH2StateResponseListener);
    }

    @Override
    protected void onResume() {
        super.onResume();
        SRCH2Engine.onStart(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        SRCH2Engine.onStop(this);
    }
}
```

Passing in each `Indexable` in `SRCH2Engine.initialize(...)` causes the `SRCH2Engine` to create and pass the necessary configuration file to the SRCH2 search server. Then the two `SRCH2Engine` callbacks are registered. Finally, the two calls `start(Context context)` and `stop(Context context)` cause the `SRCH2Engine` to start and stop the SRCH2 search server: since this server is hosted by a remote service, the context is needed; a reference to this context is not held. It is **imperative** that for every call to `start(...)` the complementary call to `stop(...)` is called in order to let the SRCH2 search server come to a stop and not take up the device's resources. 

`MyActivity` should now look something like this: 

```
public class MyActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {

    private static final String TAG = MyActivity.class.getSimpleName();

    private MovieIndex mMovieIndex;

    private ListView mSearchResultsListView;
    private SearchResultsAdapter mSearchResultsAdapter;
    private SRCH2StateResponseListener mSRCH2StateResponseListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_my);
        mSearchResultsListView = (ListView) findViewById(R.id.lv_search_results);
        mSearchResultsAdapter = new SearchResultsAdapter(this);
        mSearchResultsListView.setAdapter(mSearchResultsAdapter);

        setupSRCH2Engine();
    }

    private void setupSRCH2Engine() {
        SRCH2Engine.setSearchResultsListener(mSearchResultsAdapter
                .getSearchResultsListener());

        mSRCH2StateResponseListener = new SRCH2StateResponseListener(this);
        SRCH2Engine.setStateResponseListener(mSRCH2StateResponseListener);

        mMovieIndex = new MovieIndex();
        SRCH2Engine.initialize(mMovieIndex);
    }

    @Override
    protected void onResume() {
        super.onResume();
        SRCH2Engine.onStart(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        SRCH2Engine.onStop(this);
    }

    private static class SRCH2StateResponseListener implements
            StateResponseListener {

        private MyActivity mActivity;

        public SRCH2StateResponseListener(MyActivity myActivity) {
            mActivity = myActivity;
        }

        @Override
        public void onInsertRequestComplete(final String indexName,
                                            final InsertResponse response) {
            Log.d(TAG, "Insert for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onUpdateRequestComplete(final String indexName,
                                            final UpdateResponse response) {
            Log.d(TAG, "Update for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onDeleteRequestComplete(final String indexName,
                                            final DeleteResponse response) {
            Log.d(TAG, "Delete for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onGetRecordByIDComplete(final String indexName,
                                            final GetRecordResponse response) {
            Log.d(TAG, "Got record for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onInfoRequestComplete(final String indexName,
                                          final InfoResponse response) {
            Log.d(TAG, "Info for index: " + indexName + ". Printing info:\n"
                    + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onSRCH2ServiceReady(
                final HashMap<String, InfoResponse> indexesToInfoResponseMap) {
            Log.d(TAG, "SRCH2 Search Service is ready for action!");
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity,
                                "SRCH2 Search Service is ready for action!",
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
            InfoResponse movieIndexInfoResponse = indexesToInfoResponseMap
                    .get(MovieIndex.INDEX_NAME);
            if (movieIndexInfoResponse != null
                    && movieIndexInfoResponse.isValidInfoResponse()) {
                if (movieIndexInfoResponse.getNumberOfDocumentsInTheIndex() == 0) {
                    SRCH2Engine.insertIntoIndex(MovieIndex.INDEX_NAME,
                            MovieIndex.getAFewRecordsToInsert());
                }
            }
        }
    }
}
```

The last step is to connect the `InstantSearchEditText` and its interface `SearchInputEnteredObserver` to the `SRCH2Engine` through `MyActivity`. 

```
public class MyActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {

    ...
	
    @Override
    public void onNewSearchInput(String newSearchText) {
        SRCH2Engine.searchAllIndexes(newSearchText);
    }

    @Override
    public void onNewSearchInputIsBlank() {
        mSearchResultsAdapter.clearDisplayedSearchResults();
    }
	
	...
}
```

There are three ways to pass search input to the `SRCH2Engine`:

1. Through the non-static `Indexable` instance itself such as `mMovieIndex.search(newSearchInput)`
2. Through the static `SRCH2Engine` method targeting a specific index `SRCH2Engine.searchIndex(MovieIndex.INDEX_NAME, newSearchInput)`
3. Through the static `SRCH2Engine` method targeting all indexes `SRCH2Engine.searchAllIndexes(newSearchInput)`

Here we use the last method in case you'd like to add some more indexes, such as for music, contacts or anything you can think of. 

You are now ready to search! **Make sure you have a device connected** (at this time the SRCH2 Android SDK cannot be run on emulators), press 'Run' and after you see the toast saying "SRCH2 Service is Ready!" try entering "c" into the `EditText` at the top of your device's screen. You should see something like:<br><center>![Tutorial Finished - Seeing the SRCH2 Android SDK in action][tutorial-008]</center>

The results come back fast, and even if we had we inserted thousands of records, the search results would still take less than milliseconds to reach the `SearchResultsAdapter`. Because of this, it is recommended that, while developing with the SRCH2-Android-SDK, you employ all of the optimizations for displaying data in a `ListView` as quickly as possible (such as the ViewHolder pattern, lazy loading of bitmaps if you incorporate icons, et cetera). 

###Where To Go From Here

This concludes the Hello SRCH2 Tutorial for learning how to get set up with the SRCH2-Android-SDK. Play around with the project: try updating a record, deleting a record, adding more records with searchable data that is similar in its sequence of letters to appreciate the power of SRCH2's fuzzy search capability-- and this is only the beginning! If you read on in [Advanced Topics](advanced-topics.md) you'll learn how to form powerful queries (such as filtering the search results for the `MovieIndex` by interval of year), perform lightning fast geo-searches using the device's location, or how to set up the SRCH2-Android-SDK for Proguard, or how to manually interact with the running SRCH2 search server. Search on!


[tutorial-000]: ../img/000-tutorial.png "Module Settings - How to open 'Module Settings'"
[tutorial-001]: ../img/001-tutorial.png "Module Settings - How to add a new module"
[tutorial-002]: ../img/002-tutorial.png "Module Settings - Creating a new module from an .AAR file"
[tutorial-003]: ../img/003-tutorial.png "Module Settings - Importing the SRCH2-Android-SDK.aar file as the new module"
[tutorial-004]: ../img/004-tutorial.png "Module Settings - Adding the SRCH2-Android-SDK module as a dependency"
[tutorial-005]: ../img/005-tutorial.png "Module Settings - Selecting the SRCH2-Android-SDK module"
[tutorial-006]: ../img/006-tutorial.png "Module Settings - Verifying SRCH2-Android-SDK module as been added as a compiled dependency"
[tutorial-007]: ../img/007-tutorial.png "Installation Complete - Confirming gradle has included the SRCH2 Android SDK as a dependency in your project"
[tutorial-008]: ../img/008-tutorial.png "Tutorial Finished - Seeing the SRCH2 Android SDK in action"
[tutorial-009]: ../img/009-tutorial.png "Default Project Files - Project hierarchy after adding all the files"
