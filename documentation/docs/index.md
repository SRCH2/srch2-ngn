
###Overview

The SRCH2 Android SDK is a tool for programmers to easily develop an
Android application that supports powerful search on mobile devices.
The SDK is utilizing a search engine written in C++ ground up
to deliver superior features with high performance.  The following
figure illustrates how this SDK works.

<center>![SRCH2 Android SDK Overview][overview]</center>

The SDK supports the following search features:

- Full-text search;
- Instant search (prefix predicates);
- Error correction (fuzzy search);
- Phrase search (e.g., "new york");
- Rich data types (e.g., text, int, float, and date);
- Attribute-based search (e.g., "cameron" in a "director" attribute);
- Sorting (e.g., "price < 20");
- Multiple languages;
- Multiple data sources;
- Token-based authorization.

<!-- ![Welcome][Welcome] -->

This tutorial will show you how to:

1. Incorporate the SDK into Android Studio projects;
2. Create indexes and insert records;
3. Enable powerful search in your app and display the results on the UI.

This tutorial assumes you are familiar with [Android
Studio](https://developer.android.com/sdk/installing/studio.html).
You should also have an Android device (not an emulator) to run these
instructions.  The Android OS version should be at least 4.0 (called
"Ice Cream Sandwich").  This application is set to compile with
Android SDK version 20.  In case you have not updated your Android SDK
build and platforms tools to include this version, you should do so
first.

###Quick Start

(1) Clone the tutorial application project from our [Github
repository](https://github.com/SRCH2/hello-srch2-android-sdk):

```
 shell> git clone https://github.com/SRCH2/hello-srch2-android-sdk.git
```

(2) Open Android Studio. From the 'File' menu option (or the 'Quick
Start' menu), select 'Import Project', and choose the root folder of
the cloned project: 

<center>![Opening the cloned Hello SRCH2 Android SDK application project][tutorial-010]</center>

(3) Connect an Android device to your computer. From the 'Run' menu
option, select 'Run app'.

(4) Once the app has launched, enter characters in the text input
field.  You should instantly see search results.  The app is doing
instant search on a few dozen movie records.  The following is a
screenshot after this step:

<center>![The SRCH2 Android SDK in action!][tutorial-011]</center>

<h2> Use a screenshot with more keywords such as "beaty ame" to show the
instant search results.  Also reduce the image size.</h2> 

Congratulations! You're now using the SRCH2 Android SDK to power
searches in an Android project.

The rest of this tutorial explains how this SDK works and how to use
it for your projects.

###Installing SDK
 
We first show how to install the SRCH2 Android SDK using the built-in
[Gradle build system](http://www.gradle.org/). Suppose you have an
existing Android project in Android Studio and you want to incorporate
the SDK. You need to add the SRCH2 server Maven repository to your
project as follows: 

(1) We want to configure the Gradle build script to include the SRCH2
 Maven server repository.  To do so, on the left panel of Android
 Studio, click the 'build.gradle' file of the project. Add the
 following line to the `buildscript.repositories` node and the
 `allprojects.buildscript` node,

```
maven { url 'http://srch2.com/repo/maven' }
```
as shown in the following figure:

<center>![Including the SRCH2 server Maven repository in the top-level build.gradle file][tutorial-012] </center>

(2) Next we need to add the SRCH2 Android SDK into the dependencies of
your project.  To do so, on the left  panel, click 'app', then click
'build.gradle'.  On the right panel, add the following line into the
`dependencies` node:

```
`compile group: 'com.srch2', name: 'srch2-android-sdk', version: '0.2.0',  ext:'aar'`
```

The following is a screenshot:

<center>![Including the SRCH2-Android-SDK.aar file as a dependency in the app module's build.gradle file][tutorial-013]</center>

Notice that this tutorial assumes the SDK version is 0.2.0, which
could be different from the latest version.  Please make sure to get
the latest SDK version from the [release page](releases.md) and update
the version number accordingly.

(3) At the top of the editor window, you should be notified that
Gradle needs to be synchronized. Click 'Sync Now' to refresh the
Gradle build system to include these changes, as shown in the
following figure:

<center>![Synchronizing the Gradle build system to include the new dependency][tutorial-014]</center>

To verify if the SDK has been incorporated into your project, open the
`SearchActivity` class file and  type in a `SRCH2Engine` method called
`isUserAnAnteaterInATree()` (as a dummy function).  If the editor can
show this function automatically, it means you've successfully
installed the SDK, as shown in the following figure: 

<center>![Confirming the SRCH2 Android SDK is integrated into the application][tutorial-015]</center>

Next we will go through several steps to create an index on a data set
to support powerful search.

###Creating Index

An index is created as a subclass of
`com.srch2.android.http.service.Indexable`.  For example, the
following declaration defines an index called `MovieIndex`
for the movie data set: 

```
public class MovieIndex extends Indexable {
}
```

Each index needs to implement two abstract methods called
`getIndexName()` and `getSchema()`, shown in the following example:

```
public class MovieIndex extends Indexable {
    
    public static final String INDEX_NAME = "movies";

    public static final String INDEX_FIELD_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_TITLE = "title";
    public static final String INDEX_FIELD_YEAR = "year";
    public static final String INDEX_FIELD_GENRE = "genre";

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

The `getIndexName()` function returns the name of the index.  The
`getSchema()` function returns a `Schema` object, which includes a
list of fields with their types. The values returned for these two
methods should *never* change. 

Whenever a schema is defined in the `getSchema()` function, it must
contain a primary key field. The value of this field should
be unique for each record in the index. In addition to the primary
key, there are three other fields related to movies: two fields that are searchable and one
that is refining. A "searchable" field has textual data that will be
searched in a query.  A "refining" field is for storing data values that can be used
for filtering and post-processing.  For instance, we can use the
"year" refining attribute to specify a predicate "year > 2005" on
search results, or use it to sort the results. The values of refining and searchable
fields can be retrieved from the results returned by
the server. `Field` objects can be obtained by the
static factory method of the `Field` class.  

For a searchable field, an additional parameter can be passed as
the field's boost value, which is a relevance number that can be used in the ranking function
to compute the relevance of each answer.  Its default value is 1.
In our running example, the boost value of the field `title` is set to be 3,  while
the value for the field `genre` is the default value 1.  Check [this
  page](http://srch2.com/documentation/ranking) for more
  information about how the engine ranks results.

A `Schema` instance is obtained by the static factory method of the
`Schema` class. The order of arguments is important when creating a
default `Schema` object: the first parameter is *always* the primary
key, and the subsequent parameters are the rest of the fields, in no
particular order. 

Next we show how to form records to be inserted in the movie
index. The following method generates a `JSONArray` instance
consistent with the schema:

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

The SDK accepts a `JSONObject` or a `JSONArray` of `JSONObject`s when
inserting or updating records. Insertions and updates are invoked by calling
`insert()` and `update()` of the `Indexable` object, respectively.
For example, we can call the following method to insert those records
to the index:
```
    MovieIndex movieIndex = new MovieIndex();
    movieIndex.insert(getAFewRecordsToInsert());
```


###Sending Queries

We can call the `Indexable.search(String searchInput)` to search on a
specific index. For example, the following function call searches for
movie records that match the keywords "beaty ame":
```
    movieIndex.search("beaty ame");
```

The SDK also provides a function `SRCH2Engine.searchAllIndexes()`
that can return results from all indexes registered using the
`SRCH2Engine.initialize()` function. 
<h>Move materials from the Advanced Topic page to describe how the
engine interprets this query string.</h>

###Getting Results

The SRCH2 server passes information back to the `SRCH2Engine`
class through two asynchronous callbacks:
`com.srch2.android.http.service.SearchResultsListener` and
`com.srch2.android.http.service.StateResponseListener`. The first one
returns search results, and the second one returns state
information about the index and engine. 

These two callbacks will be called by background threads.  So in order
to update the user interface of your application, the search results
must be passed to the UI thread.  Here we assume you are familiar with
how a
[`android.widget.BaseAdapter`](http://developer.android.com/reference/android/widget/BaseAdapter.html)
works to populate a
[`android.widget.ListView`](http://developer.android.com/reference/android/widget/ListView.html). 
Familiarity with how to communicate with the UI thread via a
[`android.os.Handler`](http://developer.android.com/reference/android/os/Handler.html)
is also helpful, but you can read more about it on the 
[Android developer
web site](https://developer.android.com/training/multiple-threads/communicate-ui.html).  

In this tutorial we will use a subclass of the `android.os.Handler`
class to implement the `SearchResultsListener` interface.  However, other means
of propagating the search results to the UI thread can also be implemented, e.g., 
by using `runOnUiThread()`.

In the source code of this tutorial, there is a class called `SearchResultsAdapter` that
extends `BaseAdapter`.  Here we connect this adapter to
a handler implementing `SearchResultsListener`. In the class
`SearchResultsAdapter`, there is a nested subclass of the `Handler` class:

```
public class SearchResultsAdapter extends BaseAdapter {

	...

    private static class SearchResultsUiHandler extends Handler implements
            SearchResultsListener {
	}
}
```

In this nested class, two constant `int` fields are used as the
message keys for this handler: 

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

Also declared is a `WeakReference<T>` of type `SearchResultsAdapter`. The
`WeakReference<SearchResultsAdapter>` is necessary to prevent leaking the
context (you can read more about why this is necessary
[here](http://www.androiddesignpatterns.com/2013/01/inner-class-handler-memory-leak.html))
and can be defined in the constructor such as:

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

Now this handler subclass is ready to pass results from the `SRCH2Engine`
callback to the adapter populating the list view. This handler implements
`SearchResultsListener`, which necessarily overrides its interface method
`onNewSearchResults()` as follows:

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

Any time one of the search methods of the API is called (such as
`search()`, `onNewSearchResults(...)` will be
triggered when the search results are returned from the SRCH2 server.
Its parameters are:

1. `httpResponseCode`: it indicates how the RESTful action was handled;
2. `jsonResultsLiteral`: it is the raw JSON literal as
returned by the search server containing the search result;
3. `resultRecordMap`: it is a
mapping of index names to their corresponding results parsed from the
`jsonResultsLiteral` literal. 

The `resultRecordMap` will never be null: if there were no results for any of
the indexes you've defined, the corresponding values for
`ArrayList<JSONObject>` will be of size zero. 

The callback function checks the status of the response, consumes
the results in the map, and adds the results to an object called `newResults`.
Finally, by overriding the handler's `handleMessage()`
superclass method, the results can be pushed to the UI thread.  This
handler is created when the `SearchResultsAdapter` is initialized in the
`onCreate()` method of `SearchActivity`. Overriding this method to do this looks like:

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

###Check Status

The other asynchronous callback is
`StateResponseListener`. For this tutorial, a new nested class is
defined inside of `SearchActivity` declared as `private static class
SRCH2ControlListener implements StateResponseListener`. By overriding
its methods, we can access the status results of operations performed on indexes,
such as when specific records are requested by their primary key or
when the SRCH2Engine is ready to start searching.

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

	}
}
```

These `onInsertRequestComplete`, `onUpdateRequestComplete`, and
`onDeleteRequestComplete` methods will be triggered whenever its corresponding
action is complete. For these callbacks, the first parameter indicates which
`Indexable` is called, and this second parameter is a `response`, which is a
subclass of `RestfulResponse`.  These three `Response` subclasses
contain the raw RESTful JSON response, as well as the success and failure
counts of the insert, update, or delete tasks performed.  For example, after the
`MovieIndex`'s method `insert(getAFewRecordsToInsert())` is called and the
SRCH2 server finishes inserting the three records, the `SRCH2Engine`
will parse the restful JSON response and trigger this callback, which will
contain a `successCount` of three. 

The `onGetRecordByIDComplete` callback will pass the `GetRecordResponse
response`, which contains the record requested by the
`Indexable.getRecordbyID()` function.

Another important callback function is `onSRCH2ServiceReady()`,
which is triggered as soon as the SRCH2 server is up and running. The
user can use this callback to check the status of the current loaded index,
such as the record number inside the index.  For instance, the following example
shows that after the server is ready, we can insert records into the index.

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
					SRCH2Engine.getIndex(MovieIndex.INDEX_NAME).insert(MovieIndex.getAFewRecordsToInsert());
				} else {
					// Records already existed in the index, maybe time to check if the index need to be updated
				}
			}
		}
	}
}
```
  
###Completing Lifecycle

The `SRCH2Engine` lifecycle methods can be called in the following sequence:

1. In *Activity.onCreate()*, call `SRCH2Engine.initialize()` by
passing all the `Indexable` objects to initialize the server.
2. Register the `SearchResultsListener` and `StateResponseListener`
objects by calling `setSearchResultsListener()` and
`setStateResponseListener()`;
3. Start the server by calling `start()`;
4. Stop the server by calling `stop(Context context)`.

For an activity requiring search, it is recommended to call
`SRCH2Engine.start()` in `Activity.onResume()`, and call `SRCH2Engine.stop()` in
`Activity.onPause()`.  It is *imperative* that for every call to
`start()`, the complementary call to `stop()` is made in order to let the
SRCH2 server stop, so that it does not take up the device's resources.

The following code shows how to setup the engine in the corresponding methods in the 
lifecycle of the Android application:

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

The `SearchActivity` implements the nested interface
`InstantSearchEditText.SearchInputEnteredObserver` of the class
`InstantSearchEditText`, which is a subclass of `EditText` to capture
character-by-character input.  Feel free to reuse this
[code](https://github.com/SRCH2/hello-srch2-android-sdk/tree/master/Hello-SRCH2-Android-SDK/app/src/main/java/com/srch2/android/demo/helloworld)
in your own projects.

Whenever the SRCH2 server does a search, the results come back instantly. It
is recommended that, while developing with the SDK, you employ
optimizations for displaying data in a `ListView` as quickly as
possible, such as the ViewHolder pattern, lazy loading of bitmaps if you
incorporate icons, et cetera. 

###Next Steps

This tutorial shows how to use the SRCH2 Android SDK to do powerful search on 
an application. Play around with the project, try updating a record,
deleting a record, adding a record with searchable data that is similar in
its sequence of letters to appreciate the power of SRCH2's search
capability, and this is only the beginning! If you read on in [Advanced
Topics](advanced-topics.md), you'll learn how to form powerful queries (such as
filtering the search results for the `MovieIndex` by interval of year), or how to set up the
SDK for Proguard, or how to manually interact with the running
search server. Search on!

Please check our [Java doc](../javadoc) for the more powerful search options.

[tutorial-010]: ../img/010-tutorial.png "Opening the cloned Hello SRCH2 Android SDK application project"
[tutorial-011]: ../img/011-tutorial.png "The SRCH2 Android SDK in action!"
[tutorial-012]: ../img/012-tutorial.png "Including the SRCH2 server Maven repository in the top-level build.gradle file"
[tutorial-013]: ../img/013-tutorial.png "Including the SRCH2-Android-SDK.aar file as a dependency in the app module's build.gradle file"
[tutorial-014]: ../img/014-tutorial.png "Synchronizing the Gradle build system to include the new dependency"
[tutorial-015]: ../img/015-tutorial.png "Confirming the SRCH2 Android SDK is integrated into the application"
[overview]: ../img/Android-SDK-Overview.png "SRCH2 Android SDK Overview"
