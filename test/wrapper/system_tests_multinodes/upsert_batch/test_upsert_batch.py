# test process
# The orignal dataset has ten records, with id from '100' to '109', with body contents "january','february' ,....,'october'

# test case 1, update the body of record '101', which already exists, to "february monday" 
# Check if the returned message has '"rid":"101","update":"Existing record updated successfully"'
# search for 'monday' to see if rid 101 is returned

# test case 2, update record '111', which doesn't exist in the index. The new record contains word 'december'
# check if the returned message contains {"rid":"111","update":"New record inserted successfully"}
# search for 'december' to find rid 111

# test case 3, update multiple records, some already exist, others don't. All of them contain word 'wonderful'
# check the returned message 
# search for 'wonderful' to see if all the new records are returned

import os, time, sys, commands, urllib2, signal

sys.path.insert(0, 'srch2lib')
import test_lib
from test_insert_batch import verify_insert_response
from test_insert_batch import verify_search_rid

class UpsertTester:
    def __init__(self, binary_path):
        self.binaryPath = binary_path
        self.startServerCommand = [ self.binaryPath, './upsert_batch/srch2-config.xml', './upsert_batch/srch2-config-A.xml', './upsert_batch/srch2-config-B.xml' ]

    def startServer(self):
        os.popen('rm -rf ./upsert_batch/logs/')
	os.popen('rm -rf ./upsert_batch/indexes/')
	os.popen('rm -rf ./data/upsert_batch')
        self.serverHandle = test_lib.startServer(self.startServerCommand);
        if self.serverHandle == None:
            return -1

    def killServer(self):
        test_lib.killServer(self.serverHandle)

    #fire a single query
    def fireQuery(self, query):
        # Method 1 using urllib2.urlopen
        #queryCommand = 'http://127.0.0.1:8087/search?q=' + query
        #urllib2.urlopen(queryCommand)
        #print 'fired query ' + query
        # Method 2 using curl
        test_lib.searchRequest('q=' + query)

if __name__ == '__main__':

    binary_path = sys.argv[1]

    tester = UpsertTester(binary_path)
    tester.startServer()

    # test 1, upsert a record that already exists
    record = '{"id":"101","post_type_id":"2","parent_id":"6272262","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"3","view_count":"0","body":"february monday","owner_user_id":"356674","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"}'
    output1 = test_lib.updateRequeset(record)
    #print 'output1 --- ' + str(output1) + "\n-----------------";
    expect = '{"action":"update","coreName":"__DEFAULTCORE__","details":[],"rid":"101","status":true}'
    #print flag
    assert verify_insert_response(expect, output1), 'Error, rid 101 is not updated correctly!'

    time.sleep(1.5)
    query = 'q=monday'
    output2 = test_lib.searchRequest(query)
    #print 'output2 --- ' + str(output2) + '\n------------------'
    expect = '101';
    #print flag
    # can't find the new result untill several tries
    assert verify_search_rid(expect, output2), 'Error, id 101 is not updated correctly!'


    # test 2, upsert a record that doesn't exist
    record = '{"id":"111","post_type_id":"2","parent_id":"6272262","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"3","view_count":"0","body":"december","owner_user_id":"356674","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"}'

    output3 = test_lib.updateRequeset(record)
    #print 'output3 -----' + output3 + '\n-----------------'
    expect = '{"action":"update","coreName":"__DEFAULTCORE__","details":[{"info":"Existing record updated successfully.","message_code":7}],"rid":"111","status":true}'
    assert verify_insert_response(expect, output3), 'Error, rid 111 is not updated correctly!'

    time.sleep(1.5)
    query = 'q=december'
    output4 = test_lib.searchRequest(query)
    #print 'output4 -----' + output4 + '\n----------------'
    expect = '111'
    assert verify_search_rid(expect, output4), 'Error, rid 111 is not updated correctly!'


    # test 3, upsert multiple records, some exsit some don't
    records = '[{"id":"102","post_type_id":"2","parent_id":"6271537","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"0","view_count":"0","body":"march wonderful","owner_user_id":"274589","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"},{"id":"103","post_type_id":"2","parent_id":"6272327","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"6","view_count":"0","body":"april wonderful","owner_user_id":"597122","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"2","favorite_count":"NULL"},{"id":"115","post_type_id":"2","parent_id":"6272162","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"0","view_count":"0","body":"june wonderful","owner_user_id":"430087","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"},{"id":"116","post_type_id":"2","parent_id":"6272210","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"1","view_count":"0","body":"july wonderful","owner_user_id":"113716","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"}]'
    output5 = test_lib.updateRequeset(records)
    #print 'output5 -----' + output5 + '\n-----------------'
    expect = '{"action":"update","coreName":"__DEFAULTCORE__","details":[],"rid":"102","status":true}'
    assert verify_insert_response(expect, output5), 'Error, rid 102 is not updated correctly!'
    expect = '{"action":"update","coreName":"__DEFAULTCORE__","details":[],"rid":"103","status":true}'
    assert verify_insert_response(expect, output5), 'Error, rid 103 is not updated correctly!'
    expect = '{"action":"update","coreName":"__DEFAULTCORE__","details":[{"info":"Existing record updated successfully.","message_code":7}],"rid":"115","status":true}'
    assert verify_insert_response(expect, output5), 'Error, rid 115 is not updated correctly!'
    expect = '{"action":"update","coreName":"__DEFAULTCORE__","details":[{"info":"Existing record updated successfully.","message_code":7}],"rid":"116","status":true}'
    assert verify_insert_response(expect, output5), 'Error, rid 116 is not updated correctly!'


    time.sleep(1.5)
    query = 'q=wonderful'
    output6 = test_lib.searchRequest(query)
    #print 'output4 -----' + output4 + '\n----------------'
    expect = '102'
    assert verify_search_rid(expect, output6), 'Error, rid 102 is not updated correctly!'
    expect = '103'
    assert verify_search_rid(expect, output6), 'Error, rid 103 is not updated correctly!'
    expect = '115'
    assert verify_search_rid(expect, output6), 'Error, rid 115 is not updated correctly!'
    expect = '116'
    assert verify_search_rid(expect, output6), 'Error, rid 116 is not updated correctly!'



    time.sleep(3)
    tester.killServer()

    print '=====================Batch Upsert Test Passed!=========================='
    os._exit(0)

