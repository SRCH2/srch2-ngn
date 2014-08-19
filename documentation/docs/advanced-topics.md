
In this documentation we explain advanced features of the SRCH2 Android SDK,
such as how to formulate a sophisticated query, how to use the SDK to do
testing, and how to use Proguard.

##Advanced Queries

In the basic tutorial, we showed how to send a search *american beau* to the
SRCH2 engine.  By default, the engine uses the space delimiter to tokenize the
string to multiple keywords.  It treats the last keyword *beau* as a prefix
keyword, and other keywords (e.g., *american*) as complete keywords.  For each
keyword, the engine allows one typo for every three characters.  

Often you may want to have more control on the terms in a query, such as prefix
versus complete keyword and their fuzziness.  The SDK allows you to have this
control by customize the *Query* object and then search by the
*Indexable.advancedSearch(Query)* method.  
We use the following examples to explain.  Suppose we have defined two query
terms. Term 1 specifies a condition that the word "terminator" has to appear in
the <i>title</i> field; and term 2 says that the word "cameron" needs to be in
the <i>director</i> field.

```
    SearchableTerm term1 = new SearchableTerm("terminator").searchSpecificField("title");
    SearchableTerm term2 = new SearchableTerm("cameron").searchSpecificField("director");
```

<li> Query q1 finds records with the word "terminator" in its <i>title</i> field and the word "cameron" in its <i>director</i> field:</li>

```
    Query q1 = new Query(term1.AND(term2));
```

<li> Query q2 filters the results based on the year field, i.e., the year value needs to be between 1983 and 1992 (both inclusive).</li>

```
    Query q2 = new Query(term1.AND(term2));
    q2.filterByFieldInRange("year", "1983", "1992");
```

<li> Query q3 filters the results by requiring that the genre value is
"action". </li>
```
    Query q3 = new Query(term1.AND(term2));
    q3.filterByFieldEqualsTo("genre", "action");
```

<li> Query q4 sorts the results based on the year attribute in the ascending order.</li>
```
    Query q4 = new Query(term1.AND(term2));
    q4.sortOnFields("year").orderByAscending();
```

###SearchableTerm

We use the *SearchableTerm* class to specify keyword conditions to
define an arbitrary boolean expression.

####Search on the specified field

By default one search term can search on all the *searchable* fields that
defined in the *Indexable.getSchema()* method. A search term can be
confined to only search on one specific field by calling the *searchSpecificField()* method.

For example we can set the following search term to only search for the keyword "wind" in the field *title*.
```
  new SearchableTerm("wind").searchSpecificField("title");
```

####Prefix Condition

We can call its member method *setIsPrefixMatching()* to enable or disable
prefix matching on this keyword. By default, it is "false", i.e., a
*SearchableTerm* object is treated as a complete-word condition.

Take *american beau* search term as an example, the following code sets the "american" as
the complete condition and the "beau" as the prefix condition, 
```
    SearchableTerm term1 = new SearchableTerm("american").setIsPrefixMatching(false);
    SearchableTerm term2 = new SearchableTerm("beau").setIsPrefixMatching(true);
    Query query = new Query(term1.AND(term2));
```

####Fuzzy Search

We can call its member method *enableFuzzyMatching()* to enable the fuzzy
match condition. User can also pass the specific fuzziness similarity
threshold into that method. By default, it is "disabled", i.e., a *SearchableTerm* 
object is treated as a exact-word condition.

