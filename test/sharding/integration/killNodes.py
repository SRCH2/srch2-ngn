#To kill the engine
#Release mode
#python killNodes.py release
#Debug mode
#python killNodes.py debug

import commands,sys, json, os

if __name__ == '__main__':
   
    srch2ngnGitRepoDir = ""
    integrationTestDir = ""
    testBinaryDir = ""
    testBinaryFileName = ""
    nodesInfoFilePath = ""

    confFilePath = "config.json"
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

    fullPath = testBinaryDir + testBinaryFileName
    print str(fullPath)
    pid = commands.getstatusoutput('pgrep -f ' + str(fullPath))
    processId = pid[1].split()
    for i in range(len(processId)):
        killNodesOutput = commands.getstatusoutput('kill -9 ' + processId[i])
        err2 = os.system('rm -rf SRCH2_Cluster/node-*')
        print killNodesOutput 

