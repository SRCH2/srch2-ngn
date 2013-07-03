
Written by Chen Li in May 2012
Updated by Xiang An on June 1, 2012

This folder includes files to test the engine delivered to State Media.  
Make sure to run the scripts in the following order, since the third and
fourth steps will change the data in the search engine.

- For each test, you need to start the search engine using a command such as:
   ./path/to/srch2-search-server-${VERSION NUMBER} --config-file=/path/to/conf.ini
  
  Also, please use State Media's testing data

    - set primary key to "index"
    - set searchable attribute to "attr"

================================================================================
      How to run all the tests together automatically
================================================================================

- Make sure in the build/search-server/ folder, there is a binary srch2-search-server-{version}

- Run the following command:

      sh autotest.sh {version} 

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
