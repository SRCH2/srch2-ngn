#!/usr/bin/python

import sys
import os

template=sys.argv[1]
client_idx=sys.argv[2]
srch2Home=sys.argv[3]
dataFileRelPath=sys.argv[4]
logFileRelPath=sys.argv[5]


clusterName=os.environ['CLUSTER_NAME']
nodeName=os.environ['NODE_NAME_PREFIX']+str(client_idx)
basePort=os.environ['TM_PORT_BASE']
ipAddress=os.environ['TM_IP_ADDRESS']
wellknownHosts=os.environ['WELL_KNOWN_HOSTS']
listeningHostname=os.environ['EXT_HOSTNAME']
baseInternalPort=os.environ['EXT_PORT_BASE']
licenseFileName=os.environ['LICENSE_FILE']
numPartitions=os.environ['NUM_PARTITIONS']
numReplicas=os.environ['NUM_REPLICAS']
stopWordsFileName=os.environ['STOP_WORDS']
protectedWordsFileName=os.environ['PROTECTED_WORDS']

templateFile = open(template, 'r')

for line in templateFile:
   if "SRCH2_CLUSTER_PLACE_HOLDER" in line:
      print "    <cluster-name>"+clusterName+"</cluster-name>"
   elif "SRCH2HOME_PLACE_HOLDER" in line:
      print "    <srch2Home>"+srch2Home+"</srch2Home>"
   elif "NODE_NAME_PLACE_HOLDER" in line:
      print "    <node-name>"+nodeName+"</node-name>" 
   elif "PING_INTERVAL_PLACE_HOLDER" in line:
      print line,
   elif "PING_TIMEOUT_PLACE_HOLDER" in line:
      print line,
   elif "RETRY_COUNT_PLACE_HOLDER" in line:
      print line,
   elif "TM-PORT_PLACE_HOLDER" in line:
      print "        <port>"+str(int(basePort) + int(client_idx))+"</port>"
   elif "IPADDRESS_PLACE_HOLDER" in line:
      print "        <ipaddress>"+ipAddress+"</ipaddress>"
   elif "WELL-KNOWN-HOSTS_PLACE_HOLDER" in line:
      print "    <WellKnownHosts>"+wellknownHosts+"</WellKnownHosts>"
   elif "LISTENING-HOSTNAME_PLACE_HOLDER" in line:
      print "    <listeningHostname>"+listeningHostname+"</listeningHostname>"
   elif "LISTENING-PORT_PLACE_HOLDER" in line:
      print "    <listeningPort>"+str(int(baseInternalPort) + int(client_idx))+"</listeningPort>"
   elif "LICENSE-FILE_PLACE_HOLDER" in line:
      print "    <licenseFile>"+licenseFileName+"</licenseFile>" 
   elif "CORE-NUM_SHARDS_PLACE_HOLDER" in line:
      print "            <core-number_of_shards>"+str(numPartitions)+"</core-number_of_shards>"
   elif "CORE-NUM_REPLICAS_PLACE_HOLDER" in line:
      print "            <core-number_of_replicas>"+str(numReplicas)+"</core-number_of_replicas>"
   elif "STOP_WORDS_PLACE_HOLDER" in line:
      print "                        <filter name=\"StopFilter\" words=\""+stopWordsFileName+"\" />"
   elif "PROTECTED_WORDS_PLACE_HOLDER" in line:
      print "                        <filter name=\"protectedKeyWordsFilter\" words=\""+protectedWordsFileName+"\" />"
   elif "DATA-FILE_PLACE_HOLDER" in line:
      print "            <dataFile>"+dataFileRelPath+"</dataFile>" 
   elif "ACCESS-LOG-FILE_PLACE_HOLDER" in line:
      print "           <accessLogFile>"+logFileRelPath+"</accessLogFile>"
   else:
      print line,
