Hello and welcome to the Hello SRCH2 Android SDK tutorial. 

###Introduction
 
This tutorial will show you how to:

1. Incorporate the SRCH2 Android SDK into your Android Studio project
2. Create indexes and insert records
3. Enable powerful search in your app and publish the search results to the UI or main thread
 
To get started, this tutorial assumes you are familiar with Android Studio and can create, or have already created, Android Studio projects. 

Before you begin the tutorial, clone the project application from our [`hello-srch2-android-sdk` Github repository](https://github.com/SRCH2/hello-srch2-android-sdk). 

###Quick Start

1. Clone the tutorial application project from our [`hello-srch2-android-sdk` Github repository](https://github.com/SRCH2/hello-srch2-android-sdk). To do this from the command line you can enter the following git command: `clone githubrepo` (**SUBSTITE WITH REAL LINK: for now navigate to *master->srch2-android-sdk>source>Hello-SRCH2-Android-SDK and use this directory for step two**)
2. Open Android Studio and from the File menu option (or the Quick Start menu) select 'Import Project' choosing the root folder of the cloned project: ![Opening the cloned Hello SRCH2 Android SDK application project][tutorial-010]
3. With a device connected (currently hardware emulation does not support the SRCH2 Android SDK), from the Run menu option select 'Run app'. 
4. Once the app has launched, enter some characters in the text input field at the top of the screen and you should instantly see search results: <br>
<center>![The SRCH2 Android SDK in action!][tutorial-011]</center>
5. That's it! You're now using the SRCH2 Android SDK to power searches in an Android project with the SRCH2 search engine. 

The rest of this tutorial is an explanation of this source code and how to install the SRCH2 Android SDK in your own projects...

###Installing the SDK
 
The Gradle build system makes it easy to install the SRCH2 Android SDK in an Android Studio project.

1. If you have an open Android project in Android Studio, to incorporate the SRCH2 Android SDK, you will need to configure the Gradle build system to retrieve the `SRCH2-Android-SDK.aar` file from the SRCH2 server Maven repository. To do this, navigate to the top-level <code>build.gradle</code> file in your project view.
2. In the `repositories` node in the `buildscript` node, add the entry:<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`maven { url 'http://srch2.com/repo/maven' }`
3. In the `repositories` node in the `allprojects` node, add the entry:<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`maven { url 'http://srch2.com/repo/maven' }`
4. The `build.gradle` file should now look like:<br>![Including the SRCH2 server Maven repository in the top-level build.gradle file][tutorial-012] 
5. Now that the Gradle build script includes the SRCH2 server Maven repository, the SRCH2 Android SDK must be added as a dependency to the 'app' module. To do this, navigate to the `build.gradle` file in your `app` module.
6. In the `dependencies` node, add the entry:<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`compile group: 'com.srch2', name: 'srch2-android-sdk', version: '1.0.0-SNAPSHOT',  ext:'aar'`<br>The `build.gradle` file should now look like:<br>![Including the SRCH2-Android-SDK.aar file as a dependency in the app module's build.gradle file][tutorial-013] 
7. At the top of the editor window, you should be notified that Gradle needs to be synchronized. Click 'Sync Now' to refresh the Gradle build system to include these changes:<br>![Synchronizing the Gradle build system to include the new dependency][tutorial-014]
8. And that's all there is to it! To verify the SRCH2 Android SDK has been incorporated into your project, open up `SearchActivity` and try accessing the `SRCH2Engine` method `isUserAnAnteaterInATree()`: if **true**, you've successfully installed the SRCH2 Android SDK:<br>![Confirming the SRCH2 Android SDK is integrated into the application][tutorial-015]

###Creating an Index

The first step to searching an index is creating an index. Indexes in the SRCH2 search server are accessed in the SRCH2 Android SDK via the `com.srch2.android.http.service.Indexable` class. To create an index in the SRCH2 search server, create a subclass of `Indexable` to represent it:

```
public class MovieIndex extends Indexable {
}
```

In the `Hello SRCH2 Android SDK` project, an index representing data to be searched on about movie data is created called `MovieIndex`. An index is a collection of records defined by a data structure called the schema. The schema describes the fields of each record in that index such as the field type and default value. If the fields of the schema are comparable to the columns of an SQLITE table, then each record in the index another row in that table. An index also needs a name, which in the SRCH2 Android SDK is the identifier by which a reference to each sub-classed instance of `Indexable` can be obtained. Thus for every `Indexable` subclass there are two abstract methods that must be implemented: `String getIndexName()` and `Schema getSchema()`. Since the name and the names of the schema's fields will be reused, they will be made <b>constant</b> field members of this class:

```
public class MovieIndex extends Indexable {

    public static final String INDEX_NAME = "movies";
	
    public static final String INDEX_FIELD_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_TITLE = "title";
    public static final String INDEX_FIELD_YEAR = "year";
    public static final String INDEX_FIELD_GENRE = "genre";
}
```

And then the two abstract methods can be implemented as:

```
public class MovieIndex extends Indexable {

	...
	
	@Override
    public String getIndexName() {
        return INDEX_NAME;
    }

    @Override
    public Schema getSchema() {
        PrimaryKeyField primaryKey = Field.createDefaultPrimaryKeyField(INDEX_FIELD_PRIMARY_KEY);
        Field title = Field.createSearchableField(INDEX_FIELD_TITLE, 3);
        Field year = Field.createRefiningField(INDEX_FIELD_YEAR, Field.Type.INTEGER);
        Field genre = Field.createSearchableField(INDEX_FIELD_GENRE);
        return Schema.createSchema(primaryKey, title, year, genre);
    }
}
```

The values returned for these two methods should **never** change. The method `getIndexName()` returns the value `movies` setting the name of the index this `Indexable` represents.

The `getSchema()` method is slightly more complicated. Whenever a schema is defined, it must **always** contain a primary key field. The value of this field should be unique for each record in the index. In addition to the primary key, since this is an index representing data about movies, three fields are added to the schema: two fields that are searchable and one that is refining. A searchable field has textual data that will be searched during on for every record in the index during a search query; a refining field is for storing data values that can be used for query filtering and post-processing. Both refining and searchable field data can be retrieved from the raw search results returned by the SRCH2 search server. `Field` instances can be obtained by the static factory method of the `Field` class. 

For searchable fields, an additional parameter can be passed which will set the field's relative ranking (or relevance) to the other searchable fields--the default value is one. Here, the relevance of the field `title` is set to be three times more important than the fields `genre`.

Like a `Field` instance, a `Schema` instance is obtained by the static factory method of the `Schema` class. The order of arguments is important when creating a default `Schema` object: the first parameter is **always** the primary key and any subsequent parameters are the rest of the fields you define, in no particular order.

The `SRCH2Engine` will use the values returned from `getIndexName()` and `getSchema()` to automatically create the configuration for this index. While we're in this class, let's review how to form records to be inserted in the movie index. Observe the method `public static JSONArray getAFewRecordsToInsert()` implemented as follows:

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
			
			...
			
        } catch (JSONException oops) {
            // We know there are no errors
        }
        return jsonRecordsToInsert;
    }
```

The `SRCH2Engine` accepts a `JSONObject` or a `JSONArray` of `JSONObject`s when inserting or updating records. Insertions (and updates) are invoked by calling `insert(...)` on your `Indexable` implementation. Each `JSONObject` representing a record should be properly formed and contain as its keys only the names of the fields that were defined in the schema and its values should be of the type as defined in the schema, as well. When using a `JSONArray` to do a batch insert, the `JSONArray` should only contain the set of `JSONObject`s representing the set of records to insert. 

Before actually calling this method we will need two asynchronous callbacks to get index information and search results from the SRCH2 search server.

###Getting Search Results

The SRCH2 search server passes information back to the `SRCH2Engine` through two asynchronous callbacks: `com.srch2.android.http.service.SearchResultsListener` and `com.srch2.android.http.service.StateResponseListener`. The first passes back search results from queries performed while the second passes back information about the indexes and engine (covered in the next section). These two interfaces *should be* implemented by you--in particular, the `SearchResultsListener` is the way in which you will receive search results. 

For the demonstration of this tutorial it is assumed you are familiar with how a [`android.widget.BaseAdapter`](http://developer.android.com/reference/android/widget/BaseAdapter.html) works to populate a [`android.widget.ListView`](http://developer.android.com/reference/android/widget/ListView.html). Familiarity with how to communicate with the UI thread via a [`android.os.Handler`](http://developer.android.com/reference/android/os/Handler.html) is also helpful, but you can read more about this on the Android developer website, [Communicating with the UI Thread](https://developer.android.com/training/multiple-threads/communicate-ui.html). 

These two callbacks will execute in background threads, so in order to update the user interface of your application the search results must be passed to the UI thread. In this tutorial we will use subclass of the `android.os.Handler` class to implement the `SearchResultsListener` interface; however, other means of propagating the search results to the UI thread can be implemented, such as `runOnUiThread()` can be used. 

In the source code of this tutorial there is the class `SearchResultsAdapter` which extends `BaseAdapter`; here we will cover how to connect this adapter to a handler implementing `SearchResultsListener`. In the class `SearchResultsAdapter` there is a nested subclass of the `Handler` class:

```
public class SearchResultsAdapter extends BaseAdapter {

	...

    private static class SearchResultsUiHandler extends Handler implements
            SearchResultsListener {
	}
}
```

In this nested class, two constant `int` fields that will be used as the message keys for this handler:

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

Also declared is a `WeakReference<T>` of type `SearchResultsAdapter`. The `WeakReference<SearchResultsAdapter>` is necessary to prevent leaking the context (you can read more about why this is necessary [here](http://www.androiddesignpatterns.com/2013/01/inner-class-handler-memory-leak.html)) and can be defined in the constructor such as:

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

Now this handler subclass is ready to pass results form the `SRCH2Engine` callback to the adapter populating the list view. This handler implements `SearchResultsListener` which necessarily overrides its interface method `public void onNewSearchResults(int httpResponseCode, String jsonResultsLiteral, HashMap<String, ArrayList<JSONObject>>)` in the following way:

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

Here it is verified that `httpResponseCode` returned the status code OK, and the `ArrayList` of `JSONObject` instances corresponding to our `MovieIndex` is retrieved. If present (not -null), each `JSONObject` is converted into a `MovieSearchResult` by retrieving the JSON values associated the JSON keys of the field names defined earlier. Once converted, each `MovieSearchResult` is added to the `newResults ArrayList` which will be passed to the handler via a `Message` object. `Message.obtain(...)` is used to obtain a message from our handler, and an overloaded signature of this method is used to set the `msg.what` (to indicate no new results or zero new results depending on the size of the `newResults ArrayList`) and the `msg.obj` as the `newResults ArrayList`. If you wanted to do further post-processing, since this callback is triggered off the UI-thread, you can do any other background operations before pushing the search results to the UI thread via the handler.

Finally, by overriding the handler's `public void handleMessage(Message msg)` superclass method the results can be pushed to the UI thread, since this handler is created when the `SearchResultsAdapter` is initialized in the `protected void onCreate(Bundle savedInstanceState)` method of `SearchActivity`. Overriding this method to do this looks like:

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

The other asynchronous callback that can be implemented is the `StateResponseListener`. For this tutorial, a new nested class is defined inside of `SearchActivity` declared as `private static class SRCH2ControlListener implements StateResponseListener`. By overriding its methods the status results of operations performed on indexes, such as when specific records are requested by their primary key or when the SRCH2Engine is ready to start searching, can be accessed. For this class, a reference to the `SearchActivity` is kept to display a toast when the SRCH2 search server is ready for searching or when an information request on an index is completed (implementing of which is left as an exercise for the reader). 

```
public class SearchActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {

	...
	
    private static class SRCH2StateResponseListener implements
            StateResponseListener {
			
		private SearchActivity mActivity;
	
        public SRCH2StateResponseListener(SearchActivity SearchActivity) {
            mActivity = SearchActivity;
        }
	}
}
```

In detail, here are the insert, update and delete completion callback methods:

```
public class SearchActivity extends Activity implements
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
            }
        }
		
		@Override
        public void onUpdateRequestComplete(final String indexName,
                                            final UpdateResponse response) {
            Log.d(TAG, "Update for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            }
        }
		
	    @Override
        public void onDeleteRequestComplete(final String indexName,
                                            final DeleteResponse response) {
            Log.d(TAG, "Delete for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            }
        }
	}
}
```

These three methods will be triggered whenever its corresponding action is complete. For these callbacks, the first parameter indicates which `Indexable` the second parameter corresponds to, and this second parameter is a `response` which is a subclass of `RestfulResponse`. These classes wrap the raw JSON response as returned by the SRCH2 search server for each control-type task so that the data contained can be accessed from Java without parsing. These three `Restful Response` subclasses contain the raw RESTful JSON response, as well as the success and failure counts of the insert, update or delete task performed. For example, after the `MovieIndex`'s method `insert(getAFewRecordsToInsert())` is called and the SRCH2 search server finishes inserting the three records, the `SRCH2Engine` will parse the restful JSON response and trigger this callback which will contain a `successCount` of three. It is useful to note that each subclass of `RestfulResponse' has its `toString()` method overridden, as well as the convenience method `toToastString()` which contains line breaks for each field.  

