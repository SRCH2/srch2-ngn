#!/usr/bin/python

import sys
import os

ipAddressesStr=sys.argv[1]
portValue=sys.argv[2]
confTemplateBaseName=sys.argv[3]
ipAddresses = ipAddressesStr.split()
wellKnownHosts=""
for ip_address in ipAddresses:
    wellKnownHosts = wellKnownHosts + ip_address + ":" + portValue + ","
wellKnownHosts = wellKnownHosts[:-1]

confTemplateBase = file(confTemplateBaseName, 'r')
for line in confTemplateBase:
    if "WELL-KNOWN-HOSTS_PLACE_HOLDER" in line:
        print "<WellKnownHosts>" + wellKnownHosts + "</WellKnownHosts>"
    else:
        print line,
   

