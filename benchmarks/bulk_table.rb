require 'benchmark'
require 'rubygems'
require 'faker'
require 'date'

puts "Tokyo Tyrant Bulk Table Operations Benchmark"

$year = (1909 .. 2009).to_a
$month = (1..12).to_a
$day = (1..28).to_a # not bothering with month diffs
 
def rbdate
  DateTime.new($year[rand($year.size)], $month[rand($month.size)], $day[rand($day.size)])
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
  data[i] = { :name => Faker::Name.name, :sex => rgen, :birthday => rbdate.to_s, :divisions => rdiv }
end

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
    nothing = t.mget(0..9999)
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

puts "\nNotes:"
puts "* To supply a hash for bulk operations TokyoTyrant::RDB creates an array and join hash columns."
puts "* This operation is included in the benchmark because the same code exists within the c-ext."
puts "Rufus::Tokyo::TyrantTable does not support the misc method, therefor cannot handle a putlist/getlist"
puts "Memcached does not support bulk writing"
