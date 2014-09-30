import paramiko, sys, json
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
  print str(integrationTestDir)

  sshClient = paramiko.SSHClient()
  sshClient.set_missing_host_key_policy(paramiko.AutoAddPolicy())
  sshClient.connect("dilbert.calit2.uci.edu")
  stdin, stdout, stderr = sshClient.exec_command('cd ' + str(integrationTestDir) + '; python killNodes.py')
  stdin, stdout, stderr = sshClient.exec_command('cd ' + str(integrationTestDir) + ';rm -rf SRCH2_Cluster/node-*')
