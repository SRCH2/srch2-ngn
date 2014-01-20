
how to run system test case:

/path/to/system/test/dir/main.sh /path/to/system/test/dir /path/to/srch2-engine-executable

if you are running it for the sytem test directory itself then 

./main.sh . /path/to/srch2-search-server



*** BELOW IS THE OLD CONTENT OF README.TXT WHICH IS MAINLY ABOUT STATEMEDIA TESTS ***


Written by Chen Li in May 2012
Updated by Xiang An on June 1, 2012
Updated by Huaijie Zhu on July 17,2013


There two parts of testing framwork in wrapper:



<<<<<<<<<<<<<<   Part 1         tests_used_for_statemedia    >>>>>>>>>>>>>>>>>>>>>>>>>>

This folder includes files to test the engine delivered to State Media.  
Make sure to run the scripts in the following order, since the third and
fourth steps will change the data in the search engine.

- For each test, you need to start the search engine using a command such as:
   ./path/to/srch2-search-server --config-file=/path/to/conf.ini
  
  Also, please use State Media's testing data

    - set primary key to "index"
    - set searchable attribute to "attr"

================================================================================
      How to run all the tests together automatically
================================================================================

- Make sure in the build/src/server/ folder, there is a binary srch2-search-server

- Run the following command:

      sh autotest.sh 

================================================================================
      How to run each test separately
================================================================================

- Step 1, run the "writes_correctness_test". 

  It's a script written by Xiang to test the correctness of the engine.
  It sends insert/delete requests to the server, and checks whether the
  results of new queries get the expected records or do not get the
  deleted records.

  1) Go to writes_correctness_test folder

  2) Start the engine using data/index.json_500_head as the initial index

  3) Edit writes_correctness_test.js to set the "base_url" to the correct value

  4) Run the test with the following command

       node writes_correctness_test.js data/index.json_500_head data/index.json_500_tail

- Step 2, run the "search_precision_test"

  It's a script provided by Soum to test the accuracy of the search engine.
  It includes a set of queries and their corresponding expected results.
  The script sends these queries to the engine, and compares the results
  with the expected results to evaluate the quality of the search engine.

  1) Go to search_precision_test folder

  2) Start the engine using State Media's 10M dataset as the inital index

  3) Edit large_insertion_test.rb to set the "host" to the correct value

  4) Run the test with following command
  
       node search_precision_test.js

- Step 3, run the "large_insertion_test".

  It's a script provided by Soum to test the update function of the engine.
  Specifically, it test the case where we use a very small data set as the
  inital index and then keep updating the index with large amount of data.

  1) Go to large_insertion_test folder

  2) Start the engine using data/index.json.mini as the initil index

  3) Edit large_insertion_test.rb to set the "base_uri" to the correct value

  4) put a big update.json in data folder

  5) Run the test with the following command

       ruby large_insertion_test.rb

- Step 4, run the "update_endpoint_test"

  It tests if the wrapper's update endpoint works properly

  1) Go to update_endpoint_test folder

  2) Start the engine using /data/index.json as the initial index

  3) Run the test with following command

       python update_endpoint_test.py

- Step 5, run the "jmeter_stress_test"

  We use Jmeter to do the stress test with the following script
  
    jmeter_stress_test/jmeter_stress_test.jmx
  
  We need to install Jmeter in order to run it.

- Step 6, use the front-end in srch2_ui to try the search experience.


<<<<<<<<<<<<<<   Part 2         tests_used_for_different_types_queries    >>>>>>>>>>>>>>>>>>>>>>>>>>

There are 9 kinds of queries tests.

1. Each test case has four files:  
	conf.ini ----The configuration file;
	data.json ---- The data file of records;
	queriesAndResults.txt ---- each line is in the format "query keywords||result_record_ids", e.g., "trust happy||1 3 6".
	A python (.py) file. Its main function is used to run the engine server, do queries, and check the validity of the query result. The following is the main flow of the python file:

		 #Start the engine server
			binary='../../../../build/src/server/srch2-search-server'
			binary=binary+' --config-file=conf.ini &'
			os.popen(binary)
		 #wait until the server is by calling "pingServer()";
		 #do queries and get their json results:
			response = urllib2.urlopen(query).read()
			response_json = json.loads(response)
		 #check the validity of the results
		        checkResult(query, response_json['results'], resultValue )
		 #kill the server process.

 2,  Run  each test separately..
     For example, to run the "exact_A1" test, we can do the following
          cd exact_A1  
          python exact_A1.py queriesAndResults.txt




If we run all the tests (both tests_used_for_statemedia and tests_used_for_different_types_queries )
   
    Run the following command:

      sh main.sh 
      
