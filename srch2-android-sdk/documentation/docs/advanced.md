<h1>Advanced Features</h1>

In this documentation we explain advanced features of the SRCH2 Android SDK,
such as how to formulate a query with various conditions, how to use
the SDK to do testing and use Proguard.

##Queries

In the [basic tutorial](index.md), we showed how to send a search
"beaty ame" to the SRCH2 engine.  By default, the engine uses the
space delimiter to tokenize the string to multiple keywords ("beaty"
and "ame").  It treats the last keyword ("ame") as a prefix condition,
and other keywords (e.g., "beaty") as complete keywords.  By default,
the engine supports fuzzy search by allowing one typo for every three
characters in a keyword. 

Often we want to have more control on the keywords (a.k.a.,
terms) in a query, such as prefix versus complete keyword and their
fuzziness.  The SDK allows you to have this control by using
a *SearchableTerm* object, customizing a *Query* object, and then
calling *Indexable.advancedSearch()* method.  In particular, we can
use the *Query* class to enable the advanced search features by
calling the *Indexable.advancedSearch(Query query)* method.  A *Query*
object is constructed using a *Term* object. It can be a
*SearchableTerm* or a *CompositeTerm* that includes
boolean operators on the multiple *SearchableTerm* objects. 
All the conditions set on the *Term* object will be taken into the
*Query* object. In addition, it allows us to set a filter, a sorter, and
other operators to refine the returned query results.

##Query Examples

The following examples show some of these advanced features.
Suppose we have defined two query terms: term 1 specifies a condition 
that the keyword "terminator" has to appear in the <i>title</i> field,
and term 2 says that the keyword "cameron" needs to be in the
<i>director</i> field.

```
  SearchableTerm term1 = new SearchableTerm("terminator").searchSpecificField("title");
  SearchableTerm term2 = new SearchableTerm("cameron").searchSpecificField("director");
```

<li> Query q1 finds records with the keyword "terminator" in its
<i>title</i> field and the keyword "cameron" in its <i>director</i>
field:</li> 

```
  Query q1 = new Query(term1.AND(term2));
```

<li> Query q2 filters the results based on the year field, i.e., the
year value needs to be between 1983 and 1992 (both inclusive).</li> 

```
  Query q2 = new Query(term1.AND(term2));
  q2.filterByFieldInRange("year", "1983", "1992");
```

<li> Query q3 filters the results by requiring that the genre value of
a movie is "action". </li>
```
  Query q3 = new Query(term1.AND(term2));
  q3.filterByFieldEqualsTo("genre", "action");
```

<li> Query q4 sorts the results based on the year attribute in the
ascending order.</li> 
```
  Query q4 = new Query(term1.AND(term2));
  q4.sortOnFields("year").orderByAscending();
```

Next we explain the details of these advanced features.

##Search on Fields

By default a record is a matching answer for a keyword as long as the keyword
appears in one of the *searchable* fields defined in the schema
returned by *Indexable.getSchema()* method.  If we want to specify
attributes in which a keyword needs to appear, we can 
use the *searchSpecificField()* method.  For example, we can set the
following search term to only search for the keyword "wind" in the
field *title*.
```
  new SearchableTerm("wind").searchSpecificField("title");
```

##Prefix Condition

In type-ahead search, we may want to treat a term, especially the
last term in a query ("ame" in the example query "beaty ame") as a prefix
condition.  That is, a record is considered to match this term if
the record has a keyword (e.g., "american") with this term as a
prefix.  To specify such a condition, we can call the member method
*setIsPrefixMatching()* of the *SearchableTerm* class to enable or
disable prefix matching on this keyword. By default, it is "false",
i.e., a *SearchableTerm* object is treated as a complete-word condition.

For example, the following code sets "american" as a complete keyword
and the "beau" as a prefix keyword:
```
  SearchableTerm term1 = new SearchableTerm("american").setIsPrefixMatching(false);
  SearchableTerm term2 = new SearchableTerm("beau").setIsPrefixMatching(true);
  Query query = new Query(term1.AND(term2));
```

##Fuzzy Condition

