import commands
if __name__ == '__main__':
    pid = commands.getstatusoutput('pgrep -f ../../../build/src/server/srch2-search-server')
    processId = pid[1].split()
    for i in range(len(processId)-1):
        killNodesOutput = commands.getstatusoutput('kill -9 ' + processId[i])
        print killNodesOutput 

