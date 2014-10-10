#To kill the engine
#Release mode
#python killNodes.py release
#Debug mode
#python killNodes.py debug

import commands,sys

if __name__ == '__main__':
    
    pid = commands.getstatusoutput('pgrep -f distributedTestFramework.py')
    processId = pid[1].split()
    for i in range(len(processId)):
        print processId[i] 
        killNodesOutput = commands.getstatusoutput('kill -9 ' + processId[i])
        print killNodesOutput 

