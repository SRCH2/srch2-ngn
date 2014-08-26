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

###Examples

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

###Search on Fields

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

###Prefix Condition

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

###Fuzzy Condition

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

###Term Boost

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
###Boolean Expression

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

###Filter by Range

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

###Sorting

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

###Pagination

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

###Geo Search

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
To search using a rectangle region, we can use the *Query.insideRectangleRegion()* method by
specifying the latitudes and the longitudes of the left-bottom location and the top-right location.
To search using a circle region, we can use the *Query.insideCircleRegion()* method by 
specifying the latitude and the longitude of the center point, and also the radius of the circle.

Here are some examples:
```
  new Query(new SearchableTerm("ghirardelli")).insideRectangleRegion(61.2, -149.9, 61.2, -149.7);
  new Query(new SearchableTerm("ghirardelli")).insideCircleRegion(61.2, -149.9, 5);
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
properly on the UI.  

The following code shows how to get the highlighted "title" field into the Adapter.
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
          sendMessage(Message
                  .obtain(this,
                          newResults.size() > 0 ? MESSAGE_WHAT_PUBLISH_NEW_RESULTS
                                  : MESSAGE_WHAT_PUBLISH_NO_NEW_RESULTS,
                          newResults));
      }
  }

```

##Custom Ranking
The SDK provides a *RecordBoostField* to determine each record's score when computing the relevance of
search results. This field will always be float in type, and should be set
from 1 to 100. 

The following code tells the SDK to take the value of the "recordBoost" field from the record
as the *RecordBoostField*. 
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
each record) we can assign the value of the recordBoost field for a record like the following

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
so that any movie of the genre "Science Fiction" will be boosted 50 times higher in the search results. 

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

