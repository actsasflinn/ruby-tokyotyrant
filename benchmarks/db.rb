=begin
Tokyo::Tyrant (Ruby FFI)
                          user     system      total        real
inserting data        0.140000   0.180000   0.320000 (  1.208150)
reading data          0.240000   0.170000   0.410000 (  1.467640)


TokyoTyrant::RDB (Ruby)
                          user     system      total        real
inserting data        0.300000   0.170000   0.470000 (  1.481900)
reading data          0.410000   0.210000   0.620000 (  1.705723)


TokyoTyrant (C)
                          user     system      total        real
inserting data        0.130000   0.170000   0.300000 (  1.251842)
reading data          0.120000   0.160000   0.280000 (  1.539256)


Memcached (C) to Tyrant
                          user     system      total        real
inserting data        0.200000   0.150000   0.350000 (  1.814691)
reading data          0.180000   0.160000   0.340000 (  1.358712)


Memcached (C) to Memcached
                          user     system      total        real
inserting data        0.190000   0.140000   0.330000 (  1.022183)
reading data          0.160000   0.140000   0.300000 (  0.908084)


MemCache (Ruby)
                          user     system      total        real
inserting data        1.200000   0.210000   1.410000 (  2.329328)
reading data          1.510000   0.220000   1.730000 (  2.721509)


Voldemort (Ruby)
                          user     system      total        real
inserting data       18.290000   1.050000  19.340000 ( 37.562370)
reading data          9.040000   0.530000   9.570000 ( 15.145843)
=end

require 'benchmark'
require 'rubygems'
require 'faker'

data = []

10_000.times do |i|
  data << Faker::Name.name
end

require 'rufus/tokyo/tyrant'

r = Rufus::Tokyo::Tyrant.new('127.0.0.1', 45000)
r.clear

2.times { puts }
puts 'Tokyo::Tyrant (Ruby FFI)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data.each_with_index { |e, i| r[i.to_s] = e }
  end

  b.report('reading data') do
    data.each_with_index { |e, i| nothing = r[i.to_s] }
  end
end

require 'tokyotyrant'

rdb = TokyoTyrant::RDB::new
rdb.open("127.0.0.1", 45000)
rdb.clear

2.times { puts }
puts 'TokyoTyrant::RDB (Ruby)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data.each_with_index { |e, i| rdb.put(i.to_s, e) }
  end

  b.report('reading data') do
    data.each_with_index { |e, i| nothing = rdb.get(i.to_s) }
  end
end

require 'tokyo_tyrant'
t = TokyoTyrant::DB.new('127.0.0.1', 45000)
t.clear

2.times { puts }
puts 'TokyoTyrant (C)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data.each_with_index { |e, i| t[i] = e }
  end

  b.report('reading data') do
    data.each_with_index { |e, i| nothing = t[i] }
  end
end

require 'memcached'
m = Memcached.new('127.0.0.1:45000')
m.flush

2.times { puts }
puts 'Memcached (C) to Tyrant'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data.each_with_index { |e, i| m.set(i.to_s, e) }
  end

  b.report('reading data') do
    data.each_with_index { |e, i| nothing = m.get(i.to_s) }
  end
end

require 'memcached'
m = Memcached.new('127.0.0.1:11211')
m.flush

2.times { puts }
puts 'Memcached (C) to Memcached'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data.each_with_index { |e, i| m.set(i.to_s, e) }
  end

  b.report('reading data') do
    data.each_with_index { |e, i| nothing = m.get(i.to_s) }
  end
end

require 'memcache'
mc = MemCache.new('127.0.0.1:11211')
mc.flush_all

2.times { puts }
puts 'MemCache (Ruby)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data.each_with_index { |e, i| mc.set(i.to_s, e) }
  end

  b.report('reading data') do
    data.each_with_index { |e, i| nothing = mc.get(i.to_s) }
  end
end

require 'voldemort-rb'
vdb = VoldemortClient.new("test", "localhost:6666")
2.times { puts }
puts 'Voldemort (Ruby)'

Benchmark.benchmark(' ' * 20 + Benchmark::Tms::CAPTION, 20) do |b|
  b.report('inserting data') do
    data.each_with_index { |e, i| vdb.put(i.to_s, e) }
  end

  b.report('reading data') do
    data.each_with_index { |e, i| nothing = vdb.get(i.to_s) }
  end
end

