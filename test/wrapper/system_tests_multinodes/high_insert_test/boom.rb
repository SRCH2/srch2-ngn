require 'json'
require 'httparty'

module TestData
  def self.each_record
    File.open File.expand_path("../data-insert/index.json", __FILE__) do |f|
      f.each_line do |line|
        yield line.strip
      end
    end
  end
end

class Bimaple
  include HTTParty
  headers "Content-Type" => "application/json"

  def self.base_url
    "http://localhost:8087"
  end

  def self.insert(record)
    #puts record
    resp = put("#{base_url}/docs", :body => record)
    # puts resp
  end

  def self.remove(record)
    entity = JSON.parse(record)
    index = entity["index"]

    #puts index
    resp = delete("#{base_url}/docs", :query => {:index => index})
    # puts resp
  end

  def self.search(q)
    #puts q
    resp = get("#{base_url}/search", :query => {:q => q})
    # puts resp
  end

  def self.update(entity)
    self.remove(entity)
    self.insert(entity)
  end
end

keywords = %w(de der derek g gh ghost ghostbusters mike meyers state foo bar baz g god godfather thegodfather)

INDEX_NUM_THREADS = 10
index_threads = INDEX_NUM_THREADS.times.map do |i|
  puts "Starting index thread #{i}"
  Thread.new do
    TestData.each_record do |rec|
      Bimaple.update rec
       #puts "[#{i}] update"
    end
    puts "Indexing finished on thread #{i}"
  end
end


puts "Starting search"
100.times do
  kw = keywords.sample
  kw = "%7BdefaultPrefixComplete=COMPLETE%7D" + kw + "*"
  Bimaple.search(kw)
   #puts "search: #{kw}"
end
puts "Searching finished"

index_threads.each {|t| t.join }
