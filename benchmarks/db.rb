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
