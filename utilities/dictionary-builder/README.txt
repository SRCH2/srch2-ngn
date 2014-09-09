
#Introduction
Jianfeng Jia and Chen Li
Sept. 2014

This utility tool converts a text file with word frequencies to
a binary file using Boost::Serialization method. 

We insert this utility's CMakefile inside the engine's CMakefile to
avoid the complexity of compiling the srch2-core library twice.  

##To compile it:
```bash
 shell> cd srch2-ngn
 shell> mkdir build
 shell> cd build
 shell> cmake -DBUILD_UTILITY=1 ..
 shell> make
```

The executable file is build/utilities/dictionary-builder/DictionaryBuilder.

##To use it :
```bash
./DictionaryBuilder <InputTextFile> <OutputBinaryFile>
```

For instance, the following command generates the binary file used by
the ctest case "Analyzer_Test":

```bash
build$ utilities/dictionary-builder/DictionaryBuilder \
 ../utilities/dictionary-builder/data/chinese.dict ../test/core/unit/test_data/analyzer/srch2_dictionary_zh_cn.bin 

#The following command generates the dictionary binary in the release folder:

```bash
build$ utilities/dictionary-builder/DictionaryBuilder \
 ../utilities/dictionary-builder/data/chinese.dict ../release_files/srch2_data/srch2_data/srch2_dictionary_zh_cn.bin
```
#About the format of the text dictionary

This dictionary file contains the (word,log-value) pairs to store the
words and their scores for the use of dynamic programming algorithm.
For each word (w), we use the following formula to calculate the
log-value:
```
log-value(w) = log(0.985, probability(w))
```