The engine supports fuzzy search based on [Levenshtein distance (edit distance)](http://en.wikipedia.org/wiki/Levenshtein_distance).
We use a customizable threshold (normalized based on the term length) to
determine the edit distance used in finding the set of matching keywords.  The
*Similarity Threshold* is a normalized value (a float number between 0 and 1)
for a term.
Let "s" be the similarity threshold for a term given in the query. The engine
will use the formula
```
     floor((1-s) * length(keyword))
```
to compute the edit-distance threshold to do the search. If "s" is 1, we do an
exact search. As example, consider the following query:
```
    SearchableTerm term = new SearchableTerm("spielburrg").enableFuzzyMatching(0.8f);
```
The internal edit-distance threshold is `floor((1-0.8) * length("spielburrg")) =
floor(0.2 * 10) = 2`. So the engine will find records with a keyword whose edit
distance to the term "spielburrg" is at most two.

If we don't pass the similarity threshold, e.g.,
```
    SearchableTerm term = new SearchableTerm("spielburrg").enableFuzzyMatching();
```
the engine will use the similarity threshold specified in the *Indexable.getFuzzinessSimilarityThreshold()*. 
If user doesn't override that getter method, we will use the system default threshold of 0.65f.

####Boosting a Term

The engine provides the relevance level of matching records based on its
matching terms. The user can call the *setBoostValue(int)* method to
boost the importance of a given term. The boost value is a integer from 1 to
100. The higher the boost value, the more relevant the term will be. The default value is 1.

Boosting allows control over the relevance of a record by boosting the
significance of its terms.  For example, if we are searching for two terms
"star" and "wars", and we want the term "star" to be more relevant, we can call 
```
    new SearchableTerm("star").setBoostValue(4).AND (new SearchableTerm("wars"));
```

We can specify prefix, fuzziness and boosting condition to a single term, e.g.,  
```
    new  SearchableTerm("sta").enableFuzzyMatching(0.6f).setBoostValue(4).setIsPrefixMatching(true);
```

####Boolean Operators
The engine supports three boolean operators: AND, OR, and AND_NOT. Each of the operation will generate a
*CompositeTerm* object. Both *SearchableTerm* and *CompositeTerm* are inherited from the *Term* class. 
The *Term* class is used to initialize the *Query* object.

For example,
```
    new SearchableTerm("star").AND(new SearchableTerm("wars")).OR(new SearchableTerm("George Lucas"));
```
it means search all records which either contain "star" and "wars" or the phrase "George Lucas".

It can be used with a boolean expression. A few more examples are:
```
    new SearchableTerm("star wars").AND( new SearchableTerm("episode 3"))
      .OR( new SearchableTerm("George Lucas").AND_NOT( new SearchableTerm("Indiana Jones")));

    new SearchableTerm("big").AND(new SearchableTerm("fish"))
      .AND((new SearchableTerm("Tim Burton").OR(new SearchableTerm("MacGregor").enableFuzzyMatching(0.5f))));
```


###Query

We can use the *Query* object to enable the advanced search features by calling
the *Indexable.advancedSearch(Query query)* method.  The *Query* object is
constructed by the *Term* object. It can be a *SearchableTerm* or a
*CompositeTerm* that composed by using the boolean operators on the multiple
*SearchableTerm*s. 

All the conditions that set on the *Term* will be taken into the *Query* object.
In extra, it allows us to set the filter, sorter and some other operators to
refine the returned query results.

####Filter by range

The *filterByFieldInRange*, *filterByFieldEqualsTo*, *filterByFieldStartFrom*,
*filterByFieldEndsTo* methods are used to specify a filter restricting the set
of records returned, without influencing their scores.  In the example below,
only records having year equal to 2012 will return.  
```
    new Query(new SearchableTerm("star")).filterByFieldEqualsTo("year", "2012");
```

A range query allows us to match records whose field value is between a specified lower bound and upper bound (both inclusive). 
```
    new Query(new SearchableTerm("star")).filterByFieldInRange("year", "2010", "2012");
```

One query can have multiple filters. For example:
```
    new Query(new SearchableTerm("star"))
        .filterByFieldStartFrom("id","1000")
        .filterByFieldEqualsTo("genre","drama")
        .filterByFieldEqualsTo("year", "1975");
```
It returns the records with an *id* greater than or equal to 1000, a *genre* of drama, and *year* less than or equal to 1975.

**Note**, the engine supports only one kind of boolean operator (OR or AND) between all the filter conditions. 
By default all the filters are connected by the *AND* relation.
We can call the *setFilterRelationOR()* method to set it to the *OR* relation.

####Sorting

The engine's default behavior is to sort the results using a descending order by the overall score of each record. 
We can specify sorting by other fields. For example:
```
    new Query(new SearchableTerm("star")).sortOnFields("director","year","title");
```

**Note**: if a sort request is included in the query, the returned records' scores are still those calculated scores by the engine, 
but the order of the results corresponds to the user specification.

####Order
It specifies the order in which the result set should be sorted. The default
behavior is to sort them in the descending order. We can call the
*orderByAscending()* method to sort them in the ascending order.

As in the previous example, we can use the following code to let results
return in ascending order on the *year* field, 
```
    new Query(new SearchableTerm("star")).sortOnFields("director","year","title").orderByAscending();
```

####Pagination

We can set the offset of the complete results of the Query. The default value is 0. 
We can call the *pagingStartFrom(10)* method to return the query records that start from the 10th.

We can also set the number of records that returns at once for one Query. 
We can call the *pagingSize(20)* method to return 20 records. By default the
number is taken from the  *Indexable.getTopK()* method. If the user doesn't
override this getter method, the engine will take 10 as the system default
value.

These two method can be used to implement pagination.
For example, the following code will get the records that ranked from the 25th to the 34th of overall results.
```
    new Query(new SearchableTerm("star")).pagingStartFrom(25).pagingSize(10);
```

[//]: # (##GeoSearch)

##Testing

The SRCH2 Android SDK must be tested on a real device: the emulator does not
support running the SRCH2 http server. When you initialize the `SRCH2Engine`,
you can call `setDebugAndTestMode(true)` to enable the `SRCH2Engine` to quickly
start and stop the SRCH2 http server. 

##Configuring for Proguard

Configuring the SRCH2-Android-SDK for Proguard is easy. Just add:

```
-keep class com.srch2.** { *; } 
-keep interface com.srch2.** { *; } 
-keep enum com.srch2.** { *; } 
-dontwarn class com.srch2.** { *; } 
-dontwarn interface com.srch2.** { *; } 
-dontwarn enum com.srch2.** { *; } 
```

to your proguard configuration file.

[//]: (##Using the Eclipse IDE	)
[//]: (##Using the IntelliJ IDE	)
	
