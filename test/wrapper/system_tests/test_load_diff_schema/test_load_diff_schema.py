import sys, urllib2, time, subprocess, os, commands, signal,shutil

sys.path.insert(0,'srch2lib')
import test_lib

port = '8087'
serverHandle = None
totalFailCount = 0
binary_path = None

#Start the SRCH2 engine with sqlite config file.
def startSrch2Engine(confPath):
	global serverHandle
	#Start the engine server
        args = [binary_path , '--config-file='+confPath]

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
	startSrch2Engine('test_load_diff_schema/conf1.xml')
	shutdownSrch2Engine()
	time.sleep(2)
	startSrch2Engine('test_load_diff_schema/conf2.xml')
	shutdownSrch2Engine()
	
	errorMsg = 'The schema in the config file has changed, remove all the index files and run it again'
	testIfErrorLogExists('data/test_load_diff_schema/log.txt',errorMsg)
	print '=============================='
	
	if(os.path.exists("data")):
		shutil.rmtree("data")
	os._exit(totalFailCount)
