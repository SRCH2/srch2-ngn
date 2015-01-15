# Automated test script to start engine, fire queries and verify results
# This test case tests feedback ranking by sending a record as a feedback for a query
# and then verifying whether the record is ranked higher for the same query in a next search.
#
import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'

greenColor = "\x1B[32;40m"
redColor = "\x1B[31;40m"
endColor = "\x1B[0m"

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
            if responseJson[i]['record_id'] !=  resultValue[i]:
                isPass=0
                print query+' test ' + redColor + 'FAIL' + endColor
                print 'query results||expected results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record_id']+'||'+resultValue[i]
                break
    else:
        isPass=0
        print query+' test ' + redColor + 'FAIL' + endColor
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
             print responseJson[i]['record_id']+'||'
            elif i >= len(responseJson):
             print '  '+'||'+resultValue[i]
            else:
             print responseJson[i]['record_id']+'||'+resultValue[i]

    if isPass == 1:
        print  query+' test' + greenColor + ' PASS' + endColor
        return 0
    return 1


#prepare the query based on the valid syntax
def prepareQuery(queryKeywords):
    query = ''
    #################  prepare main query part
    query = query + 'q='
    # local parameters
    #query = query + '%7BdefaultPrefixComplete=COMPLETE%7D'
    # keywords section
    for i in range(0, len(queryKeywords)):
        if i == (len(queryKeywords)-1):
            query=query+queryKeywords[i] # last keyword prefix
        else:
            query=query+queryKeywords[i]+'%20AND%20'
    return query
    
def test(queriesAndResultsPath, binary_path, configFilePath):
    #Start the engine server
    args = [ binary_path, '--config-file=' + configFilePath]

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    #test_lib.pingServer(port, 'q=garbage', 30)

    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #if line is empty ignore
        if len(line) == 0:
            continue
        #ignore comments
        if line[0] == '#':
            continue
        #get the query keyword and results
        value=line.split('||')
        if len(value) < 2:
            continue # ignore bad line
        if value[0] == 'W':
            # sleep between test cases for merge process to finish.
            sleepTime = value[1]
            time.sleep(float(sleepTime))
        elif value[0] == 'C':
            # the line is command query (feedback)
            command = value[1]
            payload = value[2]
            coreName = ''
            if len(value) > 3:
                coreName = value[3].strip('\n').strip()

            if coreName == "":
                query='http://localhost:' + port + '/' + command
            else:
                query='http://localhost:' + port + '/' + coreName + '/' + command
            print query + " -X PUT -d '" + payload.strip('\n') + "'"
            try:
                request = urllib2.Request(query, data=payload)
                request.get_method = lambda: 'PUT'
                opener = urllib2.build_opener(urllib2.HTTPHandler)
                url = opener.open(request)
            except urllib2.HTTPError as e:
                print e.read()
                test_lib.killServer(serverHandle)
                return 1 
            time.sleep(1)
        else:
            # the line is a search query
            queryValue=value[1].split()
            resultValue=(value[2]).split()
            coreName = ''
            if len(value) > 3:
                coreName = value[3].strip('\n').strip()

            #construct the query
            if coreName == '':
                query='http://localhost:' + port + '/search?'
            else:
                query='http://localhost:' + port + '/' + coreName  + '/' + 'search?'
            query = query + prepareQuery(queryValue) 
            print query
            #do the query
            response = urllib2.urlopen(query).read()
            response_json = json.loads(response)

            #check the result
            failCount += checkResult(query, response_json['results'], resultValue )

    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount

if __name__ == '__main__':      
    binary_path = sys.argv[1]
    queriesAndResultsPath = './feedback/testCases.txt'
    exitCode = test(queriesAndResultsPath, binary_path , './feedback/conf.xml')
    os._exit(exitCode)

