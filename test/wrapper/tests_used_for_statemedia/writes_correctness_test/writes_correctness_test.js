// read paths

if (process.argv.length != 4) {
  console.log('Usage: node writes_correctness_test.js [init_data_path] [insert_data_path]');
  process.exit(1);
}

// define global vars

var init_data_path = process.argv[2]
,   insert_data_path = process.argv[3]
    ;

var request = require("../node_modules/request")
,   fs = require('fs')
    ;

//var base_url = 'http://50.56.33.28:8081/'
var base_url = 'http://localhost:8082/'
//var base_url = 'http://localhost:8081/'
,   insert_base_url = base_url + 'docs'
,   delete_base_url = base_url + 'docs'
,   search_base_url = base_url + 'search'
    ;

var search_attr = 'attr'
,   primary_key = 'index'
,   limit = '10'
    ;

var invalid_queries_num = 0
,   step = 1
,   failed = 0
,   sizes_set = []
,   cursor = 0
,   failed_in_total = 0
    ;

var data
,   init_data_in_json = []
,   insert_data_in_json = []
    ;

var step_print_out = [ ''
                     , 'Search the initial data'
                     , 'Insert new data'
                     , 'Delete the initial data'
                     , 'Search the new data'
                     , 'Search the deleted initial data'
                     , 'Insert the inserted data again'
                     , 'Delete the deleted data again'
                     ]

// functions to make http requests

function insert_request(body_str) {
  return { method: 'PUT'
         , uri: insert_base_url 
         , body: body_str
         }
}

function delete_request(key_attr, key_value) {
  return { method: 'DELETE'
         , uri: delete_base_url + '?' + key_attr + '=' + key_value
         }
}

function search_request(queries_array) {
  return { method: 'GET'
         , uri: search_base_url + '?limit=' + limit + '&q=' + queries_array.join('+')
         }
}

function make_request(req, callback, expect_success, expected_key) {
  request (
    req
  , function (error, response, body) {
    if (error) {
      console.log(error);
    }
    else if (response.statusCode == 200){
      //console.log(body)
      callback(body, expect_success, expected_key);
    }
    else {
      console.log('error: ' + response.statusCode);
      console.log('uri: ' + req.uri);
      console.log(body);
      invalid_queries_num++;
      nextstep();
    }
  }
  );
}

function insert_callback(response, expect_success) {
  var response_in_json = JSON.parse(response);
  if (  (response_in_json['log'][0]['insert'] == 'failed' && expect_success)
     || (response_in_json['log'][0]['insert'] != 'failed' && !expect_success)
     )
    failed++;

  nextstep();
}

function delete_callback(response, expect_success) {
  var response_in_json = JSON.parse(response);
  if (  (response_in_json['log'][0]['delete'] == 'failed' && expect_success)
     || (response_in_json['log'][0]['delete'] != 'failed' && !expect_success)
     )
    failed++;

  nextstep();
}

function search_callback(response, expect_success, expected_key) {
  var response_in_json = JSON.parse(response);
  var found = false;
  for (var r in response_in_json.results) {
    if ( response_in_json.results[r] && response_in_json.results[r].record[primary_key] == expected_key ) {
      found = true;
      break;
    }
  }

  // if search for deleted records, then expect not to find them
  if (!expect_success)
    found = !found;

  if (!found) {
    failed++;
    //console.log(cursor);
  }

  nextstep();
}

function do_insert(body_str, expect_success) {
  var response = make_request( insert_request(body_str), insert_callback, expect_success );
}

function do_delete(key_attr, key_value, expect_success) {
  var response = make_request( delete_request(key_attr, key_value), delete_callback, expect_success );
}

function do_search(queries_array, expect_success, expected_key) {
  var response = make_request( search_request(queries_array), search_callback, expect_success, expected_key );
}

// function to read the data from files 

