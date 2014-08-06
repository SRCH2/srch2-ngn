Hello. Welcome to the SRCH2 Android SDK Hello World Tutorial.

###Introduction
 
This tutorial will show you how to:

1. Incorporate the SRCH2 Android SDK into your Android Studio project
2. Create indexes and insert records
3. Enable powerful search in your app and publish the search results to the UI or main thread
 
To get started, this tutorial assumes you are familiar with Android Studio and can create, or have already created, Android Studio projects. For instructions how to set up the SRCH2 Android SDK on eclipse, see [Advanced Topics](advanced-topics.md). 
 
###Context 
 
Before we get started, here's a brief overview of what the SRCH2 Android SDK is:
 
The SRCH2 Android SDK is powered by the SRCH2 Search engine. This search engine runs as a http server that is hosted internally by the SDK in an Android remote service (don't worry, you don't have to bind to it). Once running, you can manually do RESTful CRUD operations by forming the proper URLs (see the [Advanced Topics](advanced-topics.md) for more information), but the SDK comes with an API that enables you to do these same operations and interact with the running SRCH2 RESTful server through Java method calls. The SRCH2 Android SDK contains indexable objects, representing the indexes you will create and query, on which you can call methods such as insert, update, query, delete and info. In your code, you can use these objects by extending the `com.srch2.android.http.service.Indexable` class; besides defining the indexes themselves, most of these operations can be done by calling on the appropriate method of `com.srch2.android.http.service.SRCH2Engine`.
 
In addition, to receive output from the SRCH2 server, there are two asynchronous callbacks you should implement: the one you will certainly want to implement is `com.srch2.android.http.service.SearchResultsListener`. On completion of a query, this interface will pass you a map of search results to their originating index as well as the raw JSON response as sent from the SRCH2 http server. The other callback provides a set of state information callbacks, such as the result of an /info command performed on an index providing information about that index, or upon completion of an insert command notifying the success of having inserted records.
 
Finally, also included is set of classes giving you the power to unlock all the potential of the SRCH2 search engine (see the [Advanced Topics](advanced-topics.md) section for usage and examples): instant search, fuzzy matching, categorical faceting, boolean operators, per record and per record data fields ranking, local parametrization and filtering of the specific record data fields, and much much more. In fact--be sure to check out how to perform GeoSearches where you can include the longitude and latitude of the device so they are combined with the keywords a user might enter, to form the search query. But first to set up the SRCH2 Android SDK...
 
###Installing the SDK
 
