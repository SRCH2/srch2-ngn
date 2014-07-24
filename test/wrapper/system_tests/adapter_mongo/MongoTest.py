#These tests are used for the adapter_mongdb
#Require pymongo and MongoDB running as replication mode
#Test 1: test loading records from the MongoDB table to create a index.
#Test 2: test listener, during the server running, update the record in the mongodb, 
#        the listener will fetch the results.
#Test 3: test the offline modification, first shut down the engine and delete the records in the mongodb,
#        then start the engine to test if the engine can fetch the deletion.

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

import MongoDBConn

port = '8087'
dbconn = MongoDBConn.DBConn();
conn = None
handler = None

#Create a  connection to the mongodb, link the  handler to the srch2Test table
def createConnection():
	#Connect to the mongodb
	conn_status = dbconn.connect()
	if conn_status == -1 :
		os._exit(10)

	global conn
	conn = dbconn.getConn()

	#Link the handler	
	mongoDBDropTable()	#Drop table if exist
	mongoDBCreateTable()


#Close the connection from the mongodb
def closeConnection():
	dbconn.close()


def mongoDBInsertRecord():
	jsonRecord = {"trailer_url" : "http://www.youtube.com/watch?v=QHhZK-g7wHo","title" : "Terminator 3: Rise of the Machines","director" : "JamesCameron","year" : 2003,"banner_url" : "http://ia.media-imdb.com/images/M/MV5BMTk5NzM1ODgyN15BMl5BanBnXkFtZTcwMzA5MjAzMw@@._V1_SY317_CR0,0,214,317_.jpg","id" : 765006,"genre" : "drama"}
	handler.insert(jsonRecord)


def mongoDBUpdateRecord():
	update_record_id = { "id": 765006 }
	update_record = {"$set": { "director": "Chen" }}
	handler.update(update_record_id,update_record,multi=True)

def mongoDBDeleteRecord():
	delete_record = {"id" : 765006 }
	handler.remove(delete_record)

def mongoDBDropTable():
	global conn
	conn.drop_database("srch2Test")

def mongoDBCreateTable():
	global handler
	handler = conn.srch2Test.movies

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


#Insert a record into the MongoDB before starting the engine.
def process():
	mongoDBInsertRecord()

#Main test including 3 test cases
def testMongoDB(binary_path,queriesAndResultPath):
	#Start the engine server
	args = [ binary_path , '--config-file=adapter_mongo/conf.xml']
	
	if test_lib.confirmPortAvailable(port) == False:
		print 'Port' + str(port) + ' already in use -aborting '
		return -1
	
	print 'starting engine: ' + args[0] + ' ' + args[1]
	serverHandle = test_lib.startServer(args)
	test_lib.pingServer(port)
	
	#Wait the engine to start, the engine will create a index from the MongoDB table.
	time.sleep(5)	

	#construct the query
	failCount = 0
	f_in = open(queriesAndResultPath, 'r')
	
	testNum = 0
	for line in f_in:
		#get the query keyword and results from the input file
		value=line.split('||')
		queryValue=value[0].split()
		resultValue=(value[1]).split()
		#construct the query
		query = prepareQuery(queryValue)
		#Test 1: test loading index from MongoDB table to create the index.
		#Do nothing, just curl the query and check the result.		

                #Test 2: test  Listener, during the server running, update the record in mongodb, the listener will fetch the result.
                if testNum == 1:
			mongoDBUpdateRecord()
			time.sleep(10)	#Has to be >=10 or test will be crashed, wait the engine to get the update.

		#Test 3: test offline modification, first shut down the engine and delete the record in mongodb, then start the engine to test if the engine can fetch the deletion.
		if testNum == 2:
			test_lib.killServer(serverHandle)
         		mongoDBDeleteRecord()
         		if test_lib.confirmPortAvailable(port) == False:
         			print 'Port' + str(port) + ' already in use -aborting '
                		return -1

         		print 'starting engine: ' + args[0] + ' ' + args[1]
         		serverHandle = test_lib.startServer(args)
         		test_lib.pingServer(port)
         		time.sleep(10)

		#do the query
		response = urllib2.urlopen(query).read()
		response_json = json.loads(response)
		
		#check the result
		failCount += checkResult(query, response_json['results'], resultValue )
		testNum = testNum + 1

	print '=============================='
	test_lib.killServer(serverHandle)
	return failCount


if __name__ == '__main__':
	binary_path = sys.argv[1]
	queriesAndResultPath = sys.argv[2]
	createConnection()
	process()	#Preprocess, insert a record into the mongodb before the engine start.

	exitCode = 0
	try:
		exitCode = testMongoDB(binary_path,queriesAndResultPath)
	except:
		mongoDBDropTable()
		closeConnection()
		os._exit(1)		  
	mongoDBDropTable()
	closeConnection()
	os._exit(exitCode)
