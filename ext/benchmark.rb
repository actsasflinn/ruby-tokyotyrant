require 'benchmark'
require 'rubygems'
require 'faker'
require 'date'

#
# the data
#
 
colnames = %w{ name sex birthday divisions }
 
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
 
data = [
  [ 'Alphonse Armalite', 'male', DateTime.new(1972, 10, 14), 'brd,dev' ],
  [ 'Brutus Beromunster', 'male', DateTime.new(1964, 07, 14), 'dev' ],
  [ 'Crystel Chucknorris', 'female', DateTime.new(1980, 07, 12), 'brd' ],
  [ 'Desree Dylan', 'female', DateTime.new(1954, 07, 13), 'brd,dev' ]
]
 
10_000.times do |i|
  data << [ Faker::Name.name, rgen, rbdate, rdiv]  
end
 
$find_name_list = []
100.times { $find_name_list << data[rand(data.size)][0] }
 
data.collect! { |e|
  (0..colnames.length - 1).inject({}) { |h, i| h[colnames[i]] = e[i]; h }
}
 
data1 = data.collect { |e|
  h = e.dup
  h['birthday'] = h['birthday'].to_s
  h
}

require 'rufus/tokyo/tyrant'

r = Rufus::Tokyo::TyrantTable.new('127.0.0.1', 1978)
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
rdb.open("127.0.0.1", 1978)
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
t = TokyoTyrant::Table.new('127.0.0.1', 1978)
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