The first step is to obtain the SRCH2 Android SDK ``.aar`` file. An `.aar` file extension is an expansion of the JAR file format to include Android native resources, which the SRCH2 Android SDK requires to get the SRCH2 search engine running. For more information on the `.aar` file format, you can refer to [AAR Format](http://tools.android.com/tech-docs/new-build-system/aar-format) at the Android Tools Project Site.

You can download the `SRCH2-Android-SDK.aar` file from [here](./tutorial.html).
 
*Quick Steps*:

1. Once you've downloaded the `SRCH2-Android-SDK.aar` file, open up your Android Studio project.
2. Goto File->New Module and select 'Import JAR / AAR file' from the list of options.
3. Select the `SRCH2-Android-SDK.aar` file from its download location.
4. Click 'Finish'.
5. Under ProjectName->app, open build.gradle and add `compile project(':SRCH2-Android-SDK')` to the dependencies node.
 
*Interactive Steps*:

1. Once you've downloaded the `SRCH2-Android-SDK.aar` file, open up Android Studio.
2. Create a new project called `HelloSRCH2SearchDemo`, with the usual minimum (at least 8), build and target API versions, declared with the package name `com.srch2.android.demo.hellosrch2` in this example.
3. Create at least one activity, by default set as the launcher activity, named `MyActivity` in this example.
4. You should now have a new project open in Android Studio.
5. To incorporate the SRCH2 Android SDK, you will need to add the `SRCH2-Android-SDK.aar` file to the project as a new module and edit the build.grandle file to enable gradle to build it when compiling your project.
6. To do this, open up your project's 'Module Settings' by either right clicking on your project and selecting 'Open Module Settings', or after clicking into your project panel pressing F4 (the default keyboard short-cut).
7. Move the cursor to the upper-left hand of the dialogue box, and click the green addition symbol. This will add a new module to your project. This will look something like:<br>![Module Settings - How to add a new module][tutorial-001]
8. From the list of options, select 'Import .JAR or .AAR Package' and click Next.<br>![Module Settings - Creating a new module from an .AAR file][tutorial-002]
9. Select `SRCH2-Android-SDK.aar` from its download location, you can leave value of the field 'Subproject Name' the same, and click Finish.<br>![Module Settings - Importing the SRCH2-Android-SDK.aar file as the new module][tutorial-003]
10. Now that you've added the `SRCH2-Android-SDK.aar` as a new module in your project, it's time to add it as a dependency to your app.
11. From the 'Module Settings' dialog, click on your app from the 'Modules' column (on the left-hand side, SRCH2-Android-SDK.aar should also be in the list). 
12. Select the 'Dependencies' tab, move the cursor to the right side of the dialog. Click the green addition symbol, and from the drop down list select 'Module dependency'. This should look something like this:<br>![Module Settings - Adding the SRCH2-Android-SDK module as a dependency][tutorial-004]
13. Select `SRCH2-Android-SDK` from the list of modules. Click OK.<br>![Module Settings - Selecting the SRCH2-Android-SDK module][tutorial-005]
14. Now can click Apply and OK, or just OK to finish this process and close 'Module Settings'.<br>![Module Settings - Verifying SRCH2-Android-SDK module as been added as a compiled dependency][tutorial-006]
15. For those curious, this process added the `SRCH2-Android-SDK.aar` as a new module, then altered the build.gradle file in your app's subdirectory to include the `SRCH2-Android-SDK` module to be compiled as a dependency; which you can verify by navigating to this file, and it will look something like:
11. Now you are ready to start accessing the API.
 
###Overview of the API

Before we dive into the code, here's a brief outline of the classes and methods you'll be using:

```
SRCH2Engine 
	initialize(SRCH2Configuration configuration, StateResponseListener StateResponseListener, SearchResultsListener searchResultsListener);
	onStart(Context context);
	onStop(Context context);
	searchAllIndexes(String searchInput);
	insertIntoIndex(String indexName, JSONObject recordToUpdate / JSONArray recordsToUpdate); (also, update/delete/retrieveSpecificRecord)
	...
Indexable
	getIndexDescription();
	search(String searchInput);
	insert(JSONObject recordToUpdate / JSONArray recordsToUpdate); 
	...
SearchResultsListener
	onNewSearchResults(int httpResponseCode, String jsonResultsLiteral, HashMap<String, ArrayList<JSONObject>> resultRecordMap);
StateResponseListener
	onSRCH2ServiceReady(final HashMap<String, InfoResponse> indexesToInfoResponseMap);
	onInsertRequestComplete(final String targetIndexName, final InsertResponse response); 
	...
```

<insert architecture diagram here>

###Creating an Index

The first step to searching an index is creating an index. In this tutorial, we'll create an index to search on the title and genre of some movies. 

Create the index for movies by creating a new class that extends `com.srch2.android.http.service.Indexable`. In this example this class will be named `MovieIndex`. An index is defined by a schema, which describes the fields in the index such as their type and default value; the fields of the schema are comparable to the columns of an SQLITE table. To define the schema for the `MovieIndex`, override the `Indexable` abstract method `public IndexDescription getIndexDescription()` if you are not prompted to do so. Since we'll use the name of the index and the names of the fields to prepare the results of a search, declare and define the following constant field members for this class:

```
public static final String INDEX_NAME = "movies";
public static final String INDEX_FIELD_PRIMARY_KEY = "id";
public static final String INDEX_FIELD_TITLE = "title";
public static final String INDEX_FIELD_YEAR = "year";
public static final String INDEX_FIELD_GENRE = "genre";
```

And then the method `public IndexDescription getIndexDescription()` can be implemented as follows:

```
@Override
public IndexDescription getIndexDescription() {
    Field primaryKey = Field.getRefiningField(INDEX_FIELD_PRIMARY_KEY, Field.Type.INTEGER);
    Field title = Field.getSearchableField(INDEX_FIELD_TITLE, 3);
    Field year = Field.getRefiningField(INDEX_FIELD_YEAR, Type.INTEGER);
    Field genre = Field.getSearchableField(INDEX_FIELD_GENRE);
    return new IndexDescription(INDEX_NAME, primaryKey, title, year, genre);
}
```

Fields can be obtained by calling the static factory methods of the class `com.srch2.android.http.service.Field`. A searchable field has textual data that will be searched during a query; a refining field is for storing data values that can be used for query filtering and post-processing. Both refining and searchable field data can be retrieved from the raw search results returned by the SRCH2 server. When invoking these factory methods for a searchable field, a third parameter can be passed which will set the field's relative ranking (or relevance) to other searchable fields--the default value is one. Here, the relevance of the `title Field` is set to be three times more important than the `genre Field`. 

An index _always_ requires one field to be defined as the primary key: each record should have a unique value for its primary key. It can be searchable, and/or refining. The primary key is the handle by which you can verify records were inserted correctly, deleted and/or retrieved by later.

When returning the `new IndexDescription(...)` the first parameter is _always_ the name of index, the second parameter _should always_ be the primary key, and each subsequent parameter are the rest of the fields you define, in no particular order. 

The `SRCH2Engine` will use this `IndexDescription` to automatically create the configuration for this index. While we're in this class, let's create a method for inserting some records we can search on. Add a method `public static JSONArray getAFewRecordsToInsert()` implemented as follows:

```
public static JSONArray getAFewRecordsToInsert() {
	JSONArray jsonRecordsToInsert = new JSONArray();
	try {
		JSONObject record = new JSONObject();
		record.put(INDEX_FIELD_PRIMARY_KEY, "1");
        record.put(INDEX_FIELD_TITLE, "The Good, the Bad And the Ugly");
        record.put(INDEX_FIELD_YEAR, 1966);
        record.put(INDEX_FIELD_GENRE, "Western Adventure");
        jsonRecordsToInsert.put(record);
		
		record = new JSONObject();
        record.put(INDEX_FIELD_PRIMARY_KEY, "2");
        record.put(INDEX_FIELD_TITLE, "Citizen Kane");
        record.put(INDEX_FIELD_YEAR, 1941);
        record.put(INDEX_FIELD_GENRE, "Mystery Drama");
        jsonRecordsToInsert.put(record);
		
		record = new JSONObject();
        record.put(INDEX_FIELD_PRIMARY_KEY, "3");
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

The `SRCH2Engine` accepts a `JSONObject` or a `JSONArray` of `JSONObject`s when inserting or updating records. Insertions (and updates) are invoked by calling `insert(...)` on your `Indexable` implementation, or by specifying the index name and calling `SRCH2Engine.insertIntoIndex(String indexName, ...)`. However, before actually calling this method it is time to set up the two asynchronous call-backs so that you can get index information and search results from the SRCH2 server.

###Getting Search Results

The SRCH2 server passes information back to the `SRCH2Engine` through two asynchronous callbacks: `com.srch2.android.http.service.SearchResultsListener` and `com.srch2.android.http.service.StateResponseListener`. The first passes back search results from queries performed while the second passes back information relating to the indexes such as whether records were inserted correctly, if a specific record is requested by id, or if an information request on a specific index is performed. These two interfaces *should be* implemented by you--in particular, the `SearchResultsListener` is the way in which you will receive search results. 

For the demonstration of this tutorial it is assumed you are familiar with how a `android.widget.BaseAdapter' works to populate a `ListView`. Familiarity with how to communicate with the UI thread via a `Handler` is also helpful, but you can read more about this on the Android developer website--(Communicating with the UI Thread)[https://developer.android.com/training/multiple-threads/communicate-ui.html]. 

These two callbacks will trigger off the main or UI thread, so in order to update the UI of your application, the search results must be passed to the UI thread. In this tutorial we will use subclass of the `android.os.Handler` class to implement the `SearchResultsListener` interface; however, other means of propagating the search results to the UI thread can be implemented, such as `runOnUiThread()` can be used. 

In the source code of this tutorial there is the class `SearchResultsAdapter` which extends `BaseAdapter`; here we will cover how to connect this adapter to a handler implementing `SearchResultsListener`. Create the class `SearchResultsAdapter` and copy and paste the `BaseAdapter`'s implementation, skipping the nested `Handler` class. At the end of this class declare the following nested subclass of `Handler`:

```
	private static class SearchResultsUiHandler extends Handler implements SearchResultsListener {
	}
```

In this class, declare a `WeakReference<T>` of type `SearchResultsAdapter` and two constant `int` fields that will be used as the message keys for this handler:

```
	private static class SearchResultsUiHandler extends Handler implements SearchResultsListener {
	
		private static final int MESSAGE_WHAT_PUBLISH_NEW_RESULTS = 001;
		private static final int MESSAGE_WHAT_PUBLISH_NO_NEW_RESULTS = 002;
		
		private final WeakReference<SearchResultsAdapter> mSearchResultsAdapterWeakReference;
	}
```

The `WeakReference<SearchResultsAdapter>` is necessary to prevent leaking the context (you can read more about why this is necessary [here](http://www.androiddesignpatterns.com/2013/01/inner-class-handler-memory-leak.html)) and can be defined in the constructor such as:

```
	private static class SearchResultsUiHandler extends Handler implements SearchResultsListener {
	
		... 
		
		public SearchResultsUiHandler(SearchResultsAdapter searchResultsAdapter) {
			mSearchResultsAdapterWeakReference = new WeakReference<SearchResultsAdapter>(searchResultsAdapter);
		}
	}
```

Since this handler implements `SearchResultsListener` override the interface method `public void onNewSearchResults(int httpResponseCode, String jsonResponse, HashMap<String, ArrayList<JSONObject>>)' in the following way:

```
	private static class SearchResultsUiHandler extends Handler implements SearchResultsListener {
	
		... 
		
		@Override
		public void onNewSearchResults(int httpResponseCode, String jsonResponse, HashMap<String, ArrayList<JSONObject>> resultRecordMap) {
			if (httpResponseCode == HttpURLConnection.HTTP_OK) {
				ArrayList<MovieSearchResult> newResults = new ArrayList<MovieSearchResult>();

				ArrayList<JSONObject> movieResults = resultRecordMap.get(MovieIndex.INDEX_NAME);
				if (movieResults != null && movieResults.size() > 0) {
					for (JSONObject jsonObject : movieResults) {
						SearchResult searchResult = null;
						try {
							searchResult = new MovieSearchResult(jsonObject.getString(MovieIndex.INDEX_FIELD_TITLE), jsonObject.getString(MovieIndex.INDEX_FIELD_GENRE), jsonObject.getInt(MovieIndex.INDEX_FIELD_YEAR));
						} catch (JSONException oops) {
							continue;
						}
						
						if (searchResult != null) {
							newResults.add(searchResult);
						}
					}
				}
				sendMessage(Message.obtain(this, newResults.size() > 0 ? MESSAGE_WHAT_PUBLISH_NEW_RESULTS : MESSAGE_WHAT_PUBLISH_NO_NEW_RESULTS, newResults));	
			}
		}
	}
```

Any time one of the query methods of the API is called (such as `SRCH2Engine.searchIndex(String indexName, String searchInput)` or `MovieIndex.search(String searchInput)` this method will be triggered upon results returned from the SRCH2 server. Its parameters are:

1. `int httpResponseCode' which indicates how the restful action was handled
2. `String jsonResponse` which is the raw JSON literal as returned by the SRCH2 server containing the full set of search result data
3. `HashMap<String, ArrayList<JSONObject>> resultRecordMap` which is a mapping of index names to their corresponding results parsed from the `jsonResponse` literal represented by how you defined them in the `Indexable` method `getIndexDescription`

The `resultRecordMap` will never be null: if there were no results for any of the indexes you've defined, the corresponding values for `ArrayList<JSONObject>` will be of size zero. 

Here we verify that `httpResponseCode` returned the status code OK, then iterate over the `ArrayList` of `JSONObject`s corresponding to our `MovieIndex`. If present, these are added to the `newResults ArrayList` which will be passed to the handler via a `Message` object. `Message.obtain(...)` is used to obtain a message from our handler, and an overloaded signature of this method is used to set the `msg.what` (to indicate no new results or zero new results depending on the size of the `newResults ArrayList`) and the `msg.obj` as the `newResults ArrayList`. If you wanted to do further post-processing, since this callback is triggered off the UI-thread, you can do any other background operations before pushing the search results to the UI thread via the handler.

Finally, by overriding the handler's `public void handleMessage(Message msg)` superclass method we can push the results to the UI thread, since this handler is created when the `SearchResultsAdapter` is initialized in the `protected void onCreate(Bundle savedInstanceState)` method of `MyActivity`. Overriding this method to do this looks like:

```
	private static class SearchResultsUiHandler extends Handler implements SearchResultsListener {
	
		... 
		
		@Override
		public void handleMessage(Message msg) {
			SearchResultsAdapter searchResultAdapter = mSearchResultsAdapterWeakReference.get();
			
			if (searchResultAdapter != null) {
				switch (msg.what){
					case MESSAGE_WHAT_PUBLISH_NEW_RESULTS:
						ArrayList<SearchResult> newResults = null;
						try {
							newResults = (ArrayList<SearchResult>) msg.obj;
						} catch (ClassCastException oops) {
							// We know what we're doing
						}
						
						if (newResults != null) {
							searchResultAdapter.updateDisplayedSearchResults(newResults);
						}
						return;
					case MESSAGE_WHAT_PUBLISH_NO_NEW_RESULTS:
						searchResultAdapter.clearDisplayedSearchResults();
						return;
				}
			}
		}
	}
```

The two methods called on the `searchResultAdapter` (`updateDisplayedSearchResults(...)` and 'clearDisplayedSearchResults()') should be self-explanatory, but you can view how they are implemented in the source code.

###Getting Index and Engine Information

The other asynchronous callback that can be implemented is the `StateResponseListener`. For this tutorial, a new nested class is defined inside of `MyActivity` declared as `private static class SRCH2ControlListener implements StateResponseListener`. By overriding its methods you can get status results of operations performed on indexes, specific records when requested by their primary key, and know when the SRCH2Engine is ready to start searching. For this class, we'll keep a reference to our `MyActivity` to display toasts for each of the method callbacks. As long as the method `SRCH2Engine.stop(...)` is called in the `protected void onPause()` method of `MyActivity`, it is safe to keep a reference to `MyActivity` in this class: however, you could chose to keep a `WeakReference<MyActivity>` (such as with the `SearchResultListener`) if you chose not (or forget) to call `SRCH2Engine.stop(...)` from `onPause()` or you could also incorporate an event bus system like [Otto](http://square.github.io/otto/) or [EventBus](https://github.com/greenrobot/EventBus) if you have more complicated application architecture. For the purposes of this tutorial, we'll keep it simple. 

```
private static class SRCH2StateResponseListener implements StateResponseListener {

	private MyActivity mActivity;
	
	public SRCH2StateResponseListener(MyActivity myActivity) {
		mActivity = myActivity;
	}
}
```

In detail, here are the methods:

```
private static class SRCH2StateResponseListener implements StateResponseListener {
	
	...
	
	@Override
	public void onInsertRequestComplete(final String indexName, final InsertResponse response) {
		Log.d(TAG, "Insert for index: " + indexName + " complete. Printing info:\n" + response.toString());
		if (mActivity != null) {
			mActivity.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
						}
					});
		}
	}
	
	@Override
	public void onDeleteRequestComplete(final String indexName, final DeleteResponse response) {
		Log.d(TAG, "Delete for index: " + indexName + " complete. Printing info:\n" + response.toString());
		if (mActivity != null) {
			mActivity.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
						}
					});
		}
	}
	
	@Override
	public void onUpdateRequestComplete(final String indexName, final UpdateResponse response) {
		Log.d(TAG, "Update for index: " + indexName + " complete. Printing info:\n" + response.toString());
		if (mActivity != null) {
			mActivity.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
						}
					});
		}
	}
}
```

These three methods will be triggered whenever its corresponding action is complete: not that like the `SearchResultsListener` these callbacks occur off the of the UI thread so in order to display the toasts, the method `runOnUiThread(...)` is used to push the returned information to the UI thread.

For these callbacks, the first parameter indicates which `Indexable` the following parameter is referring too, and the second parameter is a `response` which is a subclass of `RestfulResponse`. These classes wrap the raw JSON response as returned by the SRCH2 server for each control-type task so that the data contained can be accessed from Java without parsing. These three `response`s contain the raw RESTful JSON response, as well as the success and failure counts of the insert, update or delete task performed. For example, after the `MovieIndex`'s method `insert(getAFewRecordsToInsert())` is called and the SRCH2 server finishes inserting the three records, the `SRCH2Engine` will parse the restful JSON response and trigger this callback which will contain a `successCount` of three. It is useful to note that each subclass of `RestfulResponse' has its `toString()` method overridden, as well as the convenience method `toToastString()` which contains line breaks for each field.  

Moving on, the method:

```
private static class SRCH2StateResponseListener implements StateResponseListener {

	...

	@Override
	public void onGetRecordByIDComplete(final String indexName, final GetRecordResponse response) {
		Log.d(TAG, "Got record for index: " + indexName + ". Printing info:\n" + response.toString());
		if (mActivity != null) {
			mActivity.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
						}
					});
		}
	}
}
```

will pass the `response GetRecordResponse` which will contain the record requested, as the original `JSONObject` as it was defined when it was inserted. 

There is also a callback for obtaining information on the indexes themselves, which occurs when either the `info()` of an `Indexable` or `SRCH2Engine.getIndexInfo(String indexName)` is called which will trigger the method:

```
private static class SRCH2StateResponseListener implements StateResponseListener {

	...

	@Override
	void onInfoRequestComplete(final String targetIndexName, final InfoResponse response) {
		Log.d(TAG, "Got info for index: " + indexName + ". Printing info:\n" + response.toString());
		if (mActivity != null) {
			mActivity.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
						}
					});
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

The method `boolean isValidInfoResponse()` will be return _true_ if the JSON response from the SRCH2 server was able to be parsed: if it is _false_ it means the index is not available. The method `int getNumberOfDocumentsInTheIndex()` can be used to determine whether records need to be inserted and can generally be used for a first pass referential integrity check. 

`InfoResponse` objects are also passed back in what is perhaps the most useful of these control callback methods: 

```
private static class SRCH2StateResponseListener implements StateResponseListener {

	...

	@Override
	public void onSRCH2ServiceReady(final HashMap<String, InfoResponse> indexesToInfoResponseMap)  {
		Log.d(TAG, "SRCH2 Search Service is ready for action!");
		if (mActivity != null) {
			mActivity.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							Toast.makeText(mActivity, "SRCH2 Search Service is ready for action!", Toast.LENGTH_SHORT).show();
						}
					});
		}
	}
}
```

which is triggered as soon as the SRCH2 server is up and running. In the next section we'll see the call to initialize, start and stop the SRCH2 server, but before that let's use this callback to determine if the `MovieIndex` needs to be created--that is, whether the initial set of records should be inserted. Since the `InfoResponse` object value of the key 'movies' in the map `indexesToInfoResponse` will contain the method `getNumberOfDocumentsInTheIndex()` we can implement the following logic in this callback:

```
private static class SRCH2StateResponseListener implements StateResponseListener {

