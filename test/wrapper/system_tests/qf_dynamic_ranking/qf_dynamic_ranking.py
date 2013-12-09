#
#   Test of qf feature of the SRCH2 Engine
#
#        This test relies on three components:
#              1. This driver script
#              2. A Configuration file: conf.xml
#              3. A query and results file: queriesAndResults.txt
#
#   The format of queryNresults is a follows 
#
#     A query of space delimited keywords and a qf clause seperated by &
#     Then || with the results following. The results are space seperated
#     sorted by score, and have the row's id and score, rounded to two decimal
#     places, seprated by 's'
#
#     eg. 
#         Health Service&name%5E10||97s10 1s1 55s1 68s1 77s1
#
#      is the query of two keywords Health AND Space  with dynamic boosting
#         on the field name.
#      With 5 rows in the expected results, these row's have id's
#      97, 1, 55, 68, 77 and scores 10, 1, 1, 1, 1 respectively.
#        
#            
#     The test cases covered are, as follows:
#
#        1. Test of single keyword
#        2. Test of boost 1 having no effect on results (single keyword)
#        3. Test of two keywords hiting in boosted record
#        4. Test of two keywords with single boost

import sys, urllib2, json, time, subprocess, os, commands, signal

port = '8081'

#make sure that start the engine up
def pingServer():
    info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'
    while os.system(info) != 0:
        time.sleep(1)
        info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
          if ((responseJson[i]['record']['id'] != \
                  resultValue[i].split('s')[0]) \
                 or (round(responseJson[i]['score'], 2) != \
                   round(float(resultValue[i].split('s')[1]),2))):
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+\
                      '||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record']['id']+ \
                    's'+str(round(responseJson[i]['score'],2))+ \
                    '||'+resultValue[i]
                    continue
          break
    else:
        isPass=0
        print query+' test failed'
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


#prepare the query based on the valid syntax
def prepareQuery(queryKeywords, boostValue):
    query = ''
    #################  prepare main query part
    query = query + 'q='
    # local parameters
    query = query + '%7BdefaultPrefixComplete=COMPLETE%7D'
    # keywords section
    for i in range(0, len(queryKeywords)):
        if i == (len(queryKeywords)-1):
            query=query+queryKeywords[i]# last keyword prefix
        else:
            query=query+queryKeywords[i]+'%20AND%20'
    
    ################# fuzzy parameter
    query = query + '&fuzzy=false'

    query=query+'&qf='+boostValue
    
#    print 'Query : ' + query
    ##################################
    return query
    


def testQF(queriesAndResultsPath, binary_path):
    #Start the engine server
    binary= binary_path + '/srch2-search-server'
    binary= binary+' --config-file=./qf_dynamic_ranking/conf.xml &'
    print 'starting engine: ' + binary
    os.popen(binary)

    pingServer()

    #construct the query
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        resultValue=(value[1].split())
        value= value[0].split('&')
        queryValue=value[0].split()
        boostValue=value[1]

        #construct the query
        query='http://localhost:' + port + '/search?'
        query = query + prepareQuery(queryValue, boostValue) 
        print query
        #do the query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)

        #check the result
        checkResult(query, response_json['results'], resultValue)

    #get pid of srch2-search-server and kill the process
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
    queriesAndResultsPath = sys.argv[2]
    testQF(queriesAndResultsPath, binary_path)

