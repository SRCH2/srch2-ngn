#This test is used for adapter_mongdb
#Using:

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

import MongoDBConn

port = '8087'
dbconn = MongoDBConn.DBConn();
conn = None
handler = None

def createConnection():
	#Connect to mongodb
	conn_status = dbconn.connect()
	if conn_status == -1 :
		os._exit(1)

	global conn
	conn = dbconn.getConn()

	#Show all db
	databases = conn.database_names()

	#Link handler	
	createTable()

def closeConnection():
	#Disconnect from mongodb
	dbconn.close()


def insertRecord():
	jsonRecord = {"trailer_url" : "http://www.youtube.com/watch?v=QHhZK-g7wHo","title" : "Terminator 3: Rise of the Machines","director" : "JamesCameron","year" : 2003,"banner_url" : "http://ia.media-imdb.com/images/M/MV5BMTk5NzM1ODgyN15BMl5BanBnXkFtZTcwMzA5MjAzMw@@._V1_SY317_CR0,0,214,317_.jpg","id" : 765006,"genre" : "drama"}
	handler.insert(jsonRecord)


def updateRecord():
	update_record_id = { "id": 765006 }
	update_record = {"$set": { "director": "Chen" }}
	handler.update(update_record_id,update_record,multi=True)

def deleteRecord():
	delete_record = {"id" : 765006 }
	handler.remove(delete_record)

def dropTable():
	global conn
	conn.drop_database("srch2Test")

def createTable():
	global handler
	handler = conn.srch2Test.movies

#prepare the query based on the valid syntax
def prepareQuery(queryKeywords):
	query = ''
	#################  prepare main query part
	query = query + 'q='
	# local parameters
#	query = query + '%7BdefaultPrefixComplete=COMPLETE%7D'
	# keywords section
	for i in range(0, len(queryKeywords)):
		if i == (len(queryKeywords)-1):
			query=query+queryKeywords[i] # last keyword prefix
		else:
			query=query+queryKeywords[i]+'%20AND%20'

	################# fuzzy parameter
#	query = query + '&fuzzy=false'

	##################################
	return query


#Function of checking the results
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



def process():
	insertRecord()

def testMongoDB(binary_path,queriesAndResultPath):
	#Start the engine server
	
	args = [ binary_path , '--config-file=adapter_mongo/conf.xml']
	
	if test_lib.confirmPortAvailable(port) == False:
		print 'Port' + str(port) + ' already in use -aborting '
		return -1
	
	print 'starting engine: ' + args[0] + ' ' + args[1]
	serverHandle = test_lib.startServer(args)
	test_lib.pingServer(port)
	
	time.sleep(5)

	#construct the query
	failCount = 0
	f_in = open(queriesAndResultPath, 'r')
	
	lineNum = 0
	for line in f_in:
		#get the query keyword and results
		value=line.split('||')
		queryValue=value[0].split()
		resultValue=(value[1]).split()
		#construct the query
		query='http://localhost:' + port + '/search?'
		query = query + prepareQuery(queryValue)
		#print query
		#Test 1: test load index

                #Test 2: test  Listener
                if lineNum == 1:
                        updateRecord()
                        time.sleep(10)	#Has to be >=10 or test will be crashed

		#Test 3: test offline modification
		if lineNum == 2:
			test_lib.killServer(serverHandle)
			deleteRecord()
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
		
		#print response_json

		failCount += checkResult(query, response_json['results'], resultValue )
		lineNum = lineNum + 1

	test_lib.killServer(serverHandle)
	print '=============================='
	return failCount


if __name__ == '__main__':
	binary_path = sys.argv[1]
	queriesAndResultPath = sys.argv[2]
	createConnection()
	process()

	exitCode = testMongoDB(binary_path,queriesAndResultPath)

	dropTable()
	closeConnection()
	os._exit(exitCode)