	...

	@Override
	public void onSRCH2ServiceReady(HashMap<String, InfoResponse> indexesToInfoResponseMap)  {
		
		...
		
		InfoResponse movieIndexInfoResponse = indexesToInfoResponseMap.get(MovieIndex.INDEX_NAME);
		if (movieIndexInfoResponse != null && movieIndexInfoResponse.isValidInfoResponse()) {
			if (movieIndexInfoResponse.getNumberOfDocumentsInTheIndex() == 0) {
				SRCH2Engine.insertIntoIndex(MovieIndex.INDEX_NAME, MovieIndex.getAFewRecordsToInsert());
			}
		}
	}
}
```
  
Now when the SRCH2 server starts, it'll trigger the `SRCH2Engine` to callback this method. The first time this happens, there will be an index created for the `MovieIndex` that will have no records. The `InfoResponse` `getNumberOfDocumentsInTheIndex()` will reflect this, returning a value of zero, which in turn will tell the `SRCH2Engine` to insert the few records from the `MovieIndex`. Very shortly thereafter, the method `public void onInsertRequestComplete(String indexName, InsertResponse response)` will be triggered, containing an `InsertResponse` for the `movies` index: calling `getSuccessCount()` on this `InsertResponse` will return the value of three, since that is how many records we inserted. You could also call `MovieIndex.info()` and calling `getNumberOfDocumentsInTheIndex()` would, instead of returning zero, return three. If the application was quit from, and then started again, the method `public void onSRCH2ServiceReady(...)` would not enter the inner if statement, since `movieIndexInfoResponse.getNumberOfDocumentsInTheIndex()` would return three. This might be the time to do a referential integrity check, and update or insert more records... 

###Putting it All Together

To recap, so far we have created a `MovieIndex` as our implementation of `Indexable` representing a index with data about movies, defining its schema and creating a couple of records to insert; we have implemented the asynchronous callback for getting search results, `SearchResultsListener`, and integrated it with a `BaseAdapter` and `Handler` to update a list-view on the UI thread showing the results of a search performed in the background; we have also implemented the asynchronous callback for index and engine-server state information, the `StateResponseListener`, and integrated it in our `MyActivity` to log and display a `Toast` showing the results of each of its method callbacks, which in turn are called when the corresponding action is performed by the `SRCH2Engine`. If you've used imported the tutorial project or copy and pasted the code, `MyActivity` should implement the `InstantSearchEditText.SearchInputEnteredObserver` which is a subclass of `EditText` to capture character-by-character input: feel free to reuse this code in your own projects.

We are now ready to initialize the `SRCH2Engine` in our `MyActivity` class. Initializing the `SRCH2Engine` is simple:

1. From `onCreate(...)` or wherever a new instance of your application is created, call `SRCH2Engine.initialize(...)` passing in each `Indexable`
2. Register the `SearchResultsListener` and `StateResponseListener`, you may reset it for instance if you are inserting from a `Service`, but in this example we'll also make these calls from `MyActivity`'s `onCreate(...)`
3. Start and stop the `SRCH2Engine` by calling `SRCH2Engine.start(Context context)` and `SRCH2Engine.stop(Context context)` where needed. For an activity requiring search, the ideal and *recommended* place is to call these from `onResume()' and `onPause()` respectively