The SRCH2 engine supports fuzzy search based on [Levenshtein distance (edit
distance)](http://en.wikipedia.org/wiki/Levenshtein_distance). 
We can call the method *enableFuzzyMatching()* to enable the fuzzy
match condition of a term. By default, it is "disabled", i.e., a
*SearchableTerm*  object is treated as an exact-keyword condition.

We use a customizable threshold (normalized based on the term length) to
determine the edit distance used in finding matching keywords.  The
*Similarity Threshold* is a normalized value (a float number between 0 and 1)
for a term.
Let "s" be the similarity threshold for a term given in the query. The engine
will use the formula
```
  floor((1-s) * length(keyword))
```
to compute the edit-distance threshold to do the search. If "s" is 1, we do an
exact search. For example, consider the following query:
```
  SearchableTerm term = new SearchableTerm("spielberg").enableFuzzyMatching(0.8f);
```
The internal edit-distance threshold is *floor((1-0.8) * length("spielburrg")) =
floor(0.2 * 10) = 2*. So the engine will find records with a keyword whose edit
distance to the term "spielberg" is at most 2.

If we don't provide a similarity threshold, e.g.,
```
  SearchableTerm term = new SearchableTerm("spielberg").enableFuzzyMatching();
```
the engine will use the similarity threshold specified in the
*Indexable.getFuzzinessSimilarityThreshold()*.  If we don't
override this getter method, we will use the system's default threshold
of 0.65f.

##Term Boost

The engine allows us to specify a boost value for a term to indicate
how important this term is in the [ranking function](http://srch2.com/documentation/ranking/).
We can call the *setBoostValue(int)* method to
boost the importance of a given term. The boost value is an integer from 1 to
100. The higher the boost value, the more relevant the term will be. The default value is 1.
For example, if we are searching for two terms
"star" and "wars", and we want the term "star" to be more relevant by
giving it a boost value of 4, we can call 
```
  new SearchableTerm("star").setBoostValue(4).AND (new SearchableTerm("wars"));
```

We can specify prefix, fuzziness, and boosting conditions to a single term, e.g.,  
```
  new SearchableTerm("sta").enableFuzzyMatching(0.6f).setBoostValue(4).setIsPrefixMatching(true);
```
##Boolean Expression

The engine supports three boolean operators: AND, OR, and NOT. 
Each operator generates a *CompositeTerm*
object. Both *SearchableTerm* and *CompositeTerm* are inherited from
the *Term* class, which is used to initialize a *Query* object. 

For example, consider the following query:
```
  new SearchableTerm("star").AND(new SearchableTerm("wars")).OR(new SearchableTerm("George Lucas"));
```
It searches records that contain either "star" and "wars", or the phrase "George Lucas".
The following are a few more examples:
```
  new SearchableTerm("star wars").AND(new SearchableTerm("episode 3"))
   .OR(new SearchableTerm("George Lucas").NOT(new SearchableTerm("Indiana Jones")));

  new SearchableTerm("big").AND(new SearchableTerm("fish"))
   .AND((new SearchableTerm("Tim Burton").OR(new
   SearchableTerm("MacGregor").enableFuzzyMatching(0.5f)))); 
```

##Filter by Range

We can specify a range filter restricting the set of records by using
the *filterByFieldInRange*, *filterByFieldEqualsTo*,
*filterByFieldStartFrom*, and *filterByFieldEndsTo* methods.  In the
example below, only records having a year equal to 2012 will return.  
```
  new Query(new SearchableTerm("star")).filterByFieldEqualsTo("year", "2012");
```

A range query allows us to match records whose field value is between
a specified lower bound and upper bound (both inclusive).  For example:
```
  new Query(new SearchableTerm("star")).filterByFieldInRange("year", "2010", "2012");
```

A query can have multiple range filters. For example:
```
  new Query(new SearchableTerm("star"))
    .filterByFieldStartFrom("id", "1000")
    .filterByFieldEqualsTo("genre", "drama")
    .filterByFieldEqualsTo("year", "1975");
```
It returns the records with an *id* greater than or equal to 1000, a
*genre* of drama, and *year* less than or equal to 1975.

In this example, the engine treats these filters as conjunctive
predicates (i.e., using the "AND" semantic).  If we want the engine to
treat them as disjunctive predicates (i.e., using the "OR" semantic), 
we can call the *setFilterRelationOR()* method.

##Sorting

By default the engine sorts the results using a descending order based on
the overall score of each record.  We can specify sorting by other
fields. For example, the following query sorts the results based on
the *director*, *year*, and then *title* fields:
```
  new Query(new SearchableTerm("star")).sortOnFields("director", "year", "title");
```

We can specify the order in which the result set should be sorted. The default
behaviour is to sort them in the descending order. We can call the
*orderByAscending()* method to sort them in the ascending order.
For example, we can use the following code to sort the results
in the ascending order on the *year* field:
```
  new Query(new SearchableTerm("star")).sortOnFields("director", "year", "title").orderByAscending();
```

##Pagination

To implement pagination, we want to return some of the results
by specifying a starting offset and number of records.  
We can call the *pagingStartFrom()* method to specify the starting
offset, and its default value is 0.
We can also set the number of returned records by 
calling the *pagingSize()* method. The number is obtained
from the  *Indexable.getTopK()* method, and its default 
value is 10.

For example, the following statement returns records that are ranked from
the 25th to the 34th of all the results. 
```
  new Query(new SearchableTerm("star")).pagingStartFrom(25).pagingSize(10);
```

##Geo Search

The engine can index records with location information specified as a latitude
and a longitude, and do search based on both keywords and locations. For
example, we can use the engine to find stores called "ghirardelli" within two
miles to a location in San Francisco.  The engine provides all the features
such as instant search and fuzzy search, making it easy to develop a
mobile application to provide great user experiences.

To enable geo indexing, we need to create the geo type schema by calling the 
*Schema.createGeoSchema()* function. In addition to the normal 
*Schema.createSchema()* function, we need to provide the "latitude" and "longitude" 
field names to the schema. Here is an example:
```
  PrimaryKeyField primaryKey = Field.createDefaultPrimaryKeyField("id");
  Field nameField = Field.createSearchableField("name");
  Schema geoSchema = Schema.createGeoSchema(primaryKey, "lat", "lng", nameField);
```
The second argument is the *latitude* field name, and the third
argument is the *longitude* field name.
Each record should have two corresponding float values for these two fields.
For example:
```
 {"id" : "1234", "name" : "ghirardelli", "lat" : 43.22, "lng": -80.22}
```

We can use the *Query* object to search results inside a geo region,
such as a rectangle region or a circle region.
To search using a rectangle region, we can use the *Query.insideRectangle()* method by
specifying the latitudes and the longitudes of the left-bottom location and the top-right location.
To search using a circle region, we can use the *Query.insideCircle()* method by 
specifying the latitude and the longitude of the center point, and also the radius of the circle.

Here are some examples:
```
  new Query(new SearchableTerm("ghirardelli")).insideRectangle(61.2, -149.9, 61.2, -149.7);
  new Query(new SearchableTerm("ghirardelli")).insideCircle(61.2, -149.9, 5);
```

Often we want to search records within a region without specifying keywords.
We can use the *Query* constructor to create a geo type query.  For example:
```
  new Query(61.2, -149.9, 61.2, -149.7);
  new Query(61.2, -149.9, 5);
```
If we give four numbers, the *Query* will be treated as a
search using a rectangle region, in which the first two numbers are
the latitude and longitude of the left-bottom location,
while last two numbers are
the latitude and longitude of the top-right location.
If we give three numbers, it will be treated as a search using a
circle region, where the first two numbers are
the latitude and longitude of the center location, and the last number
is the radius.

##Highlighting
The SDK allows an index to specify a highlighter for matching keywords
of this index.  The highlighter, used by the engine, sets the pre and post html tags for matching keywords. 
We can specify the highlighter of an index by implementing the *getHighlighter()* method of the index object.

A highlighter can be enabled for certain fields.
The following code shows how to enable the feature on the field "title" and 
how to create a highlighter by the factory method.

```
public class MovieIndex extends Indexable {
  ...
  @Override
  public Schema getSchema() {
    PrimaryKeyField primaryKey = Field.createDefaultPrimaryKeyField(INDEX_FIELD_PRIMARY_KEY);
    Field title = Field.createSearchableField(INDEX_FIELD_TITLE, 3).enableHighlighting();
	Field genre = Field.createSearchableField(INDEX_FIELD_GENRE).enableHighlighting();
    ...
    return Schema.createSchema(primaryKey, title, genre);
  }

  @Override
  public Highlighter getHighlighter() {
    return Highlighter.createHighlighter()
            .formatExactTextMatches(true, false, "#FF0000")
            .formatFuzzyTextMatches(true, false, "#FF00FF");
  }
}
```

The highlighted result will be returned with the original records by the 
*SearchResultsListener.onNewSearchResults()* callback method.
The highlighted fields can be retrieved by using the constant value of 
*Indexable.SEARCH_RESULT_JSON_KEY_HIGHLIGHTED*. 
The keys in this *JSONObject* will also be the names
of those fields with highlighting enabled. 

If we use the highlighter that provided above, for the keyword of "beaty ame", 
the search result for the movie with the title "American Beauty " will
produce the output of 
```
'<b><font color="#FF0000">Ame</font></b>rican <b><font color="#FF00FF">Beauty</font></b>'
```
(or visually, <b><font color="#FF0000">Ame</font></b>rican <b><font color="#FF00FF">Beauty</font></b>).

The highlighted result should be used in conjunction with *Html.fromHtml(...)* such as
*mTextView.setText(Html.fromHtml(mHighlightTitleText))* to display it
properly on the UI. If custom tags are used, *SpannableString* can also be used to
present the formatted text properly.

The following code shows how to get the highlighted "title" field into the Adapter:
```
  @Override
  public void onNewSearchResults(int HTTPresponseCode,
                                 String JSONresponse,
                                 HashMap<String, ArrayList<JSONObject>> resultMap) {
    if (HTTPresponseCode == HttpURLConnection.HTTP_OK) {
      ArrayList<MovieSearchResult> newResults = new ArrayList<MovieSearchResult>();

      ArrayList<JSONObject> movieResults = resultMap
										   	 .get(MovieIndex.INDEX_NAME);
      if (movieResults != null && movieResults.size() > 0) {
        for (JSONObject jsonObject : movieResults) {
          MovieSearchResult searchResult = null;
          try {
            JSONObject originalRecord = 
                          jsonObject.getJSONObject(Indexable.SEARCH_RESULT_JSON_KEY_RECORD);
            JSONObject highlightedFields = 
                          jsonObject.getJSONObject(Indexable.SEARCH_RESULT_JSON_KEY_HIGHLIGHTED);
            searchResult = new MovieSearchResult(
									highlightedFields
                                      .getString(MovieIndex.INDEX_FIELD_TITLE),
                                    highlightedFields
                                      .getString(MovieIndex.INDEX_FIELD_GENRE),
          } catch (JSONException oops) {
            continue;
          }
          if (searchResult != null) {
            newResults.add(searchResult);
          }
        }
      }
	  
      if (newResults.size() > 0) {
        updateDisplayedSearchResults(newResults);
      } else {
        clearDisplayedSearchResults();
      }
    }
  }

```

##Custom Ranking
The SDK provides a *RecordBoostField* to determine each record's score when computing the relevance of
search results. This field will always be float in type, and should be set
from 1 to 100. 

The following code tells the SDK to take the value of the "recordBoost" field from the record
as the *RecordBoostField*:
```
public class MovieIndex extends Indexable {
  public static final String INDEX_FIELD_PRIMARY_KEY = "id";
  public static final String INDEX_FIELD_RECORD_BOOST = "recordBoost";
  public static final String INDEX_FIELD_TITLE = "title";


  @Override
  public Schema getSchema() {
    PrimaryKeyField primaryKey = Field.createDefaultPrimaryKeyField(INDEX_FIELD_PRIMARY_KEY);
    RecordBoostField recordBoost = Field.createRecordBoostField(INDEX_FIELD_RECORD_BOOST);
    Field title = Field.createSearchableField(INDEX_FIELD_TITLE);
    return Schema.createSchema(primaryKey, recordBoost, title);
  }
}
```

For instance, assume the user of the app was able to enter their favorite movie genres and they 
entered the genre 'science fiction'. Then when inserting the initial movie records (or by updating
each record) we can assign the value of the recordBoost field for a record like the following:

```
  public JSONArray getAFewRecordsToInsert() {

  ...
		
    JSONObject record = new JSONObject();
    record.put(INDEX_FIELD_PRIMARY_KEY, "1");
    record.put(INDEX_FIELD_TITLE, "The Good, the Bad And the Ugly");
    record.put(INDEX_FIELD_YEAR, 1966);
    record.put(INDEX_FIELD_GENRE, "Western Adventure");
	record.put(INDEX_FIELD_RECORD_BOOST, computeRecordBoostScore("Western Adventure"));
    jsonRecordsToInsert.put(record);
			
	...
			
	record = new JSONObject();
    record.put(INDEX_FIELD_PRIMARY_KEY, "14");
    record.put(INDEX_FIELD_TITLE, "The Matrix");
    record.put(INDEX_FIELD_YEAR, 1999);
    record.put(INDEX_FIELD_GENRE, "Science Fiction Action");
	record.put(INDEX_FIELD_RECORD_BOOST, computeRecordBoostScore("Science Fiction Action"));
    jsonRecordsToInsert.put(record);
			
	...
	
  ...
  }
	
  public float computeRecordBoostScore(String genre) {
    if (genre == null) {
      return 1;
    }
    return genre.contains("Science Fiction") ? 50 : 1;
  }
```	
Any movie of the genre "Science Fiction" will be boosted 50 times higher in the search results. 

##Connecting to SQLite

The SRCH2 Android SDK supports automatic generation of indexes from
SQLite database tables. The SRCH2 server 
creates triggers inside each of these tables connected to the SDK, which
automatically observe changes in the table, much like how it is possible to register a 
*android.database.ContentObserver* within the Android SDK. To do so, extend the abstract class
*com.srch2.android.sdk.SQLiteIndexable* in your Android application
project.  This class, similar to
*com.srch2.android.sdk.Indexable*, requires that the method *getIndexName()* be overridden to return
the name of the index as handle.  Other methods such as
*getHighlighter()*, *getRecordCount()*, and *getTopk()*, are also
available to customize the behavior of the *SQLiteIndexable* search
result output like doing so for *Indexable* instances. 

One critical difference between implementing the *Indexable* and
*SQLiteIndexable* classes, is that it is not necessary to override
*getSchema()*, as it will be automatically generated by the
*SRCH2Engine*. For this to work, implementations of *SQLiteIndexable*
must override a few additional methods, namely
*getSQLiteOpenHelper()*, *getTableName()*, and *getDatabaseName()*.

The method *getSQLiteOpenHelper()* should return the subclass instance
of the *SQLiteOpenHelper* used to manage the database.  A reference to
the *SQLiteOpenHelper* is not kept, and only a simple query is
performed when *SRCH2Engine.initialize()* is called to configure the 
indexes. For those interested in the technical details, the SQLite
command `PRAGMA table_info(...)` returns the columns of the specified
table in order for the columns of that table to be mapped to the
*Field* attributes, so the correct *Schema* can be generated. The
return value of the method *getTableName()* specifies which table in
the database the *SQLiteOpenHelper* manages to index, while
*getDatabaseName()* specifies which database contains the table to be
indexed. These should **exactly match** the values used in the create
table string and the *SQLiteOpenHelper* constructor. 

Another critical difference is only search requests are possible on
*SQLiteIndexable* implementations, as creating and editing table data
should be managed by the *SQLiteOpenHelper*. Thus the
*SQLiteIndexable* class does not contain the *Indexable* CRUD
callbacks such as *onInsertComplete*.  It does, however, 
have the *onIndexReady* callback notifying when the index the
*SQLiteIndexable* represents is ready for searching. 

The following code illustrates implementing an *SQLiteIndex* to
connect the SRCH2 server to a SQLite table:

```
public class DatabaseHelper extends SQLiteOpenHelper {

  public static final int DATABASE_VERSION = 1;
  public static final String DATABASE_NAME = "library";
  public static final String TABLE_NAME = "books";
  public static final String COLUMN_PRIMARY_KEY = "id";
  public static final String COLUMN_AUTHOR = "author";
  public static final String COLUMN_TITLE = "title";
  public static final String COLUMN_GENRE = "genre";
  public static final String COLUMN_YEAR = "year";
  public static final String COLUMN_SCORE = "score";
  public static final String COLUMN_THUMBNAIL = "thumbnail";

  private static final String getCreateTableString() {
    return "CREATE TABLE " + TABLE_NAME +
			" ( " +
                COLUMN_PRIMARY_KEY + " INTEGER PRIMARY KEY NOT NULL, " +
                COLUMN_AUTHOR + " TEXT, " +
                COLUMN_TITLE + " TEXT, " +
				COLUMN_GENRE + " TEXT, " +
				COLUMN_YEAR + " INTEGER, " +
                COLUMN_SCORE + " REAL, " +
                COLUMN_THUMBNAIL + " BLOB " +
			" )";
    }
	
  public DatabaseHelper(Context context) {
    super(context, DATABASE_NAME, null, DATABASE_VERSION);
  }
	
  @Override
  public void onCreate(SQLiteDatabase db) {
    db.execSQL(getCreateTableString());
  }

  @Override
  public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
    db.execSQL("DROP TABLE IF EXISTS " + TABLE_NAME);
    onCreate(db);
  }
	
  public boolean insertRecords(ArrayList<Book> insertSet) {
    ...
  }
}
```
The SQLiteIndexable class linking to this SQLite table:
```
public class SQLiteBookIndex extends SQLiteIndexable {
  private final DatabaseHelper mSQLiteOpenHelper;

  public SQLiteBookIndex(DatabaseHelper databaseHelper) {
	mSQLiteOpenHelper = databaseHelper;
  }

  @Override
  public String getIndexName() {
    return DatabaseHelper.TABLE_NAME;
  }

  @Override
  public SQLiteOpenHelper getSQLiteOpenHelper() {
    return mSQLiteOpenHelper;
  }

  @Override
  public String getDatabaseName() {
    return DatabaseHelper.DATABASE_NAME;
  }

  @Override
  public String getTableName() {
    return DatabaseHelper.TABLE_NAME;
  }

  @Override
  public String getRecordBoostColumnName() {
    return DatabaseHelper.COLUMN_SCORE;
  }

  @Override
  public int getColumnBoostValue(String textTypeColumnName) {
    int fieldBoostValue = 1;
    if (textTypeColumnName.equals(DatabaseHelper.COLUMN_AUTHOR)) {
      fieldBoostValue = 25;
    } else if (textTypeColumnName.equals(DatabaseHelper.COLUMN_TITLE)) {
      fieldBoostValue = 50;
    } else if (textTypeColumnName.equals(DatabaseHelper.COLUMN_GENRE)) {
       fieldBoostValue = 10;
    }
    return fieldBoostValue;
  }

  @Override
  public boolean getColumnIsHighlighted(String textTypeColumnName) {
    return true;
  }

  @Override
  public Highlighter getHighlighter() {
    return Highlighter.createHighlighter()
                .formatExactTextMatches(true, false, "#FF0000")
                .formatFuzzyTextMatches(true, false, "#FF00FF");
  }

  @Override
  public void onIndexReady() {
    super.onIndexReady();
  }
}
```

These last three methods *getRecordBoostColumnName()*,
*getColumnBoostValue()*, and *getColumnIsHighlighted()*  
do not have to be overridden.  However by doing so, the properties 
of an *Indexable* schema corresponding to the
*RecordBoostField*, the *boost* argument of the
*Field.getSearchableString(String name, int boost)*, 
and the *enableHighlighting()* of a *Field*,
can be set for
this *SQLiteIndexable*.

If *getRecordBoostColumnName()* is overridden, it must correspond to
the name of a column that is of type *REAL* and should contain
positive values less than 100 and greater than or equal to 1. Here the
column named score is used as the *RecordBoostField* which will set
the relative rank of each record in a search result.  

During the resolution of the schema for the index this
*SQLiteIndexable* represents, the *SRCH2Engine* will call  
the method *getColumnBoostValue(String textColumnName)* for each
column of type *TEXT* found in the table. The value returns will
determine the relative importance of each column of type *TEXT*--that
is, the searchable fields in the index. Since books are 
generally looked for by title, 50 is returned for the title column, 25
is returned for the author column, and 10 for the genre column. In
event a value less than 1 or greater than 100, or column boost is not
specified, the default value of 1 will be set for that column.

The same logic is used to determine which columns of type *TEXT* are
to be highlighted with the method *getColumnIsHighlighted(String
textColumnName)*. Here we enable highlighting for all columns of type
*TEXT*.

Implementations of the *SQLiteIndexable* class can support creating a
geo-index. If two columns of the table contain longitudinal and
latitude data, override both *getLatitudeColumnName()* and
*getLongitudeColumnName()* returning the column names as they appear
in the "create table" string used to create the table. These columns
should by of type *REAL*.  

As for an *Indexable*, *SQLiteIndexable* implementations must be
registered by calling *SRCH2Engine.setSQLiteIndexables()* before
calling *SRCH2Engine.onResume()*. To initialize the
*DatabaseIndexable* in the *SRCH2Engine*: 

```
public class SearchActivity extends Activity {

  DatabaseHelper mDatabaseHelper;
  SQLiteBookIndex mDatabaseIndexable;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
	super.onCreate(savedInstanceState);
	mDatabaseHelper = new DatabaseHelper(this);
	mDatabaseIndexable = new SQLiteBookIndex(mDatabaseHelper);
  }
  
  @Override
  protected void onResume() {
    super.onResume();
	SRCH2Engine.setSQLiteIndexables(mDatabaseIndexable);
	SRCH2Engine.onResume(this);
  }
  
  @Override
  protected void onPause() {
	super.onPause();
	SRCH2Engine.onPause();
  }
  
  ...

}
```

This is all it takes to the connect the database's 'library' table to
the SRCH2 search server. Now the SRCH2 server will generate an index
representing this table and automatically update the index to reflect
any changes in that table's data content.

There are three important notes that must be taken into consideration
when incorporating a SQLite database table into the SDK:

1. The table must contain one column specified as the
*PRIMARY KEY* that is of type *INTEGER* in the "create table" string.
It does not have to be auto incrementing, but if it is not present,
the *SRCH2Engine* will not be able to generate a schema for the index
in the SRCH2 server. 
2. Any columns that are of type *TEXT* will be resolved as searchable
fields in the schema.  That is, the text
data the column contains will be searchable against search
input. Any columns that are of type *REAL* or *INTEGER* will be
resolved as refining fields.  They can be used in the advanced
search methods of the *Query* class to filter, order, and do other
advanced search parameterizations. Thus it is impossible to have any
column that will be resolved as a field that is both searchable and
refining.  If a column containing numerical data is to be searchable,
it must be duplicated as another column that is of type
*TEXT*. Columns of type *BLOB* will be ignored. 
3. Because the SRCH2 server directly connects to the database and uses
triggers to observe changes in the data content of a table to be
indexed, and the way Android utilizes SQLite, it is recommended that
all CRUD operations implemented by the *SQLiteOpenHelper* be wrapped
in "try catch" constructs catching the *SQLiteDatabaseLockedException*
and retrying the CRUD operation if the exception is caught. While this
is probably only necessary for very high-volume CRUD operations, it is
easy to implement:

```
public class DatabaseHelper extends SQLiteOpenHelper {

  ...

  public boolean insertRecords(ArrayList<Book> insertSet) {
    boolean insertSuccess = true;
	SQLiteDatabase db = getWritableDatabase();
	db.beginTransaction();
	try {
      // For simplicity, not using SQLiteStatement to do 
	  // more efficient inserts (recommended). 
	  
	  ContentValues cv = new ContentValues();
	  for (Book b : insertSet) {
		cv.clear();
		cv.put(DatabaseHelper.COLUMN_AUTHOR, b.mAuthor);
		cv.put(DatabaseHelper.COLUMN_TITLE, b.mTitle);
		cv.put(DatabaseHelper.COLUMN_GENRE, b.mGenre);
		cv.put(DatabaseHelper.COLUMN_YEAR, b.mYear);
		cv.put(DatabaseHelper.COLUMN_SCORE, b.mUserRating);
		cv.put(DatabaseHelper.COLUMN_THUMBNAIL,
				b.mThumbnail.getBytes(Charset.forName("UTF-8")));
		db.insert(TABLE_NAME, null, cv);
	  }
	  db.setTransactionSuccessful();
	} catch (SQLiteDatabaseLockedException oops) {
      insertSuccess = false;
	} finally {
	  db.endTransaction();
	}
	return insertSuccess;
  }
}
```
Now if the call to *insertRecords(ArrayList<Book> insertSet)* returns false, the operation can be reapplied. 

##Testing and Proguard

The SRCH2 Android SDK must be tested on a real Android device, not an emulator.
When we initialize the *SRCH2Engine* class,
you can call `setDebugAndTestMode(true)` to enable the *SRCH2Engine* to quickly
start and stop the SRCH2 server. 

To configure the SRCH2-Android-SDK for
[Proguard](http://developer.android.com/tools/help/proguard.html) to
shrink, optimize, and obfuscate the code, just add the following 
to the proguard configuration file:

```
-keep class com.srch2.** { *; } 
-keep interface com.srch2.** { *; } 
-keep enum com.srch2.** { *; } 
-dontwarn class com.srch2.** { *; } 
-dontwarn interface com.srch2.** { *; } 
-dontwarn enum com.srch2.** { *; } 
```


[//]: (##Using the Eclipse IDE	)
[//]: (##Using the IntelliJ IDE	)

