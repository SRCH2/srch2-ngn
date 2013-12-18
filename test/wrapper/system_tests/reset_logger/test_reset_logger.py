
import os, time, sys, commands, urllib2


class logTester():
    def __init__(self, binary_path):
	self.binaryPath = binary_path + '/srch2-search-server'
	self.startServerCommand = self.binaryPath + ' --config-file=./reset_logger/srch2-config.xml &'

    def startServer(self):
	os.popen('rm -rf ./reset_logger/logs/')
	os.popen('rm -rf ./reset_logger/indexes/')
        #print ('starting engine: {0}'.format(self.startServerCommand))
        os.popen(self.startServerCommand)


    #make sure the server is started
    def pingServer(self):
        info = 'curl -s http://localhost:8087/search?q=Garden | grep -q results'
        while os.system(info) != 0:
            time.sleep(1)
            info = 'curl -s http://localhost:8087/search?q=Garden | grep -q results'
        #print 'server is built!'

    #fire a single query
    def fireQuery(self, query):
        # Method 1 using urllib2.urlopen
        #queryCommand = 'http://localhost:8087/search?q=' + query
        #urllib2.urlopen(queryCommand)
        #print 'fired query ' + query
        # Method 2 using curl
        curlCommand = 'curl -s http://localhost:8087/search?q=' + query
        os.popen(curlCommand)


    def killServer(self):
        """
        kills the server
        """
        try:
            #print ("killing srch2 server")
            s = commands.getoutput('ps aux | grep srch2 | grep config')
            stat = s.split()
            #print '1 ' + stat[1]
            os.kill(int(stat[1]), signal.CTRL_C_EVENT) # TODO: why does this always throw an exception
            #print ("server killed!")
        except:
            try:
                s = commands.getoutput("ps -A | fgrep -v '<defunct>' | grep -m1 srch2 | awk '{print $1}'")
                a = s.split()
                cmd = "kill -9 {0}".format(a[-1])
                os.system(cmd)
                #print ("server killed")
            except:
                print "no running instance found to kill, moving ahead."


if __name__ == '__main__':
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]

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
    status, output = commands.getstatusoutput("logrotate -s ./reset_logger/myLogrotate/status ./reset_logger/myLogrotate/logrotate.conf")

    #get the size of new empty logger file
    size_3 = os.path.getsize(logFileName)
    #print 'size_3 = ' + str(size_3)
    #get the size of old renamed logger file
    size_4 = os.path.getsize(str(logFileName) + '.1')
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
