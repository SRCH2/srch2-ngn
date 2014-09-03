#!/usr/bin/python
# This test is to make sure all of our returned responses is in a valid Json format
# It will test all the open path ( e.g. '/search', '/docs', '/update' ...). 
# It will run the positive test and the negative test. In both cases, the engine should 
# return a well formated Json response.

import sys, urllib2, json, time, subprocess, os, commands, signal
import inspect
sys.path.insert(0, 'srch2lib')
import test_lib

def check_keys( json, keys):
    for key in keys:
        if not json.has_key(key):
            print >> sys.stderr, 'missing key:', key , 'inside this json object:', json
            return False
    return True

def open_url_get(url):
    try:
        return json.loads(urllib2.urlopen(url).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())
    
def open_url_put(url, record):
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, record)
        request.get_method = lambda: 'PUT'
        return json.loads( opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

def open_url_delete(url):
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, '')
        request.get_method = lambda: 'DELETE'
        return json.loads(opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

def testSearchResponds(host_url):
    search_url = host_url + '/search'
    print inspect.stack()[0][3]
    
    def testInvalidSearch():
        print inspect.stack()[0][3]
        response = open_url_get(search_url)
        return check_keys(response, ['error'])

    def testValidSearch():
        print inspect.stack()[0][3]
        response = open_url_get(search_url + '?q=men')
        return check_keys(response, ['estimated_number_of_results', \
                'fuzzy', 'limit', 'message', 'offset', 'payload_access_time', \
                'query_keywords', 'query_keywords_complete', 'results', 'results_found',\
                'searcher_time', 'type']) and \
              check_keys(response['results'][0], \
                  ['edit_dist', 'matching_prefix', 'record_id', 'score', 'record', 'snippet'])

    return testInvalidSearch() and testValidSearch()

def testSuggestResponds(host_url):
    suggest_url = host_url +  '/suggest'
    print inspect.stack()[0][3]

    def testInvalidSuggest():
        print inspect.stack()[0][3]
        response = open_url_get(suggest_url)
        return check_keys(response, ['error'])

    def testValidSuggest():
        print inspect.stack()[0][3]
        response = open_url_get(suggest_url + "?k=man")
        return check_keys(response, ["message","payload_access_time","searcher_time","suggestions","suggestions_found"])


    return testInvalidSuggest() and testValidSuggest()

def testInfoResponds(host_url):
    info_url = host_url + '/info'
    print inspect.stack()[0][3]
    response = open_url_get(info_url)
    return check_keys( response, ['engine_status', 'version']) and \
        check_keys( response['engine_status'], \
        ['doc_count','docs_in_index', 'last_merge', 'search_requests', 'write_requests'])

def testDocsResponds(host_url):
    docs_url = host_url + '/docs'
    print inspect.stack()[0][3]
    insert_doc = '{"category": "pet", "name": "Pet", "relevance": 7, "lat": 64.8, "lng": -147.7, "id": "2014"}'

    def testInsertResponds():
        print inspect.stack()[0][3]
        
        def testInsertSuccessResponds():
            print inspect.stack()[0][3]
            response = open_url_put(docs_url, insert_doc)
            return check_keys(response, ['log', 'message']) and \
                    check_keys(response['log'][0], ['insert', 'rid'])

        def testInsertFailResponds():

            print inspect.stack()[0][3]
            def testRepeatInsertFailResponds():
                print inspect.stack()[0][3]
                response = open_url_put(docs_url, insert_doc)
                return check_keys(response, ['log', 'message']) and \
                        check_keys(response['log'][0], ['insert','reason', 'rid'])

            def testInvalidJsonRecordResponds():
                
                print inspect.stack()[0][3]
                def testMalformedJsonRecord():
                    print inspect.stack()[0][3]
                    invalid_record = '{"category": "pet", "name": "Pet", "id": }'
                    response = open_url_put(docs_url, invalid_record)
                    # expected: {"message":"JSON object parse error"}
                    return check_keys(response, ['message'])

                def testNoPrimaryKeyProvided():
                    print inspect.stack()[0][3]
                    invalid_record = '{"category": "pet", "name": "Pet"}'
                    response = open_url_put(docs_url, invalid_record)
                    # expected: {"log":[{"details":"No primary key found.","insert":"failed","reason":"parse: The record is not in a correct json format","rid":""}],"message":"..."}
                    return check_keys(response, ['log', 'message']) and \
                        check_keys(response['log'][0], ['insert','reason', 'details', 'rid'])

                return testMalformedJsonRecord() and testNoPrimaryKeyProvided()

            return testRepeatInsertFailResponds() and testInvalidJsonRecordResponds()


        return testInsertSuccessResponds() and testInsertFailResponds()

    def testDeleteResponds():
        print inspect.stack()[0][3]

        def testDeleteSuccessResponds():
            print inspect.stack()[0][3]
            response = open_url_delete(docs_url + '?id=2014')
            # expected: {"log":[{"delete":"success","rid":"2014"}],"message":"The delete was processed successfully"}
            return check_keys(response, ['log', 'message']) and\
                    check_keys(response['log'][0], ['delete', 'rid'])

        def testDeleteFailResponds():
            print inspect.stack()[0][3]
            response = open_url_delete(docs_url + '?id=2014')
            # expected: {"log":[{"delete":"failed","reason":"no record with given primary key","rid":"2014"}],"message":"The delete was processed successfully"}
            return check_keys(response, ['log', 'message']) and \
                    check_keys(response['log'][0], ['delete', 'reason', 'rid'])

        return testDeleteSuccessResponds() and testDeleteFailResponds()

    return testInsertResponds() and testDeleteResponds()

def testUpdateResponds(host_url):
    update_url = host_url + '/update'
    print inspect.stack()[0][3]

    def testUpdateSuccessResponds():

        print inspect.stack()[0][3]
        update_doc = '{"category": "pet", "name": "Pet", "relevance": 7, "lat": 64.8, "lng": -147.7, "id": "2014"}'

        def testUpsertRecord():
            print inspect.stack()[0][3]
            response = open_url_put(update_url, update_doc)
            # expected: {"log":[{"rid":"2014","update":"New record inserted successfully"}],"message":"The update was processed successfully"}
            return check_keys(response, ['log', 'message']) and \
                    check_keys(response['log'][0], ['rid', 'update'])

        def testUpdateExistedRecord():
            print inspect.stack()[0][3]
            response = open_url_put(update_url, update_doc)
            # expected: {"log":[{"rid":"2014","update":"Existing record updated successfully"}],"message":"The update was processed successfully"}
            return check_keys(response, ['log', 'message']) and \
                    check_keys(response['log'][0], ['rid', 'update'])

        return testUpsertRecord() and testUpdateExistedRecord()

    def testUpdateFailResponds():
        
        print inspect.stack()[0][3]
        def testMalformedJsonRecord():
            print inspect.stack()[0][3]
            record = '{"category": }'
            response = open_url_put( update_url, record)
            # expected: {"message":"JSON object parse error"}
            return check_keys(response, ['message'])

        def testNoPrimaryKeyProvided():
            print inspect.stack()[0][3]
            record = '{"category": "pet"}'
            response = open_url_put( update_url, record)
            return check_keys(response, ['log', 'message']) and \
                    check_keys(response['log'][0], ['reason', 'rid', 'update', 'details'])

        return testMalformedJsonRecord() and testNoPrimaryKeyProvided()

    return testUpdateFailResponds() and testUpdateSuccessResponds()

def testSaveResponds(host_url):
    print inspect.stack()[0][3]
    save_url = host_url + '/save'
    response = open_url_put(save_url, '')
    # expected: {"log":[{"save":"success"}],"message":"The indexes have been saved to disk successfully"}
    return check_keys(response, ['log', 'message']) and \
            check_keys(response['log'][0], ['save'])

def testExportResponds(host_url):
    print inspect.stack()[0][3]
    export_url = host_url + '/export'

    def testInvalidExportRequest():
        response = open_url_put(export_url, '')
        # {"error":"The request has an invalid or missing argument. See Srch2 API documentation for details."}
        return check_keys(response, ['error'])

    def testValidExport():
        response = open_url_put(export_url + '?exported_data_file=./data/mydata.json', '')
        # {"log":[{"export":"success"}],"message":"The indexed data has been exported to the file ./data/abc/mydatax.json successfully."}
        return check_keys(response, ['log', 'message']) and \
                check_keys(response['log'][0], ['export'])

    return testInvalidExportRequest() and testValidExport()

def testResetLoggerResponds(host_url):
    print inspect.stack()[0][3]
    reset_url = host_url + '/resetLogger'
    response = open_url_put(reset_url, '')
    #{"log":[".////./data/exact_a1/log.txt"],"message":"The logger file repointing succeeded"}
    return check_keys(response, ['log', 'message'] )

def testSearchAllResponds(host_url):
    print inspect.stack()[0][3]
    search_all_url = host_url +  '/_all/search'

    def testInvalidSearch():

        print inspect.stack()[0][3]
        response = open_url_get(search_all_url)
        #{"__DEFAULTCORE__":{"error":"NOTICE : topK queryWARNING : After ignoring stop words no keyword is left to search."}}
        return check_keys(response, ['__DEFAULTCORE__']) and \
                check_keys(response['__DEFAULTCORE__'], ['error'])

    def testValidSearch():
        print inspect.stack()[0][3]
        response = open_url_get(search_all_url + '?q=men')
        return check_keys(response, ['__DEFAULTCORE__']) and \
                check_keys(response['__DEFAULTCORE__'], ['estimated_number_of_results', \
                    'fuzzy', 'limit', 'message', 'offset', 'payload_access_time', \
                    'query_keywords', 'query_keywords_complete', 'results', 'results_found',\
                    'searcher_time', 'type']) and \
                check_keys(response['__DEFAULTCORE__']['results'][0], \
                    ['edit_dist', 'matching_prefix', 'record_id', 'score', 'record', 'snippet'])

    return testInvalidSearch() and testValidSearch()

def testShutDownResponds(host_url):
    print inspect.stack()[0][3]
    shutdown_url = host_url + '/_all/shutdown'

    def testInvalidShutdown():
        print inspect.stack()[0][3]
        response = open_url_get(shutdown_url)
        #{"error":"The request has an invalid or missing argument. See Srch2 API documentation for details."}
        return check_keys(response, ['error'])

    def testValidShutdown():
        print inspect.stack()[0][3]
        response = open_url_put( shutdown_url,'')
        # {"message":"Bye"}
        return check_keys(response, ['message'])

    return testInvalidShutdown() and testValidShutdown()

if __name__ == '__main__':
    binary_path = sys.argv[1]
    conf = './json_response/conf.xml'
    port = test_lib.detectPort(conf)
    if test_lib.confirmPortAvailable(port) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        os._exit(-1)

    serverHandle = test_lib.startServer([binary_path, '--config=' + conf]) 
    host_url = 'http://127.0.0.1:' + str(port) 
    test_lib.pingServer(port)
    print 'starting engine: ' + host_url + ' ' + conf
    exit_code = 0;
    try :
        if  testSearchResponds(host_url) and \
            testSuggestResponds(host_url) and \
            testInfoResponds(host_url) and \
            testDocsResponds(host_url) and \
            testUpdateResponds(host_url) and \
            testSaveResponds(host_url) and \
            testExportResponds(host_url) and \
            testResetLoggerResponds(host_url) and \
            testSearchAllResponds(host_url) and \
            testShutDownResponds(host_url) :

            exit_code = 0
        else:
            exit_code = -1
    except Exception, e:
        print 'ERROR:', e
        exit_code = -1
    finally:
        test_lib.killServer(serverHandle)  
        if exit_code == 0:
            print '\033[92m'+ "All passed", '\033[0m'
        else:
            print '\033[91m'+ "Failed", '\033[0m'
        os._exit(exit_code)
