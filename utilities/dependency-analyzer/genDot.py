# @author: Surnedra
# This program recursively parses all C++ header and code file in a 
# given directory and extract their depencencies on each other.
# The dependency info is stored in a text file in a format that is 
# recognized by the "dot" program (GraphViz)
# 
# Input : Path to src
# Output: .dot file
#
# Note: The program makes few assumptions which are not documented.
# However, the code is small and can be easily understood and modified

import os
import sys

if (len(sys.argv) < 3):
	print 'Usage: genDot.py <src_dir> <dot_file_path>'
	sys.exit(-1)
	 
rootdir = sys.argv[1]
depGraphOutFile = sys.argv[2]


dGraph = {}
fileList = []
extList = ['.h', '.hxx', '.cpp', '.c', '.cxx', '.hpp', '.cc']

for root, subFolders, files in os.walk(rootdir):
    if '.svn' in subFolders:
        subFolders.remove('.svn')
    for f in files:
		ext = f[f.rfind('.'):]
		if ext in extList:
			fp = os.path.join(root, f)
			fileList.append(fp)

#print fileList
#parse each file and build graph

line = []
for f in fileList:
	fd = open( f, 'r')
	lines = fd.readlines()
	fd.close()
	rn = f[f.rfind('/')+1:]
	rn = rn.replace('.' , '_')
	dGraph[rn] = []	
	for l in lines:
		l = l.strip('\n')
		l = l.strip('\r')
		l = l.strip(' ')
		if l.find('#include') == 0 and l.find('"') > 0:
			if l.rfind('/') > 0:
				cn = l[l.rfind('/')+1:].strip('"')
			else:
				cn = l[len('#include')+1:].strip(' ').strip('"')
			cn = cn.replace('.','_')
			dGraph[rn].append(cn)
#print dGraph

#dump graph to .DOT format
df = open(depGraphOutFile, "w")
df.write('digraph G {\n')
for k in dGraph:
    nodes = dGraph[k]
    for n in nodes:
        df.write(k + ' -> ' + n + ';\n')
df.write('}\n')
df.close()


