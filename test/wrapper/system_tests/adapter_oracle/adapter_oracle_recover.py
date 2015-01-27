#These tests are used for the adapter_oracle
#Require oracle connected, unixODBC and oracle driver installed.
#Test 1: test loading index from the table to create the index, engine exits gracefully.
#Test 2: Start the engine, update the record in database, 
#        then the listener should fetch the results, then engine exits without saving changes.
#Test 3: During the engine is down, delete the records in oracle.
#        Then start the engine to test if the engine can fetch the changes. 
#        Also test if the engine can recover the changes before it crashes.

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
dataSource = ''
server = ''
conn = None


def populateUserPassFromXML(path):
    global dataSource
    global server
    global dbName

    tree = ET.parse(path)
    dbKeyValues = list(tree.find('./dbParameters/dbKeyValues').getiterator('dbKeyValue'))
    for i in dbKeyValues:
        if(i.attrib['key']=='dataSource'):
            dataSource = i.attrib['value']
        if(i.attrib['key']=='server'):
            server = i.attrib['value']

#Start the SRCH2 engine with oracle config file.
def startSrch2Engine():
    global serverHandle
    #Start the engine server
    args = [binary_path , '--config-file=adapter_oracle/conf.xml']

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
#Compare the record 'PROD_ID' part with the result value
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
            if responseJson[i]['record']['PROD_ID'] !=  resultValue[i]:
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record']['PROD_ID']+'||'+resultValue[i]
                break
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
             print responseJson[i]['record']['PROD_ID']+'||'
            elif i >= len(responseJson):
             print '  '+'||'+resultValue[i]
            else:
             print responseJson[i]['record']['PROD_ID']+'||'+resultValue[i]

    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1


#Test 1: test loading index from the table to create the index, engine exits gracefully.
def testCreateIndexes(conn,sqlQueriesPath,testQueriesPath):
    #Create the test table and Insert record into it
    f_sql = open(sqlQueriesPath,'r')
    for line in f_sql:
        conn.cursor().execute(line)
        print line
    conn.commit()

    #Start the engine and wait to fetch the data, 
    #the engine will create an index from the table
    startSrch2Engine()
    time.sleep(5)

    #Compare the results with the expected results
    compareResults(testQueriesPath)
    shutdownSrch2Engine()
    time.sleep(2)
    print '=============================='

#Test 2: Start the engine, update the record in oracle, 
#and the listener should fetch the results, then engine exits without saving changes.
def testRunListener(conn,sqlQueriesPath,testQueriesPath):
    startSrch2Engine()
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
    #Kill the engine
    killSrch2Engine()
    print '=============================='

#Test 3: During the engine is down, delete the records in oracle.
#Then start the engine to test if the engine can fetch the changes. 
#Also test if the engine can recover the changes before it crashes.
def testOfflineLog(conn,sqlQueriesPath,testQueriesPath):
    #Modify the table while the srch2 engine is not running
    f_sql = open(sqlQueriesPath,'r')
    for line in f_sql:
        conn.cursor().execute(line)
        print line
    conn.commit()
    time.sleep(1)
    
    #Start the engine and wait it fetch the changes,
    #the engine will get the offline changes.
    startSrch2Engine()
    time.sleep(4)

    #Compare the results with the expected results
    compareResults(testQueriesPath)

    #Kill the engine. Finish the test.
    killSrch2Engine()
    print '=============================='

def signal_handler(signum,frame):
    return
def connectToDB(myUserName, myPassword):
    global conn
    global dataSource
    global server

    signal.signal(signal.SIGALRM, signal_handler)
    signal.alarm(3)
    connstr = 'DRIVER={'+str(dataSource)+'};SERVER='+str(server)+';UID='+str(myUserName)+';PWD='+str(myPassword)+';'

    try:
        conn = pyodbc.connect(connstr)
    except Exception, exc: 
        print 'Access denied while connecting to the Oracle database. Make sure the Oracle username, password, data source, server are correct in the ./adapter_oracle/conf.xml '
        print 'Also, please run the command in the ./adapter_oracle/readme.txt'
        print exc
        os._exit(-1)
    finally:
        signal.alarm(0)



