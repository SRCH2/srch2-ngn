In this documentation we explain advanced features of the SRCH2 Android SDK,
such as how to formulate a query with various conditions, how to use
the SDK to do testing and use Proguard.

##Advanced Queries

In the [basic tutorial](index.md), we showed how to send a search
*beaty ame* to the SRCH2 engine.  By default, the engine uses the
space delimiter to tokenize the string to multiple keywords ('beaty'
and 'ame').  It treats the last keyword ('ame') as a prefix condition,
and other keywords (e.g., 'beaty') as complete keywords.  By default,
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

<h1> Give an example to show how to specify multiple attributes for a
term. </h1> 

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
    SearchableTerm term = new SearchableTerm("spielburrg").enableFuzzyMatching(0.8f);
```
The internal edit-distance threshold is `floor((1-0.8) * length("spielburrg")) =
floor(0.2 * 10) = 2`. So the engine will find records with a keyword whose edit
distance to the term "spielburrg" is at most 2.

If we don't provide a similarity threshold, e.g.,
```
    SearchableTerm term = new SearchableTerm("spielburrg").enableFuzzyMatching();
```
the engine will use the similarity threshold specified in the
*Indexable.getFuzzinessSimilarityThreshold()*.  If we don't
override this getter method, we will use the system's default threshold
of 0.65f.

####Term Boost

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
  new  SearchableTerm("sta").enableFuzzyMatching(0.6f).setBoostValue(4).setIsPrefixMatching(true);
```
###Boolean Expression

The engine supports three boolean operators: AND, OR, and
AND_NOT. Each operator generates a *CompositeTerm*
object. Both *SearchableTerm* and *CompositeTerm* are inherited from
the *Term* class.  The *Term* class is used to initialize the *Query*
object. 

<h1> Why cannot we change "AND_NOT" just to "NOT"? </h1>

For example,
```
 new SearchableTerm("star").AND(new SearchableTerm("wars")).OR(new SearchableTerm("George Lucas"));
```
The query means searching records which either contain "star" and "wars", or the phrase "George Lucas".
The following are a few more examples:
```
 new SearchableTerm("star wars").AND(new SearchableTerm("episode 3"))
   .OR(new SearchableTerm("George Lucas").AND_NOT(new SearchableTerm("Indiana Jones")));

 new SearchableTerm("big").AND(new SearchableTerm("fish"))
   .AND((new SearchableTerm("Tim Burton").OR(new SearchableTerm("MacGregor").enableFuzzyMatching(0.5f))));
```

###Filter by Range

The *filterByFieldInRange*, *filterByFieldEqualsTo*, *filterByFieldStartFrom*,
*filterByFieldEndsTo* methods are used to specify a filter restricting the set
of records returned.  In the example below,
only records having year equal to 2012 will return.  
```
    new Query(new SearchableTerm("star")).filterByFieldEqualsTo("year", "2012");
```

A range query allows us to match records whose field value is between
a specified lower bound and upper bound (both inclusive).  For example:
```
  new Query(new SearchableTerm("star")).filterByFieldInRange("year", "2010", "2012");
```

A query can have multiple filters. For example:
```
  new Query(new SearchableTerm("star"))
      .filterByFieldStartFrom("id","1000")
      .filterByFieldEqualsTo("genre","drama")
      .filterByFieldEqualsTo("year", "1975");
```
It returns the records with an *id* greater than or equal to 1000, a *genre* of drama, and *year* less than or equal to 1975.

Note that the engine supports only one kind of boolean operator (OR or
AND) between all the filter conditions.  By default, all the filters
are connected by the *AND* operator. We can call the
*setFilterRelationOR()* method to set it to the *OR* operator. 

###Sorting

By default the engine sorts the results using a descending order by
the overall score of each record.  We can specify sorting by other
fields. For example, the following query sorts the results based on
the *director*, *year*, and *title* fields:  
```
  new Query(new SearchableTerm("star")).sortOnFields("director","year","title");
```

Note that if a sort request is included in the query, the returned
records' scores are still those calculated scores by the engine,  
but the order of the results corresponds to the specified fields.

We can specify the order in which the result set should be sorted. The default
behavior is to sort them in the descending order. We can call the
*orderByAscending()* method to sort them in the ascending order.

As in the previous example, we can use the following code to let results
be returned in the ascending order on the *year* field, 
```
  new Query(new SearchableTerm("star")).sortOnFields("director","year","title").orderByAscending();
```

###Pagination

We can specify a range of records in the result set to be returned,
including an offet and number of records.   The default offset value is 0. 
We can call the *pagingStartFrom(10)* method to return the query
records that start from the 10th record. 

We can also set the number of records to be returned by 
calling the *pagingSize()* method. By default the
number is taken from the  *Indexable.getTopK()* method. If we
do not override this getter method, the engine will take 10 as the
default value.

These two methods can be used to implement pagination.
For example, the following code returns records that are ranked from
the 25th to the 34th of all the results. 
```
  new Query(new SearchableTerm("star")).pagingStartFrom(25).pagingSize(10);
```

##Testing and Proguard

The SRCH2 Android SDK must be tested on a real Android device, not an
When we initialize the `SRCH2Engine` class,
you can call `setDebugAndTestMode(true)` to enable the `SRCH2Engine` to quickly
start and stop the SRCH2 http server. 

To configure the SRCH2-Android-SDK for Proguard, just add the following
to your proguard configuration file:

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
	
