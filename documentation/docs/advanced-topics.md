
## Powerful Search API

The user can use the `Indexable#search(String searchInput)` to search the records that contains the searchInput.
This query will use fuzzy search all and the prefix search on the last term pattern to search on the index.

To enable the more powerful and flexable search capability, the user can use the `Indexable#advancedSearch(Query query)`.
User can set the certain attribute of that `Query` object to enable the filtering, sorting, paging or some other features.

Here are some samples:

Query q1 get the records with the word "terminator" in its *title* field and the word "cameron" in its *director* field
```
    SearchableTerm term1 = new SearchableTerm("terminator").searchSpecificField("title");
    SearchableTerm term2 = new SearchableTerm("cameron").searchSpecificField("director");
    Query q = new Query(term1.AND(term2));
```

Query q2 filters the results based on the year field, i.e., the year value needs to be between 1983 and 1992 (both inclusive).
```
    Query q = new Query(term1.AND(term2));
    q.filterByFieldInRange("year", "1983", "1992");
```

Query q3 filters the results based on the genre field. 
```
    Query q = new Query(term1.AND(term2));
    q.filterByFieldEqualsTo("genre", "action");
```

Query q4 sorts the results based on the year attribute in the ascending order.
```
    Query q = new Query(term1.AND(term2));
    q.sortOnFields("year").orderByAscending();
```

### SearchableTerm
The `SearchableTerm` is one basic search unit. It can be a single word or a group of words. It will be searched as a whole word.

#### Prefix Search
By default the `SearchableTerm` is a complete word. You can call `SearchableTerm#setIsPrefixMatching(boolean)` method to enable
or disable the prefix matching on that query string.

#### Fuzzy Search
By default the `SearchableTerm` is a correct word. You can call `SearchableTerm#enableFuzzyMatching(float)` method to enable the 
fuzzy search by specify the similarity of the mathing words, or call the `SearchableTerm#enableFuzzyMatching()` to use the default
similarity threshold specified in the `Indexable#getFuzzinessSimilarityThreshold()`

The fuzziness number is between 0 and 1. The bigger number means the matching keyword is more similar to the search word. 
If it set to one that means exact search. 

#### Boosting a Term

The engine provides the relevance level of matching records based on its matching terms. The user can call the `SearchableTerm#setBoostValue(int)`
to boost the importance of a given term. The higher the boost value, the more relevant the term will be.

Boosting allows control over the relevance of a record by boosting the significance of its terms. 
For example, if we are searching for two terms "star" and "wars", and we want the term "star" to be more relevant, 
we can call 
```
 new SearchableTerm("star").setBoostValue(4).AND (new SearchableTerm("wars")
```
The boost value must be a positive integer, and its default value is 1.

#### Boolean Operators
The engine supports three boolean operators: AND, OR, and AND_NOT. For example:
```
  new SearchableTerm("star").AND(new SearchableTerm("wars")).OR(new SearchableTerm("George Lucas"))
```
It means search all records which either contain "star" and "wars" or the phrase "George Lucas".

#### Search on the specified field

By default one search term can be search on all the `searchable` fields that defined in `Indexable#getSchema()`
A search term can be constrained on one specific field by calling the `SearchableTerm#searchSpecificField(String)`.
For example we can set the following search term.
```
  new SearchableTerm("wind").searchSpecificField("title")
```
This search term will only search for the keyword "wind" in the field *title*. 

### Query

The `Query` object is initialized by the `Term`. We can set the filter, sorter and some other operators to refine the returned query results.

#### Filter by range

The `Query#filterByFieldInRange`, `Query#filterByFieldEqualsTo`, `Query#filterByFieldStartFrom`, `Query#filterByFieldEndsTo` functions are used 
to specify a filter restricting the set of records returned, without influencing their scores. In the example below, only records having year equal to 2012 will return.
```
  new Query(new SearchableTerm("star")).filterByFieldEqualsTo("year", "2012")
```

A range query allows us to match records whose field value is between a specified lower bound and upper bound (both inclusive). 
```
  new Query(new SearchableTerm("star")).filterByFieldInRange("year", "2010", "2012")
```

One query can enable multiple filter. For example:
```
 new Query(new SearchableTerm("star"))
        .filterByFieldStartFrom("id","1000")
        .filterByFieldEqualsTo("genre","drama")
        .filterByFieldEqualsTo("year", "1975")
```
It returns the records with an *id* greater than or equal to 1000, a *genre* of drama, and *year* less than or equal to 1975.

**Note**, the engine supports only one kind of boolean operator (OR or AND) between all the filter terms. 
By default all the filter are of the *AND* relation.
User can call the `Query#setFilterRelationOR()` to set to the *OR* relation.

#### Sorting

The engine's default behavior is to sort the results using a descending order by the overall score of each record. 
The user can specify sorting by other fields. For example:
```
 new Query(new SearchableTerm("star")).sortOnFields("director","year","title") 
```
Note: if a sort request is included in the query, the returned records' scores are still those calculated scores by the engine, 
but the order of the results corresponds to the user specification.

#### Orderby
It specifies the order in which the result set should be sorted. Its default value is "desc" (descending).
User can call the `Query#orderByAscending()` and `Query#orderByDescending()` functions to set the corresponds orders.

#### Paging

We can set the start offset in the complete result set of the query. The default value is 0. 
The user can call the `Query#pagingStartFrom(10)` to show the query results that skips the first 10 records.

We can also set the number of records that returns for one Query. 
The user can all the `Query#pagingSize(20)` to set return 20 records once. The default size should be override in the 
`Indexable#getTopK()` function.

[//]: # (##GeoSearch)

## Testing

The SRCH2 Android SDK must be tested on a real device: the emulator does not support running the SRCH2 http server. When you initialize the `SRCH2Engine`, you can call `setDebugAndTestMode(true)` to enable the `SRCH2Engine` to quickly start and stop the SRCH2 http server. 

## Configuring for Proguard

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
	
