#These tests are used for the adapter_sqlserver
#Require sqlserver connected, unixODBC and sqlserver driver installed.
#Test 1: test loading records from the sqlserver table to create an index.
#Test 2: When the server is running, update the record in sqlserver,
#        then the listener should fetch the results.
#Test 3: Shut down the engine, and delete the records in sqlserver.
#        Then start the engine to test if the engine can fetch the changes.

import sys, urllib2, json, time, subprocess, os, commands, signal,shutil

sys.path.insert(0,'srch2lib')
import test_lib

import xml.etree.ElementTree as ET

sys.path.append(os.getcwd()+'/../../../thirdparty/pyodbc/pyodbc/build/')

try:
    import pyodbc
except ImportError:
    os._exit(-1)



port = '8087'
serverHandle = None
totalFailCount = 0
binary_path = None
myUserName = ''
myPassword = ''
dataSource = ''
server = ''
dbName = ''
conn = None

def populateUserPassFromXML(path):
    global myUserName
    global myPassword
    global dataSource
    global server
    global dbName

    tree = ET.parse(path)
    dbKeyValues = list(tree.find('./dbParameters/dbKeyValues').iter('dbKeyValue'))
    for i in dbKeyValues:
        if(i.attrib['key']=='password'):
            myPassword = i.attrib['value']
        if(i.attrib['key']=='user'):
            myUserName = i.attrib['value']
        if(i.attrib['key']=='dataSource'):
            dataSource = i.attrib['value']
        if(i.attrib['key']=='server'):
            server = i.attrib['value']
        if(i.attrib['key']=='dbName'):
            dbName = i.attrib['value']

#Start the SRCH2 engine with sql server config file.
def startSrch2Engine():
    global serverHandle
    #Start the engine server
    args = [binary_path , '--config-file=adapter_sqlserver/conf.xml']

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


#Test 1: test loading index from the sql server table to create the index.
def testCreateIndexes(conn,sqlQueriesPath,testQueriesPath):
    #Create the test table and Insert record into it
    f_sql = open(sqlQueriesPath,'r')
    for line in f_sql:
        conn.cursor().execute(line)
        print line
    conn.commit()

    #Start the engine and wait to fetch the data, 
    #the engine will create an index from the sql server table
    startSrch2Engine()
    time.sleep(5)

    #Compare the results with the expected results
    compareResults(testQueriesPath)
    print '=============================='

#Test 2: When the server is running, update the record in sql server, 
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

#Test 3: Shut down the engine, and delete the records in sql server.
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
    time.sleep(4)

    #Compare the results with the expected results
    compareResults(testQueriesPath)

    #Shutdown the engine. Finish the test.
    shutdownSrch2Engine()
    print '=============================='

def signal_handler(signum,frame):
    return
def connectToDB():
    global conn
    global dataSource
    global server
    global dbName
    global myUserName
    global myPassword

    signal.signal(signal.SIGALRM, signal_handler)
    signal.alarm(3)
    try:
        conn = pyodbc.connect('DRIVER={'+str(dataSource)+'};SERVER='+str(server)+';DATABASE='+str(dbName)+';UID='+str(myUserName)+';PWD='+str(myPassword)+';')
    except Exception, exc: 
        print 'Access denied while connecting to the SQL Server database. Make sure the SQL Server username, password, data source, server and database name are correct in the ./adapter_sqlserver/conf.xml '
        os._exit(-1)
    finally:
        signal.alarm(0)

if __name__ == '__main__':
    if(os.path.exists("data")):
        shutil.rmtree("data")

    populateUserPassFromXML('./adapter_sqlserver/conf.xml')

    connectToDB()
        
    #Remove the COMPANY table
    conn.cursor().execute("IF OBJECT_ID('COMPANY', 'U') IS NOT NULL DROP TABLE COMPANY")
    conn.cursor().execute("CREATE TABLE COMPANY(ID CHAR(50) PRIMARY KEY NOT NULL, NAME CHAR(50) NOT NULL, AGE CHAR(50) NOT NULL, ADDRESS CHAR(50), SALARY CHAR(50))")
    conn.cursor().execute("ALTER TABLE COMPANY ENABLE CHANGE_TRACKING WITH (TRACK_COLUMNS_UPDATED = ON)")
    #Start the test cases
    binary_path = sys.argv[1]
    testCreateIndexes(conn,sys.argv[2],sys.argv[3])
    testRunListener(conn,sys.argv[4],sys.argv[5])
    testOfflineLog(conn,sys.argv[6],sys.argv[7])

    print '=============================='
    shutdownSrch2Engine()
    time.sleep(3)

    #Remove the COMPANY table
    conn.cursor().execute("DROP TABLE COMPANY")
    conn.close()
    if(os.path.exists("data")):
        shutil.rmtree("data")
    
    os._exit(totalFailCount)
