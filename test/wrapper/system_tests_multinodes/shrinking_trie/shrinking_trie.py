#This test is used for shrinking trie.
#It mainly does 7 steps:
#1. Insert 400 records into the engine and save the index onto disk. Then get the size of CL1.idx (trie file)
#2. Delete 100 records from the engine and save the new index onto disk. Then get the size of trie.
#3. Compare and check the size to see if the size of file is shrinked.
#4. Delete 100 records from the engine and save the new index onto disk. Then get the size of trie.
#5. Compare and check the size to see if the size of file is shrinked.
#6. Delete 100 records from the engine and save the new index onto disk. Then get the size of trie.
#7. Compare and check the size to see if the size of file is shrinked.

import sys, urllib2, os, commands, signal,shutil, commands, time
sys.path.insert(0,'srch2lib')
import test_lib

port = '8087'
serverHandle = None
totalFailCount = 0
binary_path = None

#Start the SRCH2 engine
def startSrch2Engine():
	global serverHandle
	#Start the engine server
        args = [binary_path , '--config-file=shrinking_trie/conf.xml']

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


#Send the insert command to the engine
def sendInsertQuery(id):
	status1, output1 = commands.getstatusoutput('curl "http://127.0.0.1:' + str(port) + '/docs"  -i -X PUT -d \'{"id":"' + str(id) + '","name":"'+str(id)+'test' + str(id) + '", "category":"trie", "relevance":"1"}\'')
	#print 'status1 : ' + str(status1) + ' output1 : ' + str(output1)

#Send  the save command to the engine
def sendSaveCmd():
	status1, output1 = commands.getstatusoutput('curl -i "http://127.0.0.1:' + str(port) + '/save" -X PUT')
	#print 'status1 : ' + str(status1) + ' output1 : ' + str(output1)
	time.sleep(1)

#Send the delelte command to the engine
def sendDeleteQuery(id):
	#print 'curl "http://127.0.0.1:' + str(port) + '/docs?id=' + str(id) + '" -i -X DELETE'
	status1, output1 = commands.getstatusoutput('curl "http://127.0.0.1:' + str(port) + '/docs?id=' + str(id) + '" -i -X DELETE')
	#print 'status1 : ' + str(status1) + ' output1 : ' + str(output1)

#Insert "totalCount" of records into the engine and get the size of trie file
def insertRecords(totalCount, indexFileName):
	for i in range(0, totalCount):
		sendInsertQuery(i)

	sendSaveCmd()

	#Return the size of the index file
	print 'Size of CL1.idx : ' + str(os.stat(indexFileName).st_size)
	return os.stat(indexFileName).st_size

#Remove the records with id from "start" to "end", and get the size of the trie file 
def removeRecords(start, end, indexFileName):
	for i in range(start, end):
		sendDeleteQuery(i)
	sendSaveCmd()

	#Return the size of the index file
	print 'Size of CL1.idx : ' + str(os.stat(indexFileName).st_size)
	return os.stat(indexFileName).st_size

#Compare the size to see if the trie is shrinked
def compareSize(sizeA,sizeB):
	global totalFailCount 
	if sizeA - sizeB > 0:
		print 'Size A = ' + str(sizeA) + ' | Size B =  ' + str(sizeB) + ' test pass'
	else :
		print 'Size A = ' + str(sizeA) + ' | Size B =  ' + str(sizeB) + ' test failed'
		totalFailCount = totalFailCount + 1

if __name__ == '__main__':
	if(os.path.exists("data")):
		shutil.rmtree("data")
	binary_path = sys.argv[1]
	startSrch2Engine()
	triePath = "data/shrinking_trie/CL1.idx"

        # insert 400 records
	sizeAfterInsert = insertRecords(400, triePath)

        # delete the first 100 records
	sizeAfterRemove1 = removeRecords(0, 100, triePath)
	compareSize(sizeAfterInsert, sizeAfterRemove1)

        # delete the second 100 records
	sizeAfterRemove2 = removeRecords(100, 200, triePath)
	compareSize(sizeAfterInsert, sizeAfterRemove2)

        # delete the third 100 records
	sizeAfterRemove3 = removeRecords(200, 300, triePath)
	compareSize(sizeAfterRemove1, sizeAfterRemove3)

        # delete the last 100 records
	sizeAfterRemove4 = removeRecords(300, 400, triePath)
	compareSize(sizeAfterRemove2, sizeAfterRemove4)

        # Insert them again
	sizeAfterInsert = insertRecords(400, triePath)

        # delete the first 100 records
	sizeAfterRemove1 = removeRecords(0, 100, triePath)
	compareSize(sizeAfterInsert, sizeAfterRemove1)

	print '=============================='
	shutdownSrch2Engine()
	if(os.path.exists("data")):
		shutil.rmtree("data")
	os._exit(totalFailCount)
