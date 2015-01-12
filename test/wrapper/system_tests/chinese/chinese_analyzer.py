#!/usr/bin/python
# -*- coding: utf-8 -*-
# This test is to test the Chinese language support.
# We need to set the fieldType name to "text_chinese" inside the schema configuration to 
# enable the Chinese Analyzer. And we need to provide the Chinese Dictionary to 
# tokenize the sentence. The conf.xml has all the details.
#
# This test uses the Chinese Analyzer to parse the record. The record is tokenized based
# on the Chinese Dictionary. Then we should be able to search all the records by the 
# Chinese keywords.

import sys, json, os, urllib
sys.path.insert(0, 'srch2lib')
import test_lib

def check_array_contains(results, docs):
    from sets import Set
    idset = Set([])
    for record in results:
        idset.add(record['record']['id'].encode('utf-8'))
    for doc in docs:
        if not str(doc) in idset:
            print doc , 'is not in the idset:', idset
            return -1
    return 0

def check_expected_hit(expected_hit):
    for hit in expected_hit:
        encoded = urllib.quote(hit[0])
        response = test_lib.searchRequest('q=' + encoded)
        ret = check_array_contains(response['results'], hit[1])
        if ret != 0:
            print "hit assert failed:", hit[0]
            return ret
    return 0


def testSearch():
    
    def testHit():
        expected_hit = [('曼联',[0,1,4]), 
                        ('皇马',[1,4]), 
                        ('意大利',[2,5,6]), 
                        ('胜利',[3]), 
                        ('豪赌',[1,4]), 
                        ('冠军',[5]), 
                        ('米兰',[7,8]),
                        ('利物浦',[9])]
        return check_expected_hit(expected_hit)

    def testQuerySentence():
        expected_hit = [('"孔蒂亲口点出意大利未来核心"',[5]),
                        ('"化身红色利物浦巨龙"', [9])]
        return check_expected_hit(expected_hit)

    def testPrefix():
        expected_hit = [('红*',[1,3,4,9])]
        return check_expected_hit(expected_hit)

    return testHit() + testQuerySentence() + testPrefix()

def testInsert():
    record = '{"id":10, "title":"1600万锋霸后还有强援！曝阿森纳免签世界杯队长铁卫"}'
    test_lib.insertRequest(record)
    from time import sleep
    sleep(0.5)
    expected_hit = [('阿森纳', [10])]
    return check_expected_hit(expected_hit)

if __name__ == '__main__':
    binary_path = sys.argv[1]

    import shutil
    datadir = './data/chinese'
    if os.path.isdir(datadir): shutil.rmtree(datadir) 

    serverHandle = test_lib.startServer([binary_path, './chinese/conf.xml','./chinese/conf-A.xml','./chinese/conf-B.xml'])
    if serverHandle == None:
        os._exit(-1)
    exit_code = 0

    try:
        exit_code  = testSearch()
        exit_code += testInsert()

    except Exception, e:
        print e
        exit_code = -1
    finally:
        test_lib.killServer(serverHandle)  

    if exit_code == 0:
        print '\033[92m'+ "Passed", '\033[0m'
    else:
        print '\033[91m'+ "Failed", '\033[0m'

    os._exit(exit_code)
