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