Moving on, the method:

```
public class SearchActivity extends Activity implements
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
            }
        }
	}
}
```

will pass the `GetRecordResponse response` which will contain the record requested, as the original `JSONObject` as it was defined when it was inserted. 

There is also a callback for obtaining information on the indexes themselves, which occurs when the method `info()` of an `Indexable` is called which will trigger the callback method:

```
public class SearchActivity extends Activity implements
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

to be called. Note that like the `SearchResultsListener` these callbacks occur off the of the UI thread so in order to display the toast, the method `runOnUiThread(...)` is used to push the returned information to the UI thread.

The `InfoResponse` object contains the fields that describe the state of an index:

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
public class SearchActivity extends Activity implements
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

In the next section we'll see the call to initialize, start and stop the SRCH2 search server, but before that observe the logic to determine if the `MovieIndex` needs to be created--that is, whether the initial set of records should be inserted. Since the value of the key 'movies' in the map `indexesToInfoResponse` is an `InfoResponse` object with the method `getNumberOfDocumentsInTheIndex()`, the following logic can be implemented in this callback method:

```
public class SearchActivity extends Activity implements
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
  
Now when the SRCH2 search server starts, it'll trigger the `SRCH2Engine` to callback this method. The first time this method gets called, our `MovieIndex` will contain no records. The `InfoResponse` `getNumberOfDocumentsInTheIndex()` will reflect this, returning a value of zero, which in cause the outer if statement to evaluate true, causing the method `SRCH2Engine.insertIntoIndex(MovieIndex.INDEX_NAME, MovieIndex.getAFewRecordsToInsert())` to execute. Shortly afterwards, the method `onInsertRequestComplete(...)` will be triggered, containing an `InsertResponse` for the `movies` index: calling `getSuccessCount()` on this `InsertResponse` will return the value of records inserted which should be sixty four.

