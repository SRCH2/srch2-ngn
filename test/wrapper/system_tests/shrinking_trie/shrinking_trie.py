
import sys, urllib2, os, commands, signal,shutil, commands
sys.path.insert(0,'srch2lib')
import test_lib

port = '8087'
serverHandle = None
totalFailCount = 0
binary_path = None

#Start the SRCH2 engine
def startSrch2Engine():
	global serverHandle
	print "123123" + binary_path
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



def sendInsertQuery(id):
	commands.getstatusoutput('curl "http://localhost:' + str(port) + '/docs"  -i -X PUT -d {"id":"' + id + '","name":"test", "category":"trie", "relevance":"1"}')

def sendSaveCmd():
	commands.getstatusoutput('curl -i "http://localhost:' + str(port) + '/save" -X PUT')

def sendDeleteQuery(id):
	commands.getstatusoutput('curl -i "http://localhost:' + str(port) + '/docs?id=' + id + '" -i X DELETE')

def insertRecords(totalCount, indexFileName):
	for i in range(0, totalCount):
		sendInsertQuery(i)

	sendSaveCmd()

	#Return the size of the index file
	return os.stat(indexFileName).st_size

def removeRecords(start, end, indexFileName):
	for i in range(start, end):
		sendDeleteQuery(i)
	sendSaveCmd()

	#Return the size of the index file
	return os.stat(indexFileName).st_size

def compareSize(sizeA,sizeB):
	global totalFailCount 
	if sizeA - sizeB > 0:
		print 'Size A = ' + sizeA + ' | Size B =  ' + sizeB + ' test pass'
	else :
		print 'Size A = ' + sizeA + ' | Size B =  ' + sizeB + ' test failed'
		totalFailCount = totalFailCount + 1

if __name__ == '__main__':
	if(os.path.exists("data")):
		shutil.rmtree("data")
	binary_path = sys.argv[1]
	startSrch2Engine()
	#Start the test cases
	sizeAfterInsert = insertRecords(1000, sys.argv[2])
	sizeAfterRemove1 = removeRecords(0,500, sys.argv[2])
	compareSize(sizeAfterInsert, sizeAfterRemove1)
	sizeAfterRemove2 = removeRecords(500,900, sys.argv[2])
	compareSize(sizeAfterRemove1, sizeAfterRemove2)
	sizeAfterRemove3 = removeRecords(900,1000, sys.argv[2])
	compareSize(sizeAfterRemove2, sizeAfterRemove3)
	print '=============================='
	shutdownSrch2Engine()
	if(os.path.exists("data")):
		shutil.rmtree("data")
	os._exit(totalFailCount)