Passing in each `Indexable` in `SRCH2Engine.initialize(...)` causes the `SRCH2Engine` to create and pass the necessary configuration file to the SRCH2 server. Registering the callbacks should be self-explanatory. Finally, the two calls `start(Context context)` and `stop(Context context)` cause the `SRCH2Engine` to start and stop the SRCH2 server: since this server is hosted by a remote service, the context is needed; a reference to this context is not held. It is _imperative_ that for every call to `start(...)` the complementary call to `stop(...)` is called in order to let the SRCH2 server come to a stop and not take up the device's resources. 

`MyActivity` should now look something like this: 

```
public class MyActivity extends Activity implements InstantSearchEditText.SearchInputEnteredObserver {

    private static final String TAG = MyActivity.class.getSimpleName();

    private MovieIndex mMovieIndex;

    private ListView mSearchResultsListView;
    private SearchResultsAdapter mSearchResultsAdapter;
    private SRCH2StateResponseListener mSRCH2StateResponseListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);
        mSearchResultsListView = (ListView) findViewById(R.id.lv_search_results);
        mSearchResultsAdapter = new SearchResultsAdapter(this);
        mSearchResultsListView.setAdapter(mSearchResultsAdapter);

        setupSRCH2Engine();
    }

    private void setupSRCH2Engine() {
        SRCH2Engine.setSearchResultsListener(mSearchResultsAdapter.getSearchResultsListener());

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

    private static class SRCH2StateResponseListener implements StateResponseListener {

        private MyActivity mActivity;

        public SRCH2StateResponseListener(MyActivity myActivity) {
            mActivity = myActivity;
        }

        @Override
        public void onInfoRequestComplete(final String indexName, final InfoResponse response) {
            Log.d(TAG, "Info for index: " + indexName + ". Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onInsertRequestComplete(final String indexName, final InsertResponse response) {
            Log.d(TAG, "Insert for index: " + indexName + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onUpdateRequestComplete(final String indexName, final UpdateResponse response) {
            Log.d(TAG, "Update for index: " + indexName + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onSRCH2ServiceReady(final HashMap<String, InfoResponse> indexesToInfoResponseMap) {
            Log.d(TAG, "SRCH2 Search Service is ready for action!");
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, "SRCH2 Search Service is ready for action!", Toast.LENGTH_SHORT).show();
                    }
                });
            }
            InfoResponse movieIndexInfoResponse = indexesToInfoResponseMap.get(MovieIndex.INDEX_NAME);
            if (movieIndexInfoResponse != null && movieIndexInfoResponse.isValidInfoResponse()) {
                if (movieIndexInfoResponse.getNumberOfDocumentsInTheIndex() == 0) {
                    SRCH2Engine.insertIntoIndex(MovieIndex.INDEX_NAME, MovieIndex.getAFewRecordsToInsert());
                }
            }
        }

        @Override
        public void onDeleteRequestComplete(final String indexName, final DeleteResponse response) {
            Log.d(TAG, "Delete for index: " + indexName + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onGetRecordByIDComplete(final String indexName, final GetRecordResponse response) {
            Log.d(TAG, "Got record for index: " + indexName + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }
    }
}
```

