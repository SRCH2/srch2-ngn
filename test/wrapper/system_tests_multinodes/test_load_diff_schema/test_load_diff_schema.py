#This function is doing the following things:
#Start the engine with conf1.xml. The engine will load the data and build the indexes
#Shutdown the engine.
#Start the engine with another config file conf2.xml. In conf2.xml, the schema has been changed (name  => nameChanged)
#The engine should raise a warning to tell the user that 'The schema in the config file has changed, remove all the index files and run it again'

import sys, urllib2, time, subprocess, os, commands, signal,shutil

sys.path.insert(0,'srch2lib')
import test_lib

serverHandle = None
totalFailCount = 0
binary_path = None

#Start the SRCH2 engine with sqlite config file.
def startSrch2Engine(args):
	global serverHandle
	#Start the engine server
	serverHandle = test_lib.startServer(args)
	if serverHandle == None:
		return -1

#Shut down the srch2 engine
def shutdownSrch2Engine():
	global serverHandle
	#Shutdown the engine server
	test_lib.killServer(serverHandle)

def testIfErrorLogExists(logPath,errorMsg):
	f_log = open(logPath,'r')
	global totalFailCount

        for line in f_log:
                if errorMsg in line:			
			return
	totalFailCount = 1

if __name__ == '__main__':
	if(os.path.exists("data")):
		shutil.rmtree("data")
	#Start the test cases
	binary_path = sys.argv[1]
	startSrch2Engine([binary_path , './test_load_diff_schema/conf1.xml','./test_load_diff_schema/conf1-A.xml','./test_load_diff_schema/conf1-B.xml'])
	shutdownSrch2Engine()
	time.sleep(5)
	startSrch2Engine([binary_path , './test_load_diff_schema/conf2.xml','./test_load_diff_schema/conf2-A.xml','./test_load_diff_schema/conf2-B.xml'])
	shutdownSrch2Engine()
	time.sleep(5) # sleep to wait for the engine to shutdown
	errorMsg = 'The schema in the config file is different from the serialized schema on the disk'
	testIfErrorLogExists('data/test_load_diff_schema/log.txt',errorMsg)
	print '=============================='
	time.sleep(5) # sleep to wait for the engine to shutdown
	if(os.path.exists("data")):
		shutil.rmtree("data")
	os._exit(totalFailCount)
