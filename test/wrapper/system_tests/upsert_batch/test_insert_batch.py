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


class InsertTester:
    def __init__(self, binary_path):
        self.binaryPath = binary_path + '/srch2-search-server'
        self.startServerCommand = self.binaryPath + ' --config-file=./upsert_batch/srch2-config.xml &'

    def startServer(self):
        os.popen('rm -rf ./upsert_batch/logs/')
	os.popen('rm -rf ./upsert_batch/indexes/')
        #print ('starting engine: {0}'.format(self.startServerCommand))
        os.popen(self.startServerCommand)
	#print 'server started'

    #make sure the server is started
    def pingServer(self):
        info = 'curl -s http://localhost:8087/search?q=march | grep -q results'
        while os.system(info) != 0:
            time.sleep(1)
            info = 'curl -s http://localhost:8087/search?q=march | grep -q results'
        #print 'server is built!'

    def killServer(self):
        """
        kills the server
        """
        try:
            #print ("killing srch2 server")
            s = commands.getoutput('ps aux | grep srch2 | grep config')
            stat = s.split()
            #print '1 ' + stat[1]
            os.kill(int(stat[1]), signal.SIGUSR1)
            #print ("server killed!")
        except Exception, err:
            print "Kill server exception: " + str(err)
            try:
                s = commands.getoutput("ps -A | fgrep -v '<defunct>' | grep -m1 srch2 | awk '{print $1}'")
                a = s.split()
                cmd = "kill {0}".format(a[-1])
                os.system(cmd)
                #print ("server killed")
            except:
                print "no running instance found to kill, moving ahead."

    #fire a single query
    def fireQuery(self, query):
        # Method 1 using urllib2.urlopen
        #queryCommand = 'http://localhost:8087/search?q=' + query
        #urllib2.urlopen(queryCommand)
        #print 'fired query ' + query
        # Method 2 using curl
        curlCommand = 'curl -s http://localhost:8087/search?q=' + query
        os.popen(curlCommand)


if __name__ == '__main__':

    binary_path = sys.argv[1]

    tester = InsertTester(binary_path)
    tester.startServer()
    tester.pingServer()

    # test 1, insert a record that already exists
    command1 = 'curl "http://localhost:8087/docs" -i -X PUT -d ' + '\'{"id":"101","post_type_id":"2","parent_id":"6272262","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"3","view_count":"0","body":"february monday","owner_user_id":"356674","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"}\'';
    status1, output1 = commands.getstatusoutput(command1)
    #print 'output1 --- ' + str(output1) + "\n-----------------";
    flag = str(output1).find('{"rid":"101","insert":"failed","reason":"The record with same primary key already exists"}');
    #print flag
    assert flag > -1, 'Error, rid 101 is not updated correctly!'

    # test 2, insert a record that doesn't exist
    command3 = 'curl "http://localhost:8087/docs" -i -X PUT -d ' + '\'{"id":"111","post_type_id":"2","parent_id":"6272262","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"3","view_count":"0","body":"december","owner_user_id":"356674","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"}\'';

    status3, output3 = commands.getstatusoutput(command3)
    #print 'output3 -----' + output3 + '\n-----------------'
    flag = str(output3).find('{"rid":"111","insert":"success"}');
    assert flag > -1, 'Error, rid 111 is not updated correctly!'

    time.sleep(1.5)
    command4 = 'curl -i http://localhost:8087/search?q=december'
    status4, output4 = commands.getstatusoutput(command4)
    #print 'output4 -----' + output4 + '\n----------------'
    flag = str(output4).find('"id":"111"')
    assert flag > -1, 'Error, rid 111 is not updated correctly!'


    # test 3, upsert multiple records, some exsit some don't
    command5 = 'curl "http://localhost:8087/docs" -i -X PUT -d ' + '\'[{"id":"102","post_type_id":"2","parent_id":"6271537","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"0","view_count":"0","body":"march wonderful","owner_user_id":"274589","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"},{"id":"103","post_type_id":"2","parent_id":"6272327","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"6","view_count":"0","body":"april wonderful","owner_user_id":"597122","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"2","favorite_count":"NULL"},{"id":"115","post_type_id":"2","parent_id":"6272162","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"0","view_count":"0","body":"june wonderful","owner_user_id":"430087","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"},{"id":"116","post_type_id":"2","parent_id":"6272210","accepted_answer_id":"NULL","creation_date":"06/07/2011","score":"1","view_count":"0","body":"july wonderful","owner_user_id":"113716","last_editor_user_id":"NULL","last_editor_display_name":"NULL","last_edit_date":"NULL","last_activity_date":"06/07/2011","community_owned_date":"NULL","closed_date":"NULL","title":"NULL","tags":"NULL","answer_count":"NULL","comment_count":"NULL","favorite_count":"NULL"}]\''

    status5, output5 = commands.getstatusoutput(command5)
    #print 'output5 -----' + output5 + '\n-----------------'
    flag = str(output5).find('{"rid":"102","insert":"failed","reason":"The record with same primary key already exists"}');
    assert flag > -1, 'Error, rid 102 is not updated correctly!'
    flag = str(output5).find('{"rid":"103","insert":"failed","reason":"The record with same primary key already exists"}');
    assert flag > -1, 'Error, rid 103 is not updated correctly!'
    flag = str(output5).find('{"rid":"115","insert":"success"}');
    assert flag > -1, 'Error, rid 115 is not updated correctly!'
    flag = str(output5).find('{"rid":"116","insert":"success"}');
    assert flag > -1, 'Error, rid 116 is not updated correctly!'


    time.sleep(1.5)
    command6 = 'curl -i http://localhost:8087/search?q=wonderful'
    status6, output6 = commands.getstatusoutput(command6)
    #print 'output4 -----' + output4 + '\n----------------'
    flag = str(output6).find('"id":"115"')
    assert flag > -1, 'Error, rid 115 is not updated correctly!'
    flag = str(output6).find('"id":"116"')
    assert flag > -1, 'Error, rid 116 is not updated correctly!'


    time.sleep(3)
    tester.killServer()

    print '=====================Batch Insert Test Passed!=========================='
    os._exit(0)