At this point, you could also call `MovieIndex.info()` and the `InfoResponse` returned would, instead of returning zero, return sixty four for the method `getNumberOfDocumentsInTheIndex()`. 

Also at this point, if the application was quit from and started again, the method `onSRCH2ServiceReady(...)` would not enter the inner if statement, since `movieIndexInfoResponse.getNumberOfDocumentsInTheIndex()` would return sixty four. This might be the time to do a referential integrity check and update the index...

###Putting it All Together

We are now ready to initialize the `SRCH2Engine` in our `SearchActivity` class. Initializing the `SRCH2Engine` is simple:

1. From `onCreate(...)` or wherever a new instance of your application is created, call `SRCH2Engine.initialize(...)` passing in each `Indexable`
2. Register the `SearchResultsListener` and `StateResponseListener`, you may reset it for instance if you are inserting from a `Service`, but in this example we'll also make these calls from `SearchActivity`'s `onCreate(...)`
3. Start and stop the `SRCH2Engine` by calling `SRCH2Engine.start(Context context)` and `SRCH2Engine.stop(Context context)` where needed. For an activity requiring search, the **recommended** place is to call these from `onResume()' and `onPause()` respectively

Make these calls in `SearchActivity` as follows:

```
public class SearchActivity extends Activity implements
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