The last step is to connect the `InstantSearchEditText` and its interface `SearchInputEnteredObserver` to the `SRCH2Engine` through `MyActivity`. 

```
public class MyActivity extends Activity implements InstantSearchEditText.SearchInputEnteredObserver {

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

You are now ready to search! Try entering "c" into the `EditText` at the top of your device's screen and see the speed with which the results are returned. Had we inserted thousands of records, the search results would still take less than milliseconds to reach the `SearchResultsAdapter`: because of this it is also recommended that, while developing with the SRCH2-Android-SDK, you employ all of the optimizations for displaying data in a `ListView` as quickly as possible (such as the ViewHolder pattern, lazy loading of bitmaps if you incorporate icons, et cetera). 

###Conclusion

This concludes the Hello SRCH2 Tutorial for learning how to get set up with the SRCH2-Android-SDK. Play around with the project: try updating a record, deleting a record, adding more records with searchable data that is similar in its sequence of letters to appreciate the power of SRCH2's fuzzy search capability-- and this is only the beginning! If you read on in [Advanced Topics](advanced-topics.md) you'll learn how to form powerful queries (such as filtering the search results for the `MovieIndex` by interval of year), perform lightning fast geo-searches using the device's location, or how to set up the SRCH2-Android-SDK for Proguard, or how to manually interact with the running SRCH2 server. Read and search on!



[tutorial-001]: ../img/001-tutorial.png "Module Settings - How to add a new module"
[tutorial-002]: ../img/002-tutorial.png "Module Settings - Creating a new module from an .AAR file"
[tutorial-003]: ../img/003-tutorial.png "Module Settings - Importing the SRCH2-Android-SDK.aar file as the new module"
[tutorial-004]: ../img/004-tutorial.png "Module Settings - Adding the SRCH2-Android-SDK module as a dependency"
[tutorial-005]: ../img/005-tutorial.png "Module Settings - Selecting the SRCH2-Android-SDK module"
[tutorial-006]: ../img/006-tutorial.png "Module Settings - Verifying SRCH2-Android-SDK module as been added as a compiled dependency"