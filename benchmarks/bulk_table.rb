=begin
Tokyo Tyrant Bulk Table Operations Benchmark


TokyoTyrant (C)
                          user     system      total        real
bulk writing data     0.180000   0.000000   0.180000 (  0.292081)
bulk reading data     0.120000   0.020000   0.140000 (  0.216074)


TokyoTyrant::RDB (Ruby)
                          user     system      total        real
bulk inserting data* 42.410000   4.130000  46.540000 ( 63.657929)
bulk reading data     2.360000   0.280000   2.640000 (  3.709915)


Mongo
                          user     system      total        real
bulk writing data    27.860000   0.470000  28.330000 ( 37.619586)
bulk reading data     0.650000   0.100000   0.750000 (  1.371567)

Notes:
* To supply a hash for bulk operations TokyoTyrant::RDB creates an array and join hash columns.
* This operation is included in the benchmark because the same code exists within the c-ext.
Rufus::Tokyo::TyrantTable does not support the misc method, therefor cannot handle a putlist/getlist
Memcached does not support bulk writing
=end

require 'benchmark'
require 'rubygems'
require 'faker'
require 'date'

puts "Tokyo Tyrant Bulk Table Operations Benchmark"

$year = (1909 .. 2009).to_a
$month = (1..12).to_a
$day = (1..28).to_a # not bothering with month diffs
 
def rbdate
  Date.new($year[rand($year.size)], $month[rand($month.size)], $day[rand($day.size)])
end
 
def rdiv
  case rand(3)
  when 0
    'dev'
  when 1
    'brd'
  else
    'brd,dev'
  end
end

def rgen
  (rand(2) == 1 ? 'male' : 'female')
end

data = {}
10_000.times do |i|
  data[i.to_s] = { :_id => i.to_s, :name => Faker::Name.name, :sex => rgen, :birthday => rbdate.to_s, :divisions => rdiv }
end

data_keys = data.keys
data_values = data.values

require 'tokyo_tyrant'
t = TokyoTyrant::Table.new('127.0.0.1', 45001)
t.clear

2.times { puts }
puts 'TokyoTyrant (C)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('bulk writing data') do
    nothing = t.mput(data)
  end

  b.report('bulk reading data') do
    nothing = t.mget(data_keys)
  end
end

require 'tokyotyrant'

rdb = TokyoTyrant::RDB::new
rdb.open("127.0.0.1", 45001)
rdb.clear

2.times { puts }
puts 'TokyoTyrant::RDB (Ruby)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('bulk inserting data*') do
    # is this fair to put in the benchmark? yes because it happens within the c-ext
    data_a = []
    data.each_pair{ |i,v|
      data_a << i.to_s
      data_a << v.collect{ |k,v| [k.to_s,v] }.join("\0")
    }
    rdb.misc('putlist', data_a)
  end

  b.report('bulk reading data ') do
    l = rdb.misc('getlist', data.keys)
    h = Hash[*l]
    h.each_pair do |k,v|
      a = v.split("\0")
      h[k] = Hash[*a]
    end
  end
end

require 'mongo'
db = Mongo::Connection.new.db("benchmark")
2.times { puts }
puts 'Mongo'
mdb = db["data"]
mdb.remove

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('bulk writing data') do
    nothing = mdb.insert(data_values)
  end

  b.report('bulk reading data') do
    nothing = mdb.find(:_id => {'$in' => data_keys}).to_a
  end
end

puts "\nNotes:"
puts "* To supply a hash for bulk operations TokyoTyrant::RDB creates an array and join hash columns."
puts "* This operation is included in the benchmark because the same code exists within the c-ext."
puts "Rufus::Tokyo::TyrantTable does not support the misc method, therefor cannot handle a putlist/getlist"
puts "Memcached does not support bulk writing"