if __name__ == '__main__':
    if(os.path.exists("data")):
        shutil.rmtree("data")

    populateUserPassFromXML('./adapter_oracle/conf.xml')

    connectToDB('cdcpub','cdcpub')
        
    #Remove the table
    conn.cursor().execute("""BEGIN
                             EXECUTE IMMEDIATE 'DROP USER CDCSUB';
                             EXCEPTION
                             WHEN OTHERS THEN NULL;
                             END;
                          """)

    conn.cursor().execute("""BEGIN
                             BEGIN
                             DBMS_CDC_PUBLISH.DROP_CHANGE_TABLE('cdcpub','products_ct','Y');
                             END;
                             EXCEPTION
                             WHEN OTHERS THEN NULL;
                             END;
                         """)

    conn.cursor().execute("""BEGIN
                             BEGIN
                             DBMS_CDC_PUBLISH.DROP_CHANGE_SET('cdcset');
                             END;
                             EXCEPTION
                             WHEN OTHERS THEN NULL;
                             END;
                          """)

    conn.cursor().execute("""BEGIN
                             EXECUTE IMMEDIATE 'DROP TABLE CDCPUB.PRODUCTS';
                             EXCEPTION
                             WHEN OTHERS THEN NULL;
                             END;
                          """)

    #Create change table and change table set
    conn.cursor().execute("CREATE USER cdcsub IDENTIFIED BY cdcsub DEFAULT TABLESPACE ts_cdcpub;")
    conn.cursor().execute("GRANT CREATE TABLE TO cdcsub;")
    conn.cursor().execute("GRANT CREATE SESSION TO cdcsub;")
    conn.cursor().execute("GRANT CREATE VIEW TO cdcsub;")
    conn.cursor().execute("GRANT UNLIMITED TABLESPACE TO cdcsub;")
    conn.cursor().execute("CREATE TABLE cdcpub.products(PROD_ID NUMBER(6),PROD_NAME VARCHAR2(50),PROD_LIST_PRICE NUMBER(8,2));")
    conn.cursor().execute("""BEGIN
                             DBMS_CDC_PUBLISH.CREATE_CHANGE_SET(change_set_name => 'cdcset',description => 'Change set for srch2 test',change_source_name => 'SYNC_SOURCE');
                             END;
                          """)
    conn.cursor().execute("""BEGIN
                             DBMS_CDC_PUBLISH.CREATE_CHANGE_TABLE(owner => 'cdcpub',change_table_name => 'products_ct',change_set_name => 'cdcset',source_schema => 'cdcpub',source_table => 'PRODUCTS',column_type_list => 'PROD_ID NUMBER (6),PROD_NAME VARCHAR2 (50),PROD_LIST_PRICE NUMBER (8,2) ',capture_values => 'both',rs_id => 'y',row_id => 'n',user_id => 'n',timestamp => 'n',object_id => 'n',source_colmap => 'y',target_colmap => 'y',DDL_MARKERS=>'n', options_string => 'TABLESPACE ts_cdcpub');
                             END;
                          """)
    conn.cursor().execute("GRANT ALL ON cdcpub.products TO cdcsub;")
    conn.cursor().execute("GRANT ALL ON cdcpub.products_ct TO cdcsub;")

    conn.commit()

    #Start the test cases
    binary_path = sys.argv[1]
    testCreateIndexes(conn,sys.argv[2],sys.argv[3])
    testRunListener(conn,sys.argv[4],sys.argv[5])
    testOfflineLog(conn,sys.argv[6],sys.argv[7])

    print '=============================='
    killSrch2Engine()
    time.sleep(3)

    #Remove the table
    conn.cursor().execute("""BEGIN
                             EXECUTE IMMEDIATE 'drop user cdcsub';
                             EXCEPTION
                             WHEN OTHERS THEN NULL;
                             END;
                          """)

    conn.cursor().execute("""BEGIN
                             BEGIN
                             DBMS_CDC_PUBLISH.DROP_CHANGE_TABLE('cdcpub','products_ct','Y');
                             END;
                             EXCEPTION
                             WHEN OTHERS THEN NULL;
                             END;
                         """)

    conn.cursor().execute("""BEGIN
                             BEGIN
                             DBMS_CDC_PUBLISH.DROP_CHANGE_SET('cdcset');
                             END;
                             EXCEPTION
                             WHEN OTHERS THEN NULL;
                             END;
                          """)

    conn.cursor().execute("""BEGIN
                             EXECUTE IMMEDIATE 'DROP TABLE CDCPUB.PRODUCTS';
                             EXCEPTION
                             WHEN OTHERS THEN NULL;
                             END;
                          """)

    conn.close()
    if(os.path.exists("data")):
        shutil.rmtree("data")
    
    os._exit(totalFailCount)
