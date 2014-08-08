
###Welcome

Welcome to the web repository of the SRCH2 Android SDK Documentation. 

![Welcome][Welcome]

Here you can find the SDK and related files to download under the [Download](download.md) section of this website. 

If you're new to the SRCH2 Android SDK and would like to get started using the power of the SRCH2 search engine in your app, you can read how to get it set-up and running by visiting the the [Hello SRCH2 Search Demo tutorial](tutorial.md). 

The [Advanced Topics](advanced-topics.md) section includes information on how to unlock the full power of the SRCH2 engine by using advanced features of the SRCH2 Android SDK API such as the `Query` class or geo-search. It also includes instructions for preparing your app for Progaurd when using the SRCH2 Android SDK, or how to set up the SDK for the Eclipse IDE. If you're curious as how the SRCH2 Android SDK works under the hood with your app in the Android system, you can also review the architecture and life-cycle diagrams in this section.

###Context
 
Here's a brief overview of what the SRCH2 Android SDK.
 
The SRCH2 Android SDK is powered by the SRCH2 Search engine. This search engine runs as a http server that is hosted internally by the SDK in an Android remote service (don't worry, you don't have to bind to it). Once running, you can manually do RESTful CRUD operations by forming the proper URLs (see the [Advanced Topics](advanced-topics.md) for more information), but the SDK comes with an API that enables you to do these same operations and interact with the running SRCH2 RESTful server through Java method calls. The SRCH2 Android SDK contains indexable objects, representing the indexes you will create and query, on which you can call methods such as insert, update, query, delete and info. In your code, you can use these objects by extending the `com.srch2.android.http.service.Indexable` class; besides defining the indexes themselves, most of these operations can be done by calling on the appropriate method of `com.srch2.android.http.service.SRCH2Engine`.
 
In addition, to receive output from the SRCH2 server, there are two asynchronous callbacks you should implement: the one you will certainly want to implement is `com.srch2.android.http.service.SearchResultsListener`. On completion of a query, this interface will pass you a map of search results to their originating index as well as the raw JSON response as sent from the SRCH2 http server. The other callback provides a set of state information callbacks, such as the result of an /info command performed on an index providing information about that index, or upon completion of an insert command notifying the success of having inserted records.
 
Finally, also included is set of classes giving you the power to unlock all the potential of the SRCH2 search engine (see the [Advanced Topics](advanced-topics.md) section for usage and examples): instant search, fuzzy matching, categorical faceting, boolean operators, per record and per record data fields ranking, local parametrization and filtering of the specific record data fields, and much much more. In fact--be sure to check out how to perform GeoSearches where you can include the longitude and latitude of the device so they are combined with the keywords a user might enter, to form the search query. But first to set up the SRCH2 Android SDK...

Click [here](http://www.srch2.com) to get back to the main SRCH2 page.

[welcome]: ../img/welcome.jpg "Welcome"