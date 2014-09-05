#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, json, os, urllib
sys.path.insert(0, 'srch2lib')
from test_lib import *

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

def check_expected_hit(expected_hit, search_url):
    for hit in expected_hit:
        encoded = urllib.quote(hit[0])
        response = open_url_get(search_url + encoded)
        ret = check_array_contains(response['results'], hit[1])
        if ret != 0:
            print "hit assert failed:", hit
            return ret
    return 0


def testSearch(host_url):
    search_url = host_url + '/search?q='
    
    def testHit():
        expected_hit = [('曼联',[0,1,4]), 
                        ('皇马',[1,4]), 
                        ('意大利',[2,5,6]), 
                        ('胜利',[3]), 
                        ('豪赌',[1,4]), 
                        ('冠军',[5]), 
                        ('米兰',[7,8]),
                        ('利物浦',[9])]
        return check_expected_hit(expected_hit, search_url)

    def testQuerySentence():
        expected_hit = [('"孔蒂亲口点出意大利未来核心"',[5]),
                        ('"化身红色利物浦巨龙"', [9])]
        return check_expected_hit(expected_hit, search_url)

    def testPrefix():
        expected_hit = [('红*',[1,3,4,9])]
        return check_expected_hit(expected_hit, search_url)

    return testHit() + testQuerySentence() + testPrefix()

def testInsert(host_url):
    insert_url = host_url + '/docs'
    search_url = host_url + '/search?q='
    record = '{"id":10, "title":"1600万锋霸后还有强援！曝阿森纳免签世界杯队长铁卫"}'
    response = open_url_put(insert_url, record)
    from time import sleep
    sleep(0.5)
    expected_hit = [('阿森纳', [10])]
    return check_expected_hit(expected_hit, search_url)

if __name__ == '__main__':
    binary_path = sys.argv[1]
    conf = './chinese/conf.xml'
    port = detectPort(conf)
    if confirmPortAvailable(port) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        os._exit(-1)

    import shutil
    datadir = './data/chinese'
    if os.path.isdir(datadir): shutil.rmtree(datadir) 

    serverHandle = startServer([binary_path, '--config=' + conf]) 
    host_url = 'http://127.0.0.1:' + str(port) 
    pingServer(port)
    
    print 'starting engine: ' + host_url + ' ' + conf
    exit_code = 0

    try:
        exit_code  = testSearch(host_url)
        exit_code += testInsert(host_url)

    except Exception, e:
        print e
        exit_code = -1
    finally:
        serverHandle.kill()

    if exit_code == 0:
        print '\033[92m'+ "Passed", '\033[0m'
    else:
        print '\033[91m'+ "Failed", '\033[0m'

    os._exit(exit_code)
