currentDir=`pwd`
cd ~/workspace-srch2-v4/srch2-ngn/build;
cmake ..;
make -j 8;
cd ~/workspace-srch2-1/srch2-ngn/build;
cmake ..;
make -j 8;
cd ~/workspace-srch2-2/srch2-ngn/build;
cmake ..;
make -j 8;
cd ~/workspace-srch2-3/srch2-ngn/build;
cmake ..;
make -j 8;
cd $currentDir
