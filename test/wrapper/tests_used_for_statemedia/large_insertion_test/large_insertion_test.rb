require 'rubygems'
require 'httparty'
require 'pp'
require 'json'

class Srch2Client
  include HTTParty
  base_uri "http://localhost:8082"

  def self.query( query, limit=10 )
    get( '/search', :query => { :q => query, :start => 0, :limit => limit, :fuzzy => 1, :type => 0 } )
  end


  def self.insert( data )
    put( '/docs', :body => data )
  end

  def self.remove( data )
    delete( '/docs', :query => data )
  end
end

puts 'Testing result ordering'
limit_arr = [5]
responses = {}

=begin
limit_arr.each { |lim|
	pp 'Querying srch2 with ' + lim.to_s
	resp = Srch2Client.query( 'apple', lim )
	responses[lim] = [] 
	resp['results'].each { |r| responses[lim] << r['record']['attr'] }
}

pp responses
=end

puts 'Testing index insert'
query_arr = []
counter = 0
File.foreach('data/update.json') { |data|
  
  data_h = JSON.parse(data)
#  puts data_h.inspect
  Srch2Client.insert(data)
#puts 'inserted ' + data_h['title']
  query_arr << data_h['attr']
  #sleep(1)
  if counter % 10000 == 0 then
      puts counter
  end
  counter = counter + 1
}

puts 'Inserted ' + counter.to_s + ' records in totall'
sleep(10)

#exit

puts 'Now searching for them'
search_counter_insert = 0
query_arr.each do |qr|
#puts '----------------------------------- Searching for: ' + qr
  resp = Srch2Client.query(qr)
  if resp['results'] != nil then
    if resp['results'].length == 0 then
      search_counter_insert = search_counter_insert + 1
      puts 'Searching for inserted: ' + qr + ' ...... does not have results'
    end
#    resp['results'].each { |r| 
#      puts r['record']['attr']
#      puts r['record']['index']
#    }
  end
end
puts search_counter_insert.to_s + ' out of ' + counter.to_s + ' search does not have results'

puts 'Testing index delete'
File.foreach('data/update.json') { |data|

  data_h = JSON.parse(data)
#puts 'Removing ' + data_h['index']
  Srch2Client.remove({'index' => data_h['index']})
}

sleep(10)

puts 'Now searching for them again'
search_counter_delete = 0
query_arr.each do |qr|
#  puts '----------------------------------- Searching for: ' + qr
  resp = Srch2Client.query(qr)
  if resp['results'] != nil then
    if resp['results'].length != 0 then
      search_counter_delete = search_counter_delete + 1
      #puts 'Searching for deleted: ' + qr + ' ...... have results'
    end
#    resp['results'].each { |r|
#      puts r['record']['attr']
#      puts r['record']['index']
#    }
  end
end
puts search_counter_delete.to_s + ' out of ' + counter.to_s + ' search have results'

puts '=============================='
if search_counter_insert == 0 and search_counter_delete == 0
   puts '-------- Test Passes ---------'
else
   puts '-------- Test Fails ----------'
end
puts '=============================='
