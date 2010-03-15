=begin
Tokyo Tyrant Bulk Operations Benchmark


TokyoTyrant::RDB (Ruby) mget
                          user     system      total        real
inserting data        7.770000   1.870000   9.640000 ( 12.598373)
reading data          1.540000   0.250000   1.790000 (  2.290628)


TokyoTyrant (c) mput/mget
                          user     system      total        real
inserting data        0.020000   0.000000   0.020000 (  0.037996)
reading data          0.060000   0.010000   0.070000 (  0.120651)


Memcached (C) set/get_multi
                          user     system      total        real
inserting data*       0.160000   0.140000   0.300000 (  0.884097)
reading data          0.090000   0.000000   0.090000 (  0.140217)
* bulk operation not supported
=end

require 'benchmark'
require 'rubygems'
require 'faker'

puts "Tokyo Tyrant Bulk Operations Benchmark"

data   = {}
data_e = {}
data_a = []

10_000.times do |i|
  data[i.to_s]   = Faker::Name.name
  data_e[i.to_s] = nil
  data_a << i.to_s
  data_a << data[i.to_s]
end

require 'tokyotyrant'

rdb = TokyoTyrant::RDB::new
rdb.open("127.0.0.1", 45000)
rdb.clear
nothing = nil

2.times { puts }
puts 'TokyoTyrant::RDB (Ruby) mget'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    rdb.misc('putlist', data_a)
  end

  b.report('reading data') do
    rdb.mget(data_e)
  end
end

require 'tokyo_tyrant'
t = TokyoTyrant::DB.new('127.0.0.1', 45000)
t.clear
nothing = nil

2.times { puts }
puts 'TokyoTyrant (c) mput/mget'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data ') do
    t.mput(data)
  end

  b.report('reading data') do
    nothing = t.mget(0..9999)
  end
end

require 'memcached'
m = Memcached.new('127.0.0.1:11211')
m.flush
nothing = nil

2.times { puts }
puts 'Memcached (C) set/get_multi'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data*') do
    data.each_pair { |k, v| m.set(k, v) }
  end

  b.report('reading data') do
    nothing = m.get(data.keys)
  end
end

puts "* bulk operation not supported"
