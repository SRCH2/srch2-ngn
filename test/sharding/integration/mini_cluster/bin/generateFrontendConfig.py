#!/usr/bin/python

import sys
import os

ipAddress=sys.argv[1]
portBase=sys.argv[2]
numberOfNodes=int(sys.argv[3])

### python $INSTALL_DIR/$__BIN_DIR/generateFrontendConfig.py $__IP_ADDRESS $EXT_PORT_BASE $__GROUP_PROCESS_COUNT > $INSTALL_DIR/$__BIN_DIR/$FRONTEND_CONF_FILE

portsInfo="ports|"
hostsInfo="hostnames|"
i = 0
while i < numberOfNodes:
   if i > 0 :
      portsInfo=portsInfo+' '
      hostsInfo=hostsInfo+' '
   portsInfo=portsInfo+str(int(portBase)+i)
   hostsInfo=hostsInfo+ipAddress
   i=i+1
print portsInfo
print hostsInfo
