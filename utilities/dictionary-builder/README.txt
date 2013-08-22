$Id$
#Introduction
This utility is a tool to convert the text format dictionary file into the binary version using Boost::Serialization method.

In order to maintain the dependency to the core-engine, we insert this utility's CMakefile inside the engine's CMakefile to avoid the complexity of compiling the srch2-core library twice. 

##To compile it:
```bash
 goto srch2-ngn directory
 mkdir build
 cd build
 cmake -DBUILD_UTILITY=1 ..
```

Then you will find the executable file at build/utilities/dictionary-builder/DictionaryBuilder

##To use it :
```bash
./DictionaryBuilder <InputTextFile> <OutputBinaryFile>
#To install the dictionary into release file:
./DictionaryBuilder /path/to/srch2-ngn/utilities/dictionary-builder/data/chinese.dict /path/to/srch2-ngn/release_files/srch2_data/srch2_dict_ch.core
```
#About the format of the text dictionary
This dictionary file contains the (word,log-value) pairs to store the words and their scores for the use of dynamic programming algorithm.

For each word (w), we use the following formular to calculate the log-value:
```
log-value(w) = log(0.985, probability(w))
```
