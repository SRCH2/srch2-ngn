#!/usr/bin/python
# This test is to make sure all of our returned responses are in a valid Json format
# It will test all the entry points ( e.g. '/search', '/docs', '/update' ...). 
# It will run the positive test and the negative test. In both cases, the engine should 
# return a well formatted Json response.

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

def testSearchResponds():
    print inspect.stack()[0][3]
    
    def testInvalidSearch():
        print inspect.stack()[0][3]
        response = test_lib.sendGetRequest('search')
        return check_keys(response, ['error'])

    def testValidSearch():
        print inspect.stack()[0][3]
        response = test_lib.searchRequest('q=men')
        return check_keys(response, ['estimated_number_of_results', \
                'fuzzy', 'limit', 'message', 'offset', 'payload_access_time', \
                'query_keywords', 'query_keywords_complete', 'results', 'results_found',\
                'searcher_time', 'type']) and \
              check_keys(response['results'][0], \
                  ['edit_dist', 'matching_prefix', 'record_id', 'score', 'record', 'snippet'])

    return testInvalidSearch() and testValidSearch()

def testSuggestResponds():
    print inspect.stack()[0][3]

    def testInvalidSuggest():
        print inspect.stack()[0][3]
        response = test_lib.sendGetRequest('suggest')
        return check_keys(response, ['error'])

    def testValidSuggest():
        print inspect.stack()[0][3]
        response = test_lib.sendGetRequest('suggest?k=man')
        return check_keys(response, ["message","payload_access_time","searcher_time","suggestions","suggestions_found"])


    return testInvalidSuggest() and testValidSuggest()

def testInfoResponds():
    print inspect.stack()[0][3]
    response = test_lib.infoRequest()
    return check_keys( response, ['engine_status', 'version']) and \
        check_keys( response['engine_status'], \
        ['doc_count','docs_in_index', 'last_merge', 'search_requests', 'write_requests'])

def testDocsResponds():
    print inspect.stack()[0][3]
    insert_doc = '{"category": "pet", "name": "Pet", "relevance": 7, "lat": 64.8, "lng": -147.7, "id": "2014"}'

    def testInsertResponds():
        print inspect.stack()[0][3]
        
        def testInsertSuccessResponds():
            print inspect.stack()[0][3]
            response = test_lib.insertRequest(insert_doc)
            return check_keys(response, ['log', 'message']) and \
                    check_keys(response['log'][0], ['insert', 'rid'])

        def testInsertFailResponds():

            print inspect.stack()[0][3]
            def testRepeatInsertFailResponds():
                print inspect.stack()[0][3]
                response = test_lib.insertRequest(insert_doc)
                return check_keys(response, ['log', 'message']) and \
                        check_keys(response['log'][0], ['insert','reason', 'rid'])

            def testInvalidJsonRecordResponds():
                
                print inspect.stack()[0][3]
                def testMalformedJsonRecord():
                    print inspect.stack()[0][3]
                    invalid_record = '{"category": "pet", "name": "Pet", "id": }'
                    response = test_lib.insertRequest(invalid_record)
                    # expected: {"message":"JSON object parse error"}
                    return check_keys(response, ['message'])

                def testNoPrimaryKeyProvided():
                    print inspect.stack()[0][3]
                    invalid_record = '{"category": "pet", "name": "Pet"}'
                    response = test_lib.insertRequest(invalid_record)
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
            response = test_lib.deleteRequest('id=2014')
            # expected: {"log":[{"delete":"success","rid":"2014"}],"message":"The delete was processed successfully"}
            return check_keys(response, ['log', 'message']) and\
                    check_keys(response['log'][0], ['delete', 'rid'])

        def testDeleteFailResponds():
            print inspect.stack()[0][3]
            response = test_lib.deleteRequest('id=2014')
            # expected: {"log":[{"delete":"failed","reason":"no record with given primary key","rid":"2014"}],"message":"The delete was processed successfully"}
            return check_keys(response, ['log', 'message']) and \
                    check_keys(response['log'][0], ['delete', 'reason', 'rid'])

        return testDeleteSuccessResponds() and testDeleteFailResponds()

    return testInsertResponds() and testDeleteResponds()

