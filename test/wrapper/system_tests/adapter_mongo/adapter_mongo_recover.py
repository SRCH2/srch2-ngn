#These tests are used for the adapter_mongdb
#Require pymongo and MongoDB running as replication mode
#Test 1: test loading index from the mongodb table to create the index, engine exits gracefully.
#Test 2: Start the engine, update the record in mongodb, 
#        then the listener should fetch the results, then engine exits without saving changes.
#Test 3: During the engine is down, delete the records in mongodb.
#        Then start the engine to test if the engine can fetch the changes. 
#        Also test if the engine can recover the changes before it crashes.

import sys, urllib2, json, time, subprocess, os, commands, signal,shutil

sys.path.insert(0, 'srch2lib')
import test_lib

import MongoDBConn

port = '8087'
dbconn = MongoDBConn.DBConn();
conn = None
handler = None
totalFailCount = 0

#Start the SRCH2 engine with mongodb config file.
def startSrch2Engine():
	global serverHandle
	#Start the engine server
        args = [binary_path , '--config-file=adapter_mongo/conf.xml']

        if test_lib.confirmPortAvailable(port) == False:
                print 'Port' + str(port) + ' already in use -aborting '
                return -1

        print 'starting engine: ' + args[0] + ' ' + args[1]
        serverHandle = test_lib.startServer(args)
        test_lib.pingServer(port)

#Kill the srch2 engine without saving the index and timestamp
def killSrch2Engine():
	global serverHandle
	#Shutdown the engine server
	test_lib.kill9Server(serverHandle)

#Shut down the srch2 engine
def shutdownSrch2Engine():
	global serverHandle
	#Shutdown the engine server
	test_lib.killServer(serverHandle)

#Create a  connection to the mongodb, link the  handler to the srch2Test table
def createConnection():
	global handler
	global conn
	#Connect to the mongodb
	conn_status = dbconn.connect()
	if conn_status == -1 :
		os._exit(-1)


	conn = dbconn.getConn()

	#Link the handler	
	mongoDBDropTable()	#Drop table if exist

	handler = conn.srch2Test.movies

#Close the connection from the mongodb
def closeConnection():
	dbconn.close()

def mongoDBDropTable():
	global conn
	conn.drop_database("srch2Test")

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

#Compare the results with the expected outputs.
def compareResults(testQueriesPath):
	f_test = open(testQueriesPath,'r')
	failCount = 0
	global totalFailCount

        for line in f_test:
                #Get the query keyword and result from the input file
                value = line.split('||')
		queryValue = value[0].rstrip('\n').split()
		resultValue = []
		if(len(value) != 1):
			resultValue = value[1].rstrip('\n').split('\n')
                
                #Construct the query
                query = prepareQuery(queryValue)

                #Execute the query
                response = urllib2.urlopen(query).read()
                response_json = json.loads(response)

                #Check the result
                failCount += checkResult(query, response_json['results'],resultValue)

        totalFailCount += failCount

#Function of checking the results
#Compare the record 'director' part with the result value
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
            if responseJson[i]['record']['director'] !=  resultValue[i]:
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record']['director']+'||'+resultValue[i]
                break
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
             print responseJson[i]['record']['director']+'||'
            elif i >= len(responseJson):
             print '  '+'||'+resultValue[i]
            else:
             print responseJson[i]['record']['director']+'||'+resultValue[i]

    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1

#Test 1: test loading index from the mongodb table to create the index, engine exits gracefully.
def testCreateIndexes(conn,sqlQueriesPath,testQueriesPath):
	#Create the test table and Insert record into it
	f_sql = open(sqlQueriesPath,'r')
	for line in f_sql:
		jsonRecord = json.loads(line)
		handler.insert(jsonRecord)		
	#Start the engine and wait to fetch the data, 
	#the engine will create an index from the mongodb table
	startSrch2Engine()
	time.sleep(5)

	#Compare the results with the expected results
	compareResults(testQueriesPath)
	shutdownSrch2Engine()
	time.sleep(2)
	print '=============================='

#Test 2: Start the engine, update the record in mongodb, time
#then the listener should fetch the results, then engine exits without saving changes.
def testRunListener(conn,sqlQueriesPath,testQueriesPath):
	startSrch2Engine()
	#Modify the table while the srch2 engine is running.
	f_sql = open(sqlQueriesPath,'r')
	for line in f_sql:
		value = line.split('||')
                record_id = json.loads(value[0])
                record_op = json.loads(value[1])
		print 'Updating Record ID: ' + str(record_id) + ' with OP: ' + str(record_op)
		handler.update(record_id,record_op,multi=True)
		
	#Wait for the engine to fetch the changes
	time.sleep(5)

	#Compare the results with the expected results
	compareResults(testQueriesPath)
	#Kill the engine
	killSrch2Engine()
	print '=============================='

#Test 3: During the engine is down, delete the records in mongodb.
#Then start the engine to test if the engine can fetch the changes. 
#Also test if the engine can recover the changes before it crashes.
def testOfflineLog(conn,sqlQueriesPath,testQueriesPath):
	#Modify the table while the srch2 engine is not running
	f_sql = open(sqlQueriesPath,'r')
	for line in f_sql:
		jsonRecord = json.loads(line)
		print 'Deleting record : ' + str(jsonRecord)
		handler.remove(jsonRecord)

	#Start the engine and wait it fetch the changes,
	#the engine will get the offline changes.
	startSrch2Engine()
	time.sleep(5)

	#Compare the results with the expected results
	compareResults(testQueriesPath)

	#Shutdown the engine. Finish the test.
	killSrch2Engine()
	print '=============================='

if __name__ == '__main__':
	if(os.path.exists("data")):
		shutil.rmtree("data")
	createConnection()
	#Start the test cases
	binary_path = sys.argv[1]

	try:
		testCreateIndexes(conn,sys.argv[2],sys.argv[3])
		testRunListener(conn,sys.argv[4],sys.argv[5])
		testOfflineLog(conn,sys.argv[6],sys.argv[7])
	except Exception as e:
		print e
		mongoDBDropTable()
		closeConnection()
		os._exit(-1)
	time.sleep(3)
	mongoDBDropTable()
	closeConnection()

	if(os.path.exists("data")):
		shutil.rmtree("data")
	os._exit(totalFailCount)
