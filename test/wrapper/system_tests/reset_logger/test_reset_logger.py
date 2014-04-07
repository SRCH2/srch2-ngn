
import os, time, sys, commands, urllib2, signal

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'

class logTester():
    def __init__(self, binary_path):
	self.args = [ binary_path, '--config-file=./reset_logger/srch2-config.xml' ]

    def startServer(self):
	os.popen('rm -rf ./reset_logger/logs/')
	os.popen('rm -rf ./reset_logger/indexes/')
        #print ('starting engine: {0}'.format(self.startServerCommand))
        self.serverHandle = test_lib.startServer(self.args);


    #make sure the server is started
    def pingServer(self):
        test_lib.pingServer(port)
        #print 'server is built!'

    #fire a single query
    def fireQuery(self, query):
        # Method 1 using urllib2.urlopen
        #queryCommand = 'http://localhost:8087/search?q=' + query
        #urllib2.urlopen(queryCommand)
        #print 'fired query ' + query
        # Method 2 using curl
        curlCommand = 'curl -s http://localhost:' + str(port) + '/search?q=' + query
        os.popen(curlCommand)


    def killServer(self):
        """
        kills the server
        """
        test_lib.killServer(self.serverHandle)

if __name__ == '__main__':
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]

    if test_lib.confirmPortAvailable(port) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        os._exit(-1)

    tester = logTester(binary_path)
    tester.startServer()
    tester.pingServer()

    #we assume the configuration file is using './logs/srch2-log.txt' as default log file
    logFileName = './reset_logger/logs/srch2-log.txt'
    #get the original size of logger file
 
    size_1 = os.path.getsize(logFileName)     
    #print 'size_1 = ' + str(size_1)
    #fire several queries
    numOfQueries = 10
    for i in range(0, numOfQueries):
    	tester.fireQuery('monday') 
    #get the updated size of logger file
    size_2 = os.path.getsize(logFileName)
    #print 'size_2 = ' + str(size_2)
    #check increasing
    assert size_2 > size_1, 'Error, log into not written into logger file!'

    #check if logrotate is installed
    flagNum, output = commands.getstatusoutput('logrotate --usage')
    #print flagNum
    flagNum, output = commands.getstatusoutput('echo $?')
    #print flagNum
    assert int(flagNum) == 0, 'Error, please install logrotate for reset_logger_system_test'

    #send a command to rotate logger file by using the 3rd-party program 'logrotate' 
    status, output = commands.getstatusoutput("logrotate -f -s ./reset_logger/myLogrotate/status ./reset_logger/myLogrotate/logrotate.conf")

    #get the size of new empty logger file
    size_3 = os.path.getsize(logFileName)
    #print 'size_3 = ' + str(size_3)

    #get the size of old renamed logger file
    if (os.path.exists(str(logFileName) + '.1')):
        size_4 = os.path.getsize(str(logFileName) + '.1')
    else:
        size_4 = 0
        print "Renamed logfile " + str(logFileName) + '.1 missing - Logrotate probably did not rotate it because its already been rotated recently.'

    #print 'size_4 = ' + str(size_4)
    assert (size_3 == 0) and (size_4 == size_2), 'Error, failed to create/switch to new logger file'

    #fire several queries
    for i in range(0, numOfQueries):
        tester.fireQuery('friday')
 
    size_5 = os.path.getsize(logFileName)
    #print 'size_5 = ' + str(size_5)
    assert size_5 > size_3, 'Error, failed to write log info into new logger file'

    tester.killServer()

    print '=====================Reset Logger Test Passed!=========================='
    os._exit(0)