def testUpdateResponds():
    print inspect.stack()[0][3]

    def testUpdateSuccessResponds():

        print inspect.stack()[0][3]
        update_doc = '{"category": "pet", "name": "Pet", "relevance": 7, "lat": 64.8, "lng": -147.7, "id": "2014"}'

        def testUpsertRecord():
            print inspect.stack()[0][3]
            response = test_lib.updateRequeset(update_doc)
            # expected: {"log":[{"rid":"2014","update":"New record inserted successfully"}],"message":"The update was processed successfully"}
            return check_keys(response, ['log', 'message']) and \
                    check_keys(response['log'][0], ['rid', 'update'])

        def testUpdateExistedRecord():
            print inspect.stack()[0][3]
            response = test_lib.updateRequeset(update_doc)
            # expected: {"log":[{"rid":"2014","update":"Existing record updated successfully"}],"message":"The update was processed successfully"}
            return check_keys(response, ['log', 'message']) and \
                    check_keys(response['log'][0], ['rid', 'update'])

        return testUpsertRecord() and testUpdateExistedRecord()

    def testUpdateFailResponds():
        
        print inspect.stack()[0][3]
        def testMalformedJsonRecord():
            print inspect.stack()[0][3]
            record = '{"category": }'
            response = test_lib.updateRequeset(record)
            # expected: {"message":"JSON object parse error"}
            return check_keys(response, ['message'])

        def testNoPrimaryKeyProvided():
            print inspect.stack()[0][3]
            record = '{"category": "pet"}'
            response = test_lib.updateRequeset(record)
            return check_keys(response, ['log', 'message']) and \
                    check_keys(response['log'][0], ['reason', 'rid', 'update', 'details'])

        return testMalformedJsonRecord() and testNoPrimaryKeyProvided()

    return testUpdateFailResponds() and testUpdateSuccessResponds()

def testSaveResponds():
    print inspect.stack()[0][3]
    response = test_lib.saveRequest()
    # expected: {"log":[{"save":"success"}],"message":"The indexes have been saved to disk successfully"}
    return check_keys(response, ['log', 'message']) and \
            check_keys(response['log'][0], ['save'])

def testExportResponds():
    print inspect.stack()[0][3]

    def testInvalidExportRequest():
        response = test_lib.sendPutRequest('export')
        # {"error":"The request has an invalid or missing argument. See Srch2 API documentation for details."}
        return check_keys(response, ['error'])

    def testValidExport():
        response = test_lib.sendPutRequest('export?exported_data_file=./data/mydata.json')
        # {"log":[{"export":"success"}],"message":"The indexed data has been exported to the file ./data/abc/mydatax.json successfully."}
        return check_keys(response, ['log', 'message']) and \
                check_keys(response['log'][0], ['export'])

    return testInvalidExportRequest() and testValidExport()

def testResetLoggerResponds():
    print inspect.stack()[0][3]
    response =  test_lib.resetLoggerRequest()
    #{"log":[".////./data/exact_a1/log.txt"],"message":"The logger file repointing succeeded"}
    return check_keys(response, ['log', 'message'] )

def testSearchAllResponds():
    print inspect.stack()[0][3]

    def testInvalidSearch():

        print inspect.stack()[0][3]
        response = test_lib.sendGetRequest('search','_all')
        #{"__DEFAULTCORE__":{"error":"NOTICE : topK queryWARNING : After ignoring stop words no keyword is left to search."}}
        return check_keys(response, ['__DEFAULTCORE__']) and \
                check_keys(response['__DEFAULTCORE__'], ['error'])

    def testValidSearch():
        print inspect.stack()[0][3]
        response = test_lib.searchRequest('q=men','_all')
        return check_keys(response, ['__DEFAULTCORE__']) and \
                check_keys(response['__DEFAULTCORE__'], ['estimated_number_of_results', \
                    'fuzzy', 'limit', 'message', 'offset', 'payload_access_time', \
                    'query_keywords', 'query_keywords_complete', 'results', 'results_found',\
                    'searcher_time', 'type']) and \
                check_keys(response['__DEFAULTCORE__']['results'][0], \
                    ['edit_dist', 'matching_prefix', 'record_id', 'score', 'record', 'snippet'])

    return testInvalidSearch() and testValidSearch()

def testShutDownResponds():
    print inspect.stack()[0][3]

    def testInvalidShutdown():
        print inspect.stack()[0][3]
        response = test_lib.sendGetRequest('shutdown','_all')
        #{"error":"The request has an invalid or missing argument. See Srch2 API documentation for details."}
        return check_keys(response, ['error'])

    def testValidShutdown():
        print inspect.stack()[0][3]
        response = test_lib.shutdownRequest()
        # {"message":"Bye"}
        return check_keys(response, ['message'])

    return testInvalidShutdown() and testValidShutdown()

if __name__ == '__main__':
    binary_path = sys.argv[1]

    serverHandle = test_lib.startServer([binary_path, './json_response/conf.xml','./json_response/conf-A.xml','./json_response/conf-B.xml']) 
    if serverHandle == None:
        os._exit(-1)
    exit_code = 0;
    try :
        if  testSearchResponds() and \
            testSuggestResponds() and \
            testInfoResponds() and \
            testDocsResponds() and \
            testUpdateResponds() and \
            testSaveResponds() and \
            testExportResponds() and \
            testResetLoggerResponds() and \
            testSearchAllResponds() and \
            testShutDownResponds() :

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
