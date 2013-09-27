#using python fuzzy_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands,signal

port = '8081'

#make sure that start the engine up
def pingServer():
    info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'
    while os.system(info) != 0:
        time.sleep(1)
        info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'

#the function of checking the results
def checkResult(query, responseJsonAll,resultValue):
    responseJson = responseJsonAll['results']
    isPass=1
    if  len(responseJson) == len(resultValue):
         for i in range(0, len(resultValue)):
                #print response_json['results'][i]['record']['id']
            if responseJson[i]['record']['id'] !=  resultValue[i]:
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
    else:
        print  query+' test failed'

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
    binary= binary_path + '/srch2-search-server'
    binary= binary+' --config-file=./test_search_by_id/conf.xml &'
    print 'starting engine: ' + binary 
    os.popen(binary)
    #make sure that start the engine up
    pingServer()

    ## first look for id=2
    query = 'http://localhost:' + port + '/search?docid=2'    
    # do the query
    response = urllib2.urlopen(query).read()
    response_json = json.loads(response)
    #check the result
    checkResult(query, response_json,['2'] )

    # second search for 200 which is not there    
    query = 'http://localhost:' + port + '/search?docid=200'
    # do the query
    response = urllib2.urlopen(query).read()
    response_json = json.loads(response)
    #check the result
    checkResult(query, response_json,[] )

    # now insert 200
    insertCommand = 'curl "http://localhost:'+port+'/docs" -i -X PUT -d \'{"model": "BMW","price":1.5,"likes":1,"expiration":"01/01/1911", "category": "second verycommonword vitamin Food & Beverages Retail Goods Specialty", "name": "Moondog Visions", "relevance": 8.0312880237855993, "lat": 61.207107999999998, "lng": -149.86541, "id": "200"}\''
    print insertCommand
    os.system(insertCommand)

    time.sleep(10)
    # third search for 200 which is there    
    query = 'http://localhost:' + port + '/search?docid=200'
    # do the query
    response = urllib2.urlopen(query).read()
    response_json = json.loads(response)
    #check the result
    checkResult(query, response_json,['200'] )


    # now delete record 2
    deleteCommand = 'curl "http://localhost:8081/docs?id=2" -i -X DELETE'
    print deleteCommand
    os.system(deleteCommand)
    time.sleep(10)

    # search for record 2 which should not be there
    query = 'http://localhost:' + port + '/search?docid=2'
    # do the query
    response = urllib2.urlopen(query).read()
    response_json = json.loads(response)
    #check the result
    checkResult(query, response_json,[] )


    # now insert 2
    insertCommand = 'curl "http://localhost:'+port+'/docs" -i -X PUT -d \'{"model": "BMW","price":1.5,"likes":1,"expiration":"01/01/1911", "category": "record 2 second verycommonword vitamin Food & Beverages Retail Goods Specialty", "name": "Moondog Visions", "relevance": 8.0312880237855993, "lat": 61.207107999999998, "lng": -149.86541, "id": "2"}\''
    print insertCommand
    os.system(insertCommand)

    time.sleep(10)
    # third search for 200 which is there    
    query = 'http://localhost:' + port + '/search?docid=2'
    # do the query
    response = urllib2.urlopen(query).read()
    response_json = json.loads(response)
    #check the result
    checkResult(query, response_json,['2'] )
    try:
        s = commands.getoutput('ps aux | grep srch2-search-server')
        stat = s.split()
        os.kill(int(stat[1]), signal.SIGUSR1)
    except: 
        s = commands.getoutput("ps -A | grep -m1 srch2-search-server | awk '{print $1}'")
        a = s.split()
        cmd = "kill -9 {0}".format(a[-1])
        os.system(cmd)
    print '=============================='

if __name__ == '__main__':    
   #Path of the query file
   #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
   binary_path = sys.argv[1]
   testNewFeatures(binary_path)