function read_data(init_data_path, insert_data_path) {
  var init_data
,     insert_data
      ;
  try {
    init_data = fs.readFileSync(init_data_path).toString().split('\n');
  } catch(e) {
    console.log("Error happens when read the init data file");
  }

  console.log("Init data reading completes.");

  try {
    insert_data = fs.readFileSync(insert_data_path).toString().split('\n');
  } catch(e) {
    console.log("Error happens when read the insert data file");
  }

  console.log("Insert data reading completes.");

  init_data.splice(init_data.length - 1, 1);
  insert_data.splice(insert_data.length - 1, 1);

  return { init: init_data
         , insert: insert_data
         }
}

// the test process

function nextstep() {
  cursor++;
  if (cursor == sizes_set[step]) {
    failed_in_total += failed;
    console.log("---------------------------------------");
    console.log('Failed: ' + failed);
    console.log('Invalid query: ' + invalid_queries_num);
    console.log('Total: ' + sizes_set[step]);
    step++;
    cursor = 0;
    failed = 0;
    invalid_queries_num = 0;

    if( step_print_out[step] ) {
      console.log("---------------------------------------");
      console.log('Step ' + step + ': ' + step_print_out[step] + '.')
      console.log("sleep 4 seconds ..");
      //setTimeout(stepon, 10000);
      setTimeout(stepon, 4000);
    }
    else
      stepon();
  }
  else
    stepon();
}

function stepon() {
  // search the initial data
  if (step == 1) {
    var json_object = JSON.parse(data.init[cursor]);
    init_data_in_json.push(json_object);
    do_search( json_object[search_attr].split(' '), true, json_object[primary_key] );
  }

  // insert new data
  else if (step == 2) {
    var json_object = JSON.parse(data.insert[cursor]);
    insert_data_in_json.push(json_object);
    do_insert( data.insert[cursor], true );
  }

  // delete the initial data
  else if (step == 3) {
    do_delete(primary_key, init_data_in_json[cursor][primary_key], true);
  }

  // search the new data
  else if (step == 4) {
    //console.log(cursor);
    do_search( insert_data_in_json[cursor][search_attr].split(' '), true, insert_data_in_json[cursor][primary_key] );
  }

  // search the deleted initial data
  else if (step == 5 ) {
    do_search( init_data_in_json[cursor][search_attr].split(' '), false, init_data_in_json[cursor][primary_key] );
  }

  // insert the inserted data again
  else if (step == 6 ) {
    do_insert( data.insert[cursor], false );
  }

  // delete the deleted data again
  else if (step == 7 ) {
    do_delete(primary_key, init_data_in_json[cursor][primary_key], false);
  }

  // no more tests
  else if (step == 8) {
    var total_query_num = 0;
    for ( var i=1; i< sizes_set.length; i++)
      total_query_num += sizes_set[i];

    var percentage = Math.round( failed_in_total / total_query_num * 100 );
    console.log("---------------------------------------");
    console.log("Failed percentage in total: " + percentage + "%.");
    console.log("==============================");
    if (percentage == 0)
        console.log("-------- Test Passes ---------");
    else
        console.log("-------- Test Fails ----------");
    console.log("==============================");
  }

  else
    console.log("Error: wrong step number!");
}

function run() {
  console.log("Data reading starts.");

  data = read_data(init_data_path, insert_data_path);
  init_data_records_num = data.init.length;
  insert_data_records_num = data.insert.length;

  console.log("Data reading completes.");
  console.log("Number of initial docs: " + init_data_records_num);
  console.log("Number of docs to insert: " + insert_data_records_num);


  sizes_set = [ 0
               , init_data_records_num
               , insert_data_records_num
               , init_data_records_num
               , insert_data_records_num
               , init_data_records_num
               , insert_data_records_num
               , init_data_records_num
               ]

  console.log("---------------------------------------");
  console.log('Step ' + step + ': ' + step_print_out[step] + '.')
  stepon()
}

// Start the test, cross your finger!
run();
