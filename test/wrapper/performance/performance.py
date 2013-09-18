from datetime import datetime, timedelta
from pymongo import Connection
import commands
import httplib
import json
import os
import signal
import subprocess
import sys
import time
import urllib
import urllib2
CONST_FAIL_STRING = "FAIL"
CONST_PASS_STRING = "PASS"
class PerformanceTest():

    def __init__(self, config):
        """
        @config , config dict. 
        example:  
        config = { 'server_binaryPath': '../../../build/src/server/',
                'queriesPath': './queries.txt',
                'server_port':8081,
                'server_host': 'http://localhost',
                'server_confg_file_path':'./conf.xml',
                'debug' : True,
                'server_binary_name':'srch2-search-server'
        }
        """
        self.binaryPath = config['server_binaryPath']
        self.binaryName = config['server_binary_name']
        self.config_file_path = config['server_confg_file_path']
        self.port = config['server_port']
        self.host = config['server_host']
        self.debug = config['debug']
        self.mongoConnection = Connection(config['mongoConfig']['host'])
        self.db = self.mongoConnection[config['mongoConfig']['db']]
        self.counter = 1
        self.successFile=config['successFile']
        self.failureFile=config['failureFile']
        self.sfile = open(self.successFile, 'a')
        self.wfile = open(self.failureFile, 'a')

    def startServer(self):
        """
        start the server. 
        """
        binary = self.binaryPath + self.binaryName
        binary = binary + ' --config-file={0} &'.format(self.config_file_path)
        self.debugToConsole('starting engine: {0}'.format(binary))
        os.popen(binary)

    def pingServer(self):
        """
        ping the server
        """
        self.debugToConsole("pinging server")
        info = 'curl -s {0}:{1}/search?q=Garden | grep -q results'.format(self.host, self.port)
        while os.system(info) != 0:
            time.sleep(1)
            info = 'curl -s {0}:{1}/search?q=Garden | grep -q results'.format(self.host, self.port)
        self.debugToConsole("pinging done returning")

    def pushDataToMongo(self, collection, row):
        """
        pushes data to mongodb.
        """
        self.db[collection].insert(row)

    def searchData(self, query):
        pass
    def fireQuery(self, preparedQuery):
        """
        @preparedQuery : query string 
        """
        self.debugToConsole("inside fireQuery")
        try:
            connection = urllib2.urlopen(preparedQuery)
            code = connection.getcode()
            self.debugToConsole("response code received is: {0}".format(code))
            if code == 200:
                response = connection.read()
                response_json = json.loads(response)
                if 'results' in response_json:
                    result = response_json['results']
                    if len(result) > 0:
                        return CONST_PASS_STRING, "{0}: ,{1}".format(code, preparedQuery)
                    else:
                        return CONST_FAIL_STRING, "{0}: ,{1}".format(code, "empty results or no results")
                else:
                    return CONST_FAIL_STRING, "{0}: ,{1}".format(code, "no key as result ")
            else:
                return CONST_FAIL_STRING, "{0}: ,{1}".format(code, "error returned")
        except urllib2.HTTPError, error:
            self.debugToConsole("urllib2.HTTPError cought")
            return CONST_FAIL_STRING, "{0}: ,{1}".format(error.code, error)
        except urllib2.URLError, error:
            self.debugToConsole("urllib2.URLError cought")
            return CONST_FAIL_STRING, error
        except httplib.BadStatusLine, error:
            self.debugToConsole("httplib.BadStatusLine error cought")
            self.restartServer();
            return CONST_FAIL_STRING, "{0}: {1}".format("server crashed for query", error)

    def prepareQuery(self, record):
        """
        @query : the query string as read from the queries.txt file
        url encodes the query
        """
        #q="record["body"]"
        query = 'q={0}'.format(record['body'])
        query = self.quote_url(query, "~*&$=,") # encode the uri
        preparedQuery = '{0}:{1}/search?{2}'.format(self.host, self.port, query)
        return preparedQuery
    def prepareRecord(self):
        """
        returns a dict
        """
        content = "{0}{1}{2}{1}".format("cool", self.counter, "engine");
        obj = {"body":content}
        return obj
        
    def doTest(self):
        """
        - prepare a record to be inserted in mongodb.
        - prepare a query for search engine to search the above record
        - insert record in mongodb
        - in a loop of 10
        -     fire query to search ending to search the above record
        """
        self.debugToConsole("inside doTest")
        queryList = []
        for i in range(0, 1000):
            record = self.prepareRecord() # prepare and get the record 
            self.counter += 1
            self.pushDataToMongo("performance", record)
            query = self.prepareQuery(record) # preapare a query
            queryList.append(query)

        baseQueryList = queryList
        failedQueryList = []
        attempts = 2
        for attempt in range(0, attempts):
            failedQueryList = []
            for i in range(0, len(baseQueryList)):
                try:
                    startTime = datetime.now()
                    status, msg = self.fireQuery(baseQueryList[i])
                    timeTaken = datetime.now() - startTime
                    if status == CONST_FAIL_STRING:
                        # failed 
                        self.debugToConsole("MSG is {0}".format(msg))
                        failedQueryList.append(baseQueryList[i])
                        continue
                    elif status == CONST_PASS_STRING:
                        # passed on this attempt
                        text = "{0},{1},{2},{3}\n".format(i+1, baseQueryList[i], timeTaken.microseconds, attempt)
                        self.writeToFile(status, text)                        
                    else:
                        print "UNKNOWN STATUS", status
                except:
                    print "query was", baseQueryList[i]
                    print "Unexpected error:", sys.exc_info()[0]
            # only re attempt the failed query
            baseQueryList = failedQueryList            
        
        for i in range(0, len(failedQueryList)):
            text = "{0},{1},{2},{3}\n".format(self.counter, failedQueryList[i], timeTaken.microseconds, 2)
            self.writeToFile(CONST_FAIL_STRING, text)        

    def writeToFile(self, status, text):
            if status == CONST_PASS_STRING:
                self.sfile.write(text)
            else:
                self.wfile.write(text)
            
    def closeFiles(self):
        self.sfile.close()
        self.wfile.close()
            
    def quote_url(self, url, safe):
        """URL-encodes a string (either str (i.e. ASCII) or unicode);
        uses de-facto UTF-8 encoding to handle Unicode codepoints in given string.
        """
        return urllib.quote(unicode(url).encode('utf-8'), safe)

    def killServer(self):
        """
        kills the server
        """
        try:
            self.debugToConsole ("killing srch2 server")
            s = commands.getoutput('ps aux | grep srch2-search-server')
            stat = s.split()
            os.kill(int(stat[1]), signal.SIGUSR1)
            self.debugToConsole("server killed!")
        except:
            try:
                s = commands.getoutput("ps -A | grep -m1 srch2-search-server | awk '{print $1}'")
                a = s.split()
                cmd = "kill -9 {0}".format(a[-1])
                os.system(cmd)
                self.debugToConsole("server killed")
            except:
                print "no running instance found to kill, moving ahead."
    def restartServer(self):
        """
            restart the server, first kills the existing running server, then starts server
        """
        self.debugToConsole("********restarting the server**********")
        self.killServer()  
        self.startServer()
        self.pingServer()
        self.debugToConsole("********server up**********")

    def debugToConsole(self, msg):
        if self.debug:
            print msg

    def rebootServer(self):
        self.killServer()
        self.startServer()
        time.sleep(3) # let the server restart

if __name__ == '__main__':
    mongoConfig = {
                   'host' : 'calvin.calit2.uci.edu',
                   'port' : '',
                   'db' : 'test_performance',
    }
    config = { 'server_binaryPath': '../../../build/src/server/',
                'server_port':8081,
                'server_host': 'http://localhost',
                'server_confg_file_path':'./srch2home/conf.xml',
                'debug' : True,
                'server_binary_name':'srch2-search-server',
                'successFile':'successFile.csv',
                'failureFile':'failureFile.csv',
                'mongoConfig':mongoConfig
    }
    tester = PerformanceTest(config)
    try:
        #kill any existing instance of server
        #tester.rebootServer()
        decoration = 40
        print "*"*decoration, "TESTING BEGINS", "*"*decoration
        for num in range(2):
            results = tester.doTest()
        #print the test results
        print "*"*decoration, "TESTING ENDS", "*"*decoration
        tester.closeFiles()
        #tester.killServer();
    except:
        #exception caught, kill the server
        print "sys.exc_info:", sys.exc_info()[0]
        #tester.killServer();
        tester.closeFiles()