Passing in each `Indexable` in `SRCH2Engine.initialize(Indexable firstIndex, Indexable... additionalIndexes)` causes the `SRCH2Engine` to create and pass the necessary configuration file to the SRCH2 search server. Then the two `SRCH2Engine` callbacks are registered. Finally, the two calls `start(Context context)` and `stop(Context context)` cause the `SRCH2Engine` to start and stop the SRCH2 search server: since this server is hosted by a remote service, the context is needed; a reference to this context is not held. It is **imperative** that for every call to `start(...)` the complementary call to `stop(...)` is made in order to let the SRCH2 search server come to a stop and not take up the device's resources. 

`SearchActivity` should now look something like this: 

```
public class SearchActivity extends Activity implements
        InstantSearchEditText.SearchInputEnteredObserver {

    private static final String TAG = SearchActivity.class.getSimpleName();

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

        private SearchActivity mActivity;

        public SRCH2StateResponseListener(SearchActivity SearchActivity) {
            mActivity = SearchActivity;
        }

        @Override
        public void onInsertRequestComplete(final String indexName,
                                            final InsertResponse response) {
            Log.d(TAG, "Insert for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            }
        }

        @Override
        public void onUpdateRequestComplete(final String indexName,
                                            final UpdateResponse response) {
            Log.d(TAG, "Update for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            }
        }

        @Override
        public void onDeleteRequestComplete(final String indexName,
                                            final DeleteResponse response) {
            Log.d(TAG, "Delete for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
            }
        }

        @Override
        public void onGetRecordByIDComplete(final String indexName,
                                            final GetRecordResponse response) {
            Log.d(TAG, "Got record for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
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

Note that SearchActivity implements the nested interface `InstantSearchEditText.SearchInputEnteredObserver` of the class `InstantSearchEditText` which is a subclass of `EditText` to capture character-by-character input: feel free to reuse this code in your own projects.

The last step is to connect the `InstantSearchEditText` and its interface `SearchInputEnteredObserver` to the `SRCH2Engine` through `SearchActivity`. 

```
public class SearchActivity extends Activity implements
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
2. Through the static `SRCH2Engine` method `getIndex(String indexName)` obtaining a specific `Indexable` subclass instance by its index name and using the non-static method from option 1.
3. Through the static `SRCH2Engine` method targeting all indexes `SRCH2Engine.searchAllIndexes(newSearchInput)`

