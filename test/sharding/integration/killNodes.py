#To kill the engine
#Release mode
#python killNodes.py release
#Debug mode
#python killNodes.py debug

import commands,sys

if __name__ == '__main__':
    pid = commands.getstatusoutput('pgrep -f ../../../build/src/server/srch2-search-server-'+sys.argv[1])
    processId = pid[1].split()
    for i in range(len(processId)-1):
        killNodesOutput = commands.getstatusoutput('kill -9 ' + processId[i])
        print killNodesOutput 

