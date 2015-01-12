#using python fuzzy_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands,signal

sys.path.insert(0, 'srch2lib')
import test_lib

#the function of checking the results
def checkResult(query, responseJsonAll,resultValue):
    responseJson = responseJsonAll['results']
    isPass=1
    if  len(responseJson) == len(resultValue):
         for i in range(0, len(resultValue)):
                #print response_json['results'][i]['record']['id']
            if (resultValue.count(responseJson[i]['record']['id']) != 1):
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record']['id']+'||'+resultValue[i]
                break
    else:
        isPass=0
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
                 print responseJson[i]['record']['id']+'||'
            elif i >= len(responseJson):
                 print '  '+'||'+resultValue[i]
            else:
                 print responseJson[i]['record']['id']+'||'+resultValue[i]

    if isPass == 1:
        print  query+' test pass'
        return 0
    else:
        print  query+' test failed'
        return 1

# This test tests search by ID api
# 1. it searches for id=2 and it should find it
# 2. it searches for id=200 and it should not find it
# 3. it inserts id=200
# 4. it searches for id=200 and it finds it this time
# 5. it deletes id=2
# 6. it searches for id=2 and should not find it
# 7. it inserts id=2
# 8. it searches for id=2 and should find it again
def testNewFeatures( binary_path):
    # Start the engine server
    args = [ binary_path, './test_search_by_id/conf.xml', './test_search_by_id/conf-A.xml', './test_search_by_id/conf-B.xml' ]

    serverHandle = test_lib.startServer(args)
    if serverHandle == None:
        return -1

    ## first look for id=2
    print "#1: search for id=2 and should find it"
    query = 'docid=2'    
    # do the query
    response_json = test_lib.searchRequest(query)
    #check the result
    failCount = checkResult(query, response_json,['2'] )

    # second search for 200 which is not there    
    print "# 2. Search for id=200 and should not find it"
    query = 'docid=200'
    response_json = test_lib.searchRequest(query)
    #check the result
    # NOTE: If you inexplicably don't understand why this test fails and record 200 is found, try deleting
    # the index files.  They're probably leftover from a prior execution.
    failCount += checkResult(query, response_json,[] )

    # now insert 200
    print "# 3. inserts id=200"
    record = '{"model": "BMW","price":1.5,"likes":1,"expiration":"01/01/1911", "category": "second verycommonword vitamin Food & Beverages Retail Goods Specialty", "name": "Moondog Visions", "relevance": 8.0312880237855993, "lat": 61.207107999999998, "lng": -149.86541, "id": "200"}'
    jsonResponse = test_lib.insertRequest(record)
    if jsonResponse['items'][0]['status'] != True:
        print "Insertion of record 200 failed: " + response
        failCount += 1
    time.sleep(3)

    # third search for 200 which is there    
    print "# 4. search for id=200 and should find it this time"
    query = 'docid=200'
    response_json = test_lib.searchRequest(query)
    #check the result
    failCount += checkResult(query, response_json,['200'] )


    # now delete record 2
    print "# 5. delete id=2"
    deleteQuery = 'id=2'
    jsonResponse = test_lib.deleteRequest(deleteQuery)
    if jsonResponse['items'][0]['status'] != True:
        print "Deletion of record 2 failed: " + response
        failCount += 1
    time.sleep(3)

    # search for record 2 which should not be there
    print "# 6. search for id=2 and should not find it"
    query = 'docid=2'
    response_json = test_lib.searchRequest(query)
    #check the result
    failCount += checkResult(query, response_json,[] )


    # now insert 2
    print "# 7. insert id=2"
    record = '{"model": "BMW","price":1.5,"likes":1,"expiration":"01/01/1911", "category": "record 2 second verycommonword vitamin Food & Beverages Retail Goods Specialty", "name": "Moondog Visions", "relevance": 8.0312880237855993, "lat": 61.207107999999998, "lng": -149.86541, "id": "2"}'
    jsonResponse = test_lib.insertRequest(record)
    if jsonResponse['items'][0]['status'] != True:
        print "Insertion of record 2 failed: " + response
        failCount += 1
    time.sleep(3)

    # third search for 200 which is there    
    print "# 8. searches for id=2 and should find it again"
    query = 'docid=2'
    response_json = test_lib.searchRequest(query)

    #check the result
    failCount += checkResult(query, response_json,['2'] )
    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount

if __name__ == '__main__':    
   #Path of the query file
   #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
   binary_path = sys.argv[1]
   exitCode = testNewFeatures(binary_path)
   os._exit(exitCode)

