#These tests are used for the adapter_sqlite
#Require sqlite3 installed.
#Test 1: test loading records from the sqlite table to create an index.
#Test 2: When the server is running, update the record in sqlite,
#        then the listener should fetch the results.
#Test 3: Shut down the engine, and delete the records in sqlite.
#        Then start the engine to test if the engine can fetch the changes.

import sys, urllib2, json, time, subprocess, os, commands, signal,shutil

try:
	import sqlite3
except ImportError:
	os._exit(-1)

sys.path.insert(0,'srch2lib')
import test_lib

port = '8087'
serverHandle = None
totalFailCount = 0
binary_path = None

#Start the SRCH2 engine with sqlite config file.
def startSrch2Engine():
	global serverHandle
	#Start the engine server
        args = [binary_path , '--config-file=adapter_sqlite/conf.xml']

        if test_lib.confirmPortAvailable(port) == False:
                print 'Port' + str(port) + ' already in use -aborting '
                return -1

        print 'starting engine: ' + args[0] + ' ' + args[1]
        serverHandle = test_lib.startServer(args)
        test_lib.pingServer(port)

#Shut down the srch2 engine
def shutdownSrch2Engine():
	global serverHandle
	#Shutdown the engine server
	test_lib.killServer(serverHandle)

#Compare the results with the expected outputs.
def compareResults(testQueriesPath):
	f_test = open(testQueriesPath,'r')
	failCount = 0
	global totalFailCount

        for line in f_test:
                #Get the query keyword and result from the input file
                value = line.split('||')
                queryValue = value[0].split()
                resultValue = value[1].split()

                #Construct the query
                query = prepareQuery(queryValue)

                #Execute the query
                response = urllib2.urlopen(query).read()
                response_json = json.loads(response)

                #Check the result
                failCount += checkResult(query, response_json['results'],resultValue)

        totalFailCount += failCount

#prepare the query based on the valid syntax
def prepareQuery(queryKeywords):
        query = 'http://localhost:' + port + '/search?'
        # prepare the main query part
        query = query + 'q='
        # keywords section
        for i in range(0, len(queryKeywords)):
                if i == (len(queryKeywords)-1):
                        query=query+queryKeywords[i] # last keyword prefix
                else:
                        query=query+queryKeywords[i]+'%20AND%20'

        return query


#Function of checking the results
#Compare the record 'ID' part with the result value
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
            if responseJson[i]['record']['ID'] !=  resultValue[i]:
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record']['ID']+'||'+resultValue[i]
                break
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
             print responseJson[i]['record']['ID']+'||'
            elif i >= len(responseJson):
             print '  '+'||'+resultValue[i]
            else:
             print responseJson[i]['record']['ID']+'||'+resultValue[i]

    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1


#Test 1: test loading index from the sqlite table to create the index.
def testCreateIndexes(conn,sqlQueriesPath,testQueriesPath):
	#Create the test table and Insert record into it
	f_sql = open(sqlQueriesPath,'r')
	for line in f_sql:
		conn.cursor().execute(line)
		print line
	conn.commit()

	#Start the engine and wait to fetch the data, 
	#the engine will create an index from the Sqlite table
	startSrch2Engine()
	time.sleep(5)

	#Compare the results with the expected results
	compareResults(testQueriesPath)
	print '=============================='

#Test 2: When the server is running, update the record in sqlite, 
#then the listener should fetch the results.
def testRunListener(conn,sqlQueriesPath,testQueriesPath):
	#Modify the table while the srch2 engine is running.
	f_sql = open(sqlQueriesPath,'r')
	for line in f_sql:
		conn.cursor().execute(line)
		print line
	conn.commit()

	#Wait for the engine to fetch the changes
	time.sleep(5)

	#Compare the results with the expected results
	compareResults(testQueriesPath)
	print '=============================='

#Test 3: Shut down the engine, and delete the records in sqlite.
#Then start the engine to test if the engine can fetch the changes
def testOfflineLog(conn,sqlQueriesPath,testQueriesPath):
	#Shutdown the engine
	shutdownSrch2Engine()
	time.sleep(3)

	#Modify the table while the srch2 engine is not running
	f_sql = open(sqlQueriesPath,'r')
	for line in f_sql:
		conn.cursor().execute(line)
		print line
	conn.commit()

	#Start the engine and wait it fetch the changes,
	#the engine will get the offline changes.
	startSrch2Engine()
	time.sleep(5)

	#Compare the results with the expected results
	compareResults(testQueriesPath)

	#Shutdown the engine. Finish the test.
	shutdownSrch2Engine()
	print '=============================='

if __name__ == '__main__':
	if(os.path.exists("data")):
		shutil.rmtree("data")
	if(os.path.exists("./adapter_sqlite/srch2Test.db")):
		os.remove("./adapter_sqlite/srch2Test.db")
	conn = sqlite3.connect('./adapter_sqlite/srch2Test.db')
	#Start the test cases
	binary_path = sys.argv[1]
	testCreateIndexes(conn,sys.argv[2],sys.argv[3])
	testRunListener(conn,sys.argv[4],sys.argv[5])
	testOfflineLog(conn,sys.argv[6],sys.argv[7])

	#Do not need to drop the table, remove the db file after the exit.
	print '=============================='
	time.sleep(3)
	conn.close()

	if(os.path.exists("data")):
		shutil.rmtree("data")
	if(os.path.exists("./adapter_sqlite/srch2Test.db")):
		os.remove("./adapter_sqlite/srch2Test.db")
	os._exit(totalFailCount)