Here we use the last method in case you'd like to add some more indexes, such as for music, contacts or anything you can think of. 

Whenever the SRCH2 search server does a search, the results come back fast and even if we had we inserted thousands of records, the search results would still take less than milliseconds to reach the `SearchResultsAdapter`. Because of this, it is recommended that, while developing with the SRCH2 Android SDK, you employ all of the optimizations for displaying data in a `ListView` as quickly as possible (such as the ViewHolder pattern, lazy loading of bitmaps if you incorporate icons, et cetera). 

###Where To Go From Here

This concludes the Hello SRCH2 Tutorial for learning how to get set up with the SRCH2 Android SDK. Play around with the project: try updating a record, deleting a record, adding more records with searchable data that is similar in its sequence of letters to appreciate the power of SRCH2's fuzzy search capability--and this is only the beginning! If you read on in [Advanced Topics](advanced-topics.md) you'll learn how to form powerful queries (such as filtering the search results for the `MovieIndex` by interval of year), perform lightning fast geo-searches using the device's location, or how to set up the SRCH2 Android SDK for Proguard, or how to manually interact with the running SRCH2 search server. Search on!


[tutorial-010]: ../img/010-tutorial.png "Opening the cloned Hello SRCH2 Android SDK application project"
[tutorial-011]: ../img/011-tutorial.png "The SRCH2 Android SDK in action!"
[tutorial-012]: ../img/012-tutorial.png "Including the SRCH2 server Maven repository in the top-level build.gradle file"
[tutorial-013]: ../img/013-tutorial.png "Including the SRCH2-Android-SDK.aar file as a dependency in the app module's build.gradle file"
[tutorial-014]: ../img/014-tutorial.png "Synchronizing the Gradle build system to include the new dependency"
[tutorial-015]: ../img/015-tutorial.png "Confirming the SRCH2 Android SDK is integrated into the application"
