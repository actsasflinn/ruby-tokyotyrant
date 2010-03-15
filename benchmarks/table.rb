=begin
Tokyo::TyrantTable (Ruby FFI)
                          user     system      total        real
inserting data        0.840000   0.190000   1.030000 (  3.214565)
reading data          1.310000   0.210000   1.520000 (  3.024142)


TokyoTyrant::RDB (Ruby)
                          user     system      total        real
inserting data        1.170000   0.240000   1.410000 (  2.859321)
reading data          2.250000   0.660000   2.910000 (  4.752291)


TokyoTyrant (C)
                          user     system      total        real
inserting data        0.260000   0.180000   0.440000 (  1.637843)
reading data          0.300000   0.170000   0.470000 (  1.631104)


Memcached (C)
                          user     system      total        real
inserting data        0.300000   0.160000   0.460000 (  1.230830)
reading data          0.310000   0.140000   0.450000 (  1.171531)


MemCache (Ruby)
                          user     system      total        real
inserting data        2.080000   0.220000   2.300000 (  3.379047)
reading data          2.220000   0.200000   2.420000 (  4.018160)


Mongo
                          user     system      total        real
inserting data        1.590000   0.130000   1.720000 (  2.310027)
reading data          4.220000   0.380000   4.600000 (  7.178335)
=end

require 'benchmark'
require 'rubygems'
require 'faker'
require 'date'

#
# the data
#
 
colnames = %w{ _id name sex birthday divisions }
 
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

data = [
  ['alphonse-armalite', 'Alphonse Armalite', 'male', Date.new(1972, 10, 14).to_s, 'brd,dev'],
  ['brutus-beromunster', 'Brutus Beromunster', 'male', Date.new(1964, 07, 14).to_s, 'dev'],
  ['crystel-chucknorris', 'Crystel Chucknorris', 'female', Date.new(1980, 07, 12).to_s, 'brd'],
  ['desree-dylan', 'Desree Dylan', 'female', Date.new(1954, 07, 13).to_s, 'brd,dev']
]
 
10_000.times do |i|
  data << [i.to_s, Faker::Name.name, rgen, rbdate.to_s, rdiv]
end
 
$find_name_list = []
100.times { $find_name_list << data[rand(data.size)][0] }
 
data.collect! { |e|
  (0..colnames.length - 1).inject({}) { |h, i| h[colnames[i]] = e[i]; h }
}

data_h = {}
i = 0
data1 = data.collect { |e|
  i = i + 1
  h = e.dup
  h['birthday'] = h['birthday'].to_s
  data_h[i.to_s] = h
  h
}

require 'rufus/tokyo/tyrant'

r = Rufus::Tokyo::TyrantTable.new('127.0.0.1', 45001)
r.clear

2.times { puts }
puts 'Tokyo::TyrantTable (Ruby FFI)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data1.each_with_index { |e, i| r[i.to_s] = e }
  end

  b.report('reading data') do
    data1.each_with_index { |e, i| nothing = r[i.to_s] }
  end
end

require 'tokyotyrant'

rdb = TokyoTyrant::RDBTBL::new
rdb.open("127.0.0.1", 45001)
rdb.clear

2.times { puts }
puts 'TokyoTyrant::RDB (Ruby)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data1.each_with_index { |e, i| rdb.put(i.to_s, e) }
  end

  b.report('reading data') do
    data1.each_with_index { |e, i| nothing = rdb.get(i.to_s) }
  end
end

require 'tokyo_tyrant'
t = TokyoTyrant::Table.new('127.0.0.1', 45001)
t.clear

2.times { puts }
puts 'TokyoTyrant (C)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data1.each_with_index { |e, i| t[i] = e }
  end

  b.report('reading data') do
    data1.each_with_index { |e, i| nothing = t[i] }
  end
end

require 'memcached'
m = Memcached.new('127.0.0.1:11211')
m.flush

2.times { puts }
puts 'Memcached (C)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data1.each_with_index { |e, i| m.set(i.to_s, e) }
  end

  b.report('reading data') do
    data1.each_with_index { |e, i| nothing = m.get(i.to_s) }
  end
end

require 'memcache'
mc = MemCache.new('127.0.0.1:11211')
mc.flush_all

2.times { puts }
puts 'MemCache (Ruby)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data1.each_with_index { |e, i| mc.set(i.to_s, e) }
  end

  b.report('reading data') do
    data1.each_with_index { |e, i| nothing = mc.get(i.to_s) }
  end
end

require 'mongo'
db = Mongo::Connection.new.db("benchmark")
2.times { puts }
puts 'Mongo'
mdb = db["data"]
mdb.remove

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data1.each_with_index { |e, i| mdb.insert(e) }
  end

  b.report('reading data') do
    data1.each_with_index { |e, i| nothing = mdb.find_one({ "_id" => i.to_s }) }
  end
end
