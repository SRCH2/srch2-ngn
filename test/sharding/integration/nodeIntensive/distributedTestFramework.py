import sys, urllib2, json, time, subprocess, os, commands, signal, paramiko, socket, threading

sys.path.insert(0, 'srch2lib')
import test_lib


nodesInfoRelFilePath = "./nodeIntensive/nodeAddress.txt"

srch2ngnGitRepoDir = ""
integrationTestDir = ""
testBinaryDir = ""
testBinaryFileName = ""
nodesInfoFilePath = ""
transactionFile = ""
nodes = dict()
serverHandles = []
sshClient = dict()
temp = ""
myIpAddress = [(s.connect(('8.8.8.8', 80)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1]

coreName = ""

#The node class holds node specific information like port number, ip Address, path of config file and process id

class node:
    def __init__(self, nodeId, portNum, conf, ipAddr, proc):
        self.portNo = portNum
        self.Id = nodeId
        self.conf = conf
        self.ipAddress = ipAddr
        self.pid = ""
        self.proc = ""

def confirmPortAvailable(ipAddress,port) :
    query = 'http://'+ipAddress+':' + str(port) + '/info'
    opener = urllib2.build_opener(urllib2.HTTPHandler)
    request = urllib2.Request(query, '')
    try:
        response = opener.open(request).read()
    except urllib2.URLError as err:
        # err code is 111 on Ubuntu Linux and 61 on Mac (darwin)
        if hasattr(err, 'reason') and hasattr(err.reason, 'errno') and ((sys.platform == 'linux2' and err.reason.errno == 111) or (sys.platform == 'darwin' and err.reason.errno == 61)):
            return True # connection refused - port available
        return False # unexpected error response - nonetheless port must be in use
    return False # no error - port already in use


def checkQueryResult(resultValue, output):
    for result in resultValue:
        print result
        flag = str(output).find(result)      
        assert flag > -1, "incorrect query result"


def pingServer(ipAddr, port, query = 'q=march', timeout = 60):

    info = 'curl -s \"http://' + ipAddr +':' + str(port) + '/search?' + query + '\" | grep -q results'
    #print "Pinging with: " + info
    while timeout >= 0 and subprocess.call(info, shell=True) != 0:
        timeout -= 1
        time.sleep(1)
    #print 'server is built!'
    if timeout < 0:
        print "WARNING: Timed out waiting for the server to start!"
        return -1
    return 0

#It kills all the engine running
def killServers():
    for key in nodes:
        if(nodes[key].ipAddress == myIpAddress):
            os.system("kill -9 " + str(nodes[key].pid))
            os.popen('rm -rf ./temporaryStackoverflow/core2/*.idx')
            continue
        sshClient[nodes[key].Id] = paramiko.SSHClient()
        sshClient[nodes[key].Id].set_missing_host_key_policy(paramiko.AutoAddPolicy())
        sshClient[nodes[key].Id].connect(nodes[key].ipAddress)
        stdin, stdout, stderr = sshClient[nodes[key].Id].exec_command('kill -9 ' + nodes[key].pid)
        stdin, stdout, stderr = sshClient[nodes[key].Id].exec_command('cd gitrepo/srch2-ngn/test/sharding/integration');
        stdin, stdout, stderr = sshClient[nodes[key].Id].exec_command('rm -rf ./temporaryStackoverflow/core2/*.idx')

#It only sets up SSH connections without starting the engine
def setupSSH():
    for key in nodes:
        if(nodes[key].ipAddress == myIpAddress):
            continue
        sshClient[nodes[key].Id] = paramiko.SSHClient() 
        sshClient[nodes[key].Id].set_missing_host_key_policy(paramiko.AutoAddPolicy())
        sshClient[nodes[key].Id].connect(nodes[key].ipAddress)

#prepare the query for querying the "id" attribute
def prepareQueryID(queryKeywords):
    query = ''
    #################  prepare main query part
    query = query + 'q=id:'+queryKeywords[0]
    # local parameters
    
    ##################################
    return query

#Prepares a general query format
def prepareQuery(queryKeywords):
    query = ''
    #################  prepare main query part
    query = query + 'q='
    # local parameters
    # keywords section
    return query

#Reads the nodeAddress file and populates the list of nodes
def parseNodes(nodesPath):
    f_in = open(nodesPath, 'r')
    for line in f_in:
        value=line.split('||')
        print "node details is" 
        nodeId=value[0].split()
        print str(nodeId)  
        ipAddr = value[1].split()
        print ipAddr
        portNum=value[2].split()
        print portNum
        tempConf = value[3].split()
        conf= integrationTestDir + tempConf[0]
        print conf
        nodes[nodeId[0]] = node(nodeId[0], portNum[0], conf, ipAddr[0], "")

#Starts the engine on the corresponding machine. Note that before this function is called, SSH connection has already been set up.
def startEngine(nodeId, transactionFile):
    binary_path = testBinaryDir + testBinaryFileName
    if(nodes[nodeId].ipAddress == myIpAddress):
        out = open(integrationTestDir + '/dashboardFiles/' + transactionFile[0:-4] + '-dashboard-'+nodes[nodeId].Id+ '.txt','w')
        temp = subprocess.Popen([binary_path, '--config='+nodes[nodeId].conf], stdout=out) 
        nodes[nodeId].proc = temp
        #os.system(binary_path + " --config=" + nodes[nodeId].conf + ' > ' + integrationTestDir + '/dashboardFiles/' + transactionFile[0:-4] + '-dashboard-'+nodes[nodeId].Id+ '.txt')      
        nodes[nodeId].pid = temp.pid
        #print str(nodes[nodeId].pid) + "---------------------------"
        pingServer(nodes[nodeId].ipAddress, nodes[nodeId].portNo)
        return
    print binary_path + ' --config=' + nodes[nodeId].conf
    stdin, stdout, stderr = sshClient[nodes[nodeId].Id].exec_command('cd ' + integrationTestDir + '; echo $$;exec '+binary_path+' --config='+ nodes[nodeId].conf + ' > ' + integrationTestDir + '/dashboardFiles/'+transactionFile[0:-4] + '-dashboard-' + nodes[nodeId].Id + '.txt')
    #stdin, stdout, stderr = sshClient[nodes[nodeId].Id].exec_command('cd gitrepo/srch2-ngn/test/sharding/integration;mkdir temporaryCheck');
    nodes[nodeId].pid = stdout.readline()
    #print str(stdout.readline())
#    if(confirmPortAvailable(nodes[nodeId].ipAddress, nodes[nodeId].portNo) == false):
#        print "port not available, so exiting"
#        os._exit()
    pingServer(nodes[nodeId].ipAddress, nodes[nodeId].portNo)
    #time.sleep(5)

#Kills the engine by sending kill signal
def killEngine(nodeId):
    fin = open("crashReports.txt","a")
    if(nodes[nodeId].ipAddress == myIpAddress):
        print "process to be deleted " + str(nodes[nodeId].pid) + " " + str(nodes[nodeId].Id)
        err = -1
        #nodes[nodeId].proc.kill()
        #nodes[nodeId].proc.wait()
        err = os.system('kill -9 ' + str(nodes[nodeId].pid))
        err2 = os.system('rm -rf SRCH2_Cluster/node-' + nodes[nodeId].Id)
        nodes[nodeId].proc.wait()
        print "responseCode is " + str(err) 
        if (err != 0):
            errorMessage = "Error in killing node " + str(nodes[nodeId].Id) + " in transaction file " + sys.argv[2] + "\n"
            fin.write(errorMessage)  
        return
    print "process to be deleted " + str(nodes[nodeId].pid) + " " + str(nodes[nodeId].Id)
    stdin, stdout, stderr = sshClient[nodes[nodeId].Id].exec_command('kill -9 ' + nodes[nodeId].pid)    
    responseCode = stderr.readlines()
    stdin, stdout, stderr = sshClient[nodes[nodeId].Id].exec_command('cd ' + integrationTestDir + ';rm -rf SRCH2_Cluster/node-' + nodes[nodeId].Id)
    print "responseCode is " + str(responseCode)
    if(responseCode != []):
        errorMessage = "Error in killing node " + str(nodes[nodeId].Id) + " in transaction file " + sys.argv[2] + "\n"
        fin.write(errorMessage)

#Opens the file containing record, forms json arrays and does bulk insert.
def bulkInsert(inputFile, k, num, ipAddress, portNo, operation):
    i = 0;
    for counter in range(0,k):
        
        data = []
        for line in inputFile:
            i = i+1;
            val = line.split('\n')
            jsonrec = val[0]
            if(jsonrec[0] == '['):
                continue;
            elif(jsonrec[0] == ']'):
                continue;
            if(jsonrec[-1] == ','):
                data.append(jsonrec[:-1])
            else:
                data.append(val[0])
            if (i%50 == 0 ):
                break;

        dd =  "["+','.join(data) +"]"
        if(operation == "bulkInsertFromBeginning2"):
            global coreName 
            coreName = "/statemedia"
        else:
            global coreName
            coreName = ""
        #qq = 'curl "http://' + ipAddress + ':' + portNo + '/docs" -i -X PUT -d ' + '\''+dd+'\'';
        #status, output = commands.getstatusoutput(qq)
        try:
            req = urllib2.Request("http://" + ipAddress + ":" + portNo + coreName + "/docs", dd, {'Content-Type': 'application/json'})
            req.get_method = lambda:'PUT'
            f = urllib2.urlopen(req)
            response = f.read()
            print counter
            f.close()
        except Exception:
            print "node " + ipAddress + ":" + portNo + " not available"
        #print output

#It is used for sending stress queries.
def sendQuery(inputFile, ipAddress, portNo):
    fin = open(inputFile[0])
    for line in fin:
        val = line.split()

        query = ''
    #################  prepare main query part
        query = query + 'q='
    # local parameters
    # keywords section
        for i in range(0, len(val)):
            if i == (len(val)-1):
                query=query+val[i]+'*' # last keyword prefix
            else:
                query=query+val[i]+'%20AND%20'

    ################# fuzzy parameter

#    print 'Query : ' + query
    ##################################
        q = 'curl ' + '\"http://' + ipAddress + ':' + portNo + '/search?' + query +'\"'
        try:
            status, output = commands.getstatusoutput(q)
            #print output
            print str(status)
        except Exception:
            print "node not available"

#Sends curl request based on command written in transaction file. query2, info2, bulkInsertFromBeginning2 commands are for statemedia core.
def test(transactionFile):
    f_in = open(transactionFile, 'r')
    inputFile = ""
    for line in f_in:
        if ((line.strip() == '')or(line[0][0] == '#')):
            continue
        value = line.split('||')
        nodeId=value[0].split()
        operation=value[1].split()
        if(operation[0] == 'start'):
             startEngine(nodeId[0], transactionFile)
        if(operation[0] == 'kill'):
             killEngine(nodeId[0]) 

        if(operation[0] == 'queryID'):
             queryValue=value[2].split()
             resultValue=(value[3]).split()
             numberOfResultsFound=(value[4]).split()
             #construct the query
             query='http://' + nodes[nodeId[0]].ipAddress + ':' + nodes[nodeId[0]].portNo + '/search?'
             query = query + prepareQueryID(queryValue)
             qq = 'curl "'+ query +'"'
             status, output = commands.getstatusoutput(qq)
             print output
             flag1 = str(output).find(resultValue[0]);
             flag2 = str(output).find(numberOfResultsFound[0]);
             assert flag1 > -1, "querying failed - inconsistent output"
             assert flag2 > -1, "Wrong number of results returned"

        if(operation[0] == 'query'):
             queryValue = value[2]
             resultValue = value[3].split()
             numberOfResultsFound=(value[4]).split()
             query='http://' + nodes[nodeId[0]].ipAddress + ':' + nodes[nodeId[0]].portNo + '/search?' 
             qq = 'curl "'+ query + queryValue + '"'
             status, output = commands.getstatusoutput(qq)
             print output
             flag = str(output).find(numberOfResultsFound[0]);
             assert flag > -1, "Wrong number of results returned"
             checkQueryResult(resultValue, output)

        if(operation[0] == 'query2'):
             queryValue = value[2]
             resultValue = value[3].split()
             numberOfResultsFound=(value[4]).split()
             query='http://' + nodes[nodeId[0]].ipAddress + ':' + nodes[nodeId[0]].portNo + '/statemedia/search?'
             qq = 'curl "'+ query + queryValue + '"'
             status, output = commands.getstatusoutput(qq)
             print output
             flag = str(output).find(numberOfResultsFound[0]);
             assert flag > -1, "Wrong number of results returned"
             checkQueryResult(resultValue, output)

        if(operation[0] == 'insert'):
             insertValue=value[2]
             expectedValue=value[3].split()
             #test , insert a record
             command = 'curl "http://' + nodes[nodeId[0]].ipAddress + ':' + nodes[nodeId[0]].portNo + '/docs" -i -X PUT -d ' + '\''+insertValue+'\'';
             status, output = commands.getstatusoutput(command)
             print output
             print expectedValue[0]
             flag = str(output).find(expectedValue[0])
             print flag
             assert flag > -1, 'Error, rid <no.> is not inserted'

        if(operation[0] == 'bulkInsert'):
             filePath = value[2].split()
             print "file path " + filePath[0]
             if(inputFile == ""):
                 inputFile = open(filePath[0])
             k = value[3].split()
             num = value[4].split()
             start = time.time()
             bulkInsert(inputFile, int(k[0]), int(num[0]), nodes[nodeId[0]].ipAddress, nodes[nodeId[0]].portNo, "bulkInsert") 
             end = time.time()
             elapsedtime = end - start
             print str(elapsedtime)
  
        if(operation[0] == 'bulkInsertFromBeginning'):
             filePath = value[2].split()
             inputFile = open(filePath[0])
             k = value[3].split()
             num = value[4].split()
             bulkInsert(inputFile, int(k[0]), int(num[0]), nodes[nodeId[0]].ipAddress, nodes[nodeId[0]].portNo, "bulkInsertFromBeginning")

        if(operation[0] == 'bulkInsertFromBeginning2'):
             filePath = value[2].split()
             inputFile = open(filePath[0])
             k = value[3].split()
             num = value[4].split()
             bulkInsert(inputFile, int(k[0]), int(num[0]), nodes[nodeId[0]].ipAddress, nodes[nodeId[0]].portNo, "bulkInsertFromBeginning2")


        if(operation[0] == 'delete'):
             deleteId=value[2].split()
             expectedValue=value[3].split()
             commandDelete = 'curl "http://' + nodes[nodeId[0]].ipAddress + ':'+nodes[nodeId[0]].portNo + '/docs?id='+deleteId[0]+'" -i -X DELETE';
             status, output = commands.getstatusoutput(commandDelete)
             flag = str(output).find(expectedValue[0]);
             print output
             assert flag > -1, 'Error file could not be deleted'
        if(operation[0] == 'update'):
             inputValue=value[2]
             expectedValue=value[3]
             command = 'curl "http://' + nodes[nodeId[0]].ipAddress + ':' + nodes[nodeId[0]].portNo + '/update" -i -X PUT -d ' + '\'' + inputValue + '\'';
             status, output = commands.getstatusoutput(command)
             print output
             flag = str(output).find(expectedValue[0]);
             assert flag > -1, 'Error, record could not be updated'

        if(operation[0] == 'info'):
             command = 'curl -i "http://' + nodes[nodeId[0]].ipAddress + ':' + nodes[nodeId[0]].portNo + '/info"'
             expectedValue = value[2].split()
             status, output = commands.getstatusoutput(command)
             print output
             flag1 = str(output).find(expectedValue[0])
             assert flag1 > -1, "Incorrect info response"

        if(operation[0] == 'info2'):
             command = 'curl -i "http://' + nodes[nodeId[0]].ipAddress + ':' + nodes[nodeId[0]].portNo + '/statemedia/info"'
             expectedValue = value[2].split()
             status, output = commands.getstatusoutput(command)
             print output
             flag1 = str(output).find(expectedValue[0])
             assert flag1 > -1, "Incorrect info response"

        if(operation[0] == 'save'):
             command = 'curl -i "http://' + nodes[nodeId[0]].ipAddress + ':' + nodes[nodeId[0]].portNo + '/save" -X PUT'
             status, output = commands.getstatusoutput(command)
             expectedValue = value[2].split()      
             flag = str(output).find(expectedValue[0])
             print output
             assert flag > -1, "Save command failed"
        if(operation[0] == 'export'):
             print "export"
             intputValue = value[2]
             expectedOutput = value[3].split()
             #print "input in export " + inputValue
             command = 'curl -i "http://' + nodes[nodeId[0]].ipAddress + ':' + nodes[nodeId[0]].portNo + '/export?exported_data_file=' + value[2] + '\" -X PUT'    
             status, output = commands.getstatusoutput(command)
             print "export output " + output    
             flag = str(output).find(expectedOutput[0])
             assert flag > -1, "export command failed"
        if(operation[0] == 'sleep'):
             inputTime = value[2]
             time.sleep(int(inputTime))
        
        if(operation[0]=='stressQuery'):
             inputFile = value[2].split()
             sendQuery(inputFile, nodes[nodeId[0]].ipAddress, nodes[nodeId[0]].portNo) 
        
        if(operation[0]=='numberOfRecordsInCore'):
             coreIndex = int(value[2])
             expectedValue = int(value[3])
             response = urllib2.urlopen('http://' + nodes[nodeId[0]].ipAddress + ":" + nodes[nodeId[0]].portNo + '/_debug/stats')
             jsonData = response.read()
             jsonOutput = json.loads(jsonData)
             numberOfRecords = jsonOutput["cores"][coreIndex]["totalNumberOfDocuments"]
             assert (int(numberOfRecords) == expectedValue), "wrong number of records in the core"
        
        if(operation[0]=='nodeCount'):
             expectedValue = value[2].split()
             response = urllib2.urlopen('http://' + nodes[nodeId[0]].ipAddress + ":" + nodes[nodeId[0]].portNo + '/_debug/stats')
             jsonData = response.read()             
             jsonOutput = json.loads(jsonData)
             numberOfNodes = str(jsonOutput["nodes"]["count"])
             assert (str(expectedValue[0]) == str(numberOfNodes)), "wrong number of nodes"       
         
        if(operation[0] == 'numberOfRecordsInShards'):
             coreIndex = int(value[2])
             expectedValue = int(value[3])
             response = urllib2.urlopen('http://' + nodes[nodeId[0]].ipAddress + ":" + nodes[nodeId[0]].portNo + '/_debug/stats')
             jsonData = response.read()
             jsonOutput = json.loads(output)
             numOfPrimaryShards = 2
             totalDoc = 0
             # we assume 0 is the index of stackoverflow core
             for i in range(0, numOfPrimaryShards):
                 totalDoc = totalDoc + jsonOutput["cores"][coreIndex]["all-cluster-shards"][i]["numberOfDocuments"]

             assert (totalDoc == int(expectedValue)), "total number of document in shards are wrong"
        
        if(operation[0] == 'merge'):
             command = 'curl -i "http://' + nodes[nodeId[0]].ipAddress + ':' + nodes[nodeId[0]].portNo + '/merge" -i -X PUT ';
             status, output = commands.getstatusoutput(command)
             print output

              
 
if __name__ == '__main__':

    confFilePath = sys.argv[1]
    confContent = ""
    confFile = open(confFilePath, 'r')
    for line in confFile:
       confContent = confContent + line
    confJson = json.loads(confContent)
    print confJson
    srch2ngnGitRepoDir = str(confJson['srch2ngnGitRepoDir'])
    integrationTestDir = srch2ngnGitRepoDir + str(confJson['integrationTestDir'])
    testBinaryDir = srch2ngnGitRepoDir + str(confJson['testBinaryDir'])
    testBinaryFileName = str(confJson['testBinaryFileName'])
    nodesInfoFilePath = integrationTestDir + nodesInfoRelFilePath   

    parseNodes(nodesInfoFilePath)
    setupSSH()
    transactionFile = sys.argv[2]
    try:
        exitCode = test(transactionFile)
        #killServers()
        os._exit(1)
    finally:
        print "test case failed"
        killServers()
        print "inside finally"
