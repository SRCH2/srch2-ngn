# test process
# The orignal dataset has ten records, with id from '100' to '109', with body contents "january','february' ,....,'october'

# test case 1, insert record '101', which already exists 
# Check if the returned message has '"rid":"101","update":"Existing record updated successfully"'

# test case 2, insert record '111', which doesn't exist in the index. 
# check if the returned message contains {"rid":"111","update":"New record inserted successfully"}
# search for 'monday' to see if rid 101 is returned

# test case 3, update multiple records, some already exist, others don't. All of them contain word 'wonderful'
# check the returned message 
# search for 'wonderful' to see if all the new records are returned

import os, time, sys, commands, urllib2, signal
import json

sys.path.insert(0, 'srch2lib')
import test_lib

class InsertTester:
    def __init__(self, binary_path):
        self.args = [ binary_path, './upsert_batch/srch2-config.xml', './upsert_batch/srch2-config-A.xml', './upsert_batch/srch2-config-B.xml' ]

    def startServer(self):
        os.popen('rm -rf ./upsert_batch/logs/')
	os.popen('rm -rf ./upsert_batch/indexes/')
	os.popen('rm -rf ./data/upsert_batch/')
        #print ('starting engine: {0}'.format(self.startServerCommand))
        self.serverHandle = test_lib.startServer(self.args);
        if self.serverHandle == None:
            return -1

    def killServer(self):
        """
        kills the server
        """
        test_lib.killServer(self.serverHandle)

    #fire a single query
    def fireQuery(self, query):
        # Method 1 using urllib2.urlopen
        #queryCommand = 'http://127.0.0.1:8087/search?q=' + query
        #urllib2.urlopen(queryCommand)
        #print 'fired query ' + query
        # Method 2 using curl
        test_lib.searchRequest('q=' + query)

def verify_insert_response(expectedResponse, actualOutput):
    #actualOutput = actualOutput.splitlines()[-1]

    actualJson = (actualOutput)
    expectJson = json.loads(expectedResponse)
    actualJson = actualJson['items']
    for eachInsertResult in actualJson:
        match = True 
        for key in expectJson:
            if expectJson[key] != eachInsertResult[key]:
                match = False
                continue
        if match:
            return True
    return False

def verify_search_rid(expectedRID, actualOutput):
    #actualOutput = actualOutput.splitlines()[-1]
    print "ACT:" , str(actualOutput).splitlines()[-1]
    print "EXP:" , expectedRID
    return expectedRID in str(actualOutput).splitlines()[-1]

    
if __name__ == '__main__':

    binary_path = sys.argv[1]

    tester = InsertTester(binary_path)
    tester.startServer()

    # test 1, insert a record that already exists
    record = '{"id":"101","post_type_id":"2","parent_id":"6272262","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"3","view_count":"0","body":"february monday","owner_user_id":"356674","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"}'
    output1 = test_lib.insertRequest(record)

    #print 'output1 --- ' + str(output1) + "\n-----------------";
    expect = '{"action":"insert","coreName":"__DEFAULTCORE__","details":[{"error":"The record with same primary key already exists","message_code":5}],"rid":"101","status":false}';
    #print flag
    #assert verify_insert_response(expect, output1), 'Error, rid 101 is not updated correctly!'

    # test 2, insert a record that doesn't exist
    record = '{"id":"111","post_type_id":"2","parent_id":"6272262","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"3","view_count":"0","body":"december","owner_user_id":"356674","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"}'

    output3 = test_lib.insertRequest(record)
    #print 'output3 -----' + output3 + '\n-----------------'
    expect = '{"action":"insert","coreName":"__DEFAULTCORE__","details":[],"rid":"111","status":true}';
    assert verify_insert_response(expect, output3), 'Error, rid 111 is not updated correctly!'

    time.sleep(1.5)
    query = 'q=december'
    output4 = test_lib.searchRequest(query)
    #print 'output4 -----' + output4 + '\n----------------'
    expect = '111'
    assert verify_search_rid(expect, output4) , 'Error, rid 111 is not updated correctly!'


    # test 3, upsert multiple records, some exsit some don't
    records = '[{"id":"102","post_type_id":"2","parent_id":"6271537","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"0","view_count":"0","body":"march wonderful","owner_user_id":"274589","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"},{"id":"103","post_type_id":"2","parent_id":"6272327","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"6","view_count":"0","body":"april wonderful","owner_user_id":"597122","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"2","favorite_count":"NULL"},{"id":"115","post_type_id":"2","parent_id":"6272162","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"0","view_count":"0","body":"june wonderful","owner_user_id":"430087","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"},{"id":"116","post_type_id":"2","parent_id":"6272210","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"1","view_count":"0","body":"july wonderful","owner_user_id":"113716","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"}]'
    output5 = test_lib.insertRequest(records)
    #print 'output5 -----' + output5 + '\n-----------------'
    expect = '{"action":"insert","coreName":"__DEFAULTCORE__","details":[{"error":"The record with same primary key already exists","message_code":5}],"rid":"102","status":false}'
    assert verify_insert_response(expect, output5), 'Error, rid 102 is not updated correctly!'
    expect = '{"action":"insert","coreName":"__DEFAULTCORE__","details":[{"error":"The record with same primary key already exists","message_code":5}],"rid":"103","status":false}'
    assert verify_insert_response(expect, output5), 'Error, rid 103 is not updated correctly!'
    expect = '{"action":"insert","coreName":"__DEFAULTCORE__","details":[],"rid":"115","status":true}'
    assert verify_insert_response(expect, output5), 'Error, rid 115 is not updated correctly!'
    expect = '{"action":"insert","coreName":"__DEFAULTCORE__","details":[],"rid":"116","status":true}'
    assert verify_insert_response(expect, output5), 'Error, rid 116 is not updated correctly!'

    time.sleep(1.5)
    query = 'q=wonderful'
    output6 =  test_lib.searchRequest(query)
    #print 'output4 -----' + output4 + '\n----------------'
    expect = '115'
    assert verify_search_rid(expect, output6), 'Error, rid 115 is not updated correctly!'
    expect = '116'
    assert verify_search_rid(expect, output6), 'Error, rid 116 is not updated correctly!'


    time.sleep(3)
    tester.killServer()

    print '=====================Batch Insert Test Passed!=========================='

    #The test "batch insert" may take time to close the SRCH2 engine. 
    #If the engine is still running, starting another engine on the same port could cause error.
    #So we sleep several seconds to wait for the engine to shut down completely.
    time.sleep(3)

    os._exit(0)










