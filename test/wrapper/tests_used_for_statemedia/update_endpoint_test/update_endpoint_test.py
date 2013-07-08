import sys, urllib2, json, time

port = '8082'
base_url = 'http://localhost'

primary_key = 'index'
attr = 'attr'

new_attr_value = 'new attr zzz'

# Search the old record using old attribute value
response = urllib2.urlopen('http://localhost:' + port + '/search?q=Harry+Potter+12345').read()
response_json = json.loads(response)
old_record_key = response_json['results'][0]['record'][primary_key]
old_record_attr = response_json['results'][0]['record'][attr]

# Use the new record to update
new_record = '{"index":"01c90b4effb2353742080000","_id":"01c90b4effb2353742080000","attr":"new attr zzz","title":"Harry Potter","description":"Film series based on the seven Harry Potter novels by British author J.K. Rowling","primary_category":"/category/book/literary_series","relevance":0.3632313166195419,"uri":"/topic/harry-potter.1","avatar":"http://c234912.r12.cf1.rackcdn.com/a6d8303a14_4ee8c5c08bbc4056d80005d5.jpeg","parent":"01c90b4effb2353742080000"}'

opener = urllib2.build_opener(urllib2.HTTPHandler)
request = urllib2.Request(base_url + ':' + port + '/update?' + primary_key + '=01c90b4effb2353742080000', data = new_record)
request.add_header('Content-Type', 'application/json')
request.get_method = lambda: 'PUT'
resp = opener.open(request)

# Wait for merge
print 'record updated'
print 'wait 10 seconds ..'
print resp.read()
time.sleep(10)

# Search the new record using new attribute value
response = urllib2.urlopen('http://localhost:' + port + '/search?q=' + '+'.join(new_attr_value.split())).read()
response_json = json.loads(response)
new_record_attr = response_json['results'][0]['record'][attr]
new_record_key = response_json['results'][0]['record'][primary_key]

# Try the old attribute value
response = urllib2.urlopen('http://localhost:' + port + '/search?q=Harry+Potter+12345').read()
response_json = json.loads(response)
old_search_res = len(response_json['results'])

print 'old record key: ', old_record_key, ' old record attr: ', old_record_attr 
print 'new record key: ', new_record_key, ' new record attr: ', new_record_attr 

print '=============================='
if (old_record_key == new_record_key) and ((old_record_attr != new_record_attr) and (old_search_res == 0)):
    print "-------- Test Passes ---------"
else:
    print "-------- Test Fails ---------"
print '=============================='
