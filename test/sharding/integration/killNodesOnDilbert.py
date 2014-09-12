import paramiko, sys
if __name__ == '__main__':
  sshClient = paramiko.SSHClient()
  sshClient.set_missing_host_key_policy(paramiko.AutoAddPolicy())
  sshClient.connect("dilbert.calit2.uci.edu")
  stdin, stdout, stderr = sshClient.exec_command('cd gitrepo/srch2-ngn/test/sharding/integration; python killNodes.py ' + sys.argv[1])
