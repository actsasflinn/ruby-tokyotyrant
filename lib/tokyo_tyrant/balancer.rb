require 'tokyo_tyrant'
# require 'hash_ring'
require 'fast_hash_ring'

module TokyoTyrant
  module Balancer
    class Base
      def initialize(servers = [], weights = {})
        servers.collect! do |server|
          host, port = server.split(':')
          klass.new(host, port.to_i)
        end
        @servers = servers
        # @ring = HashRing.new(servers, weights)
        @ring = FastHashRing.new(servers, weights)
      end

      def ring
        @ring
      end

      def servers
        @servers
      end

      def db_for_key(key)
        ring.get_node(key)
      end

      # Delegate Methods
      def get(key)
        db = db_for_key(key)
        db.get(key)
      end
      alias :[] :get

      def add_int(key, i = 1)
        db = db_for_key(key)
        db.add_int(key, i)
      end
      alias :addint    :add_int
      alias :increment :add_int

      def get_int(key)
        db = db_for_key(key)
        db.get_int(key)
      end

      def add_double(key, i = 1.0)
        db = db_for_key(key)
        db.add_double(key, i)
      end
      alias :adddouble :add_double

      def get_double(key)
        db = db_for_key(key)
        db.get_double(key)
      end

      def put(key, value)
        db = db_for_key(key)
        db.put(key, value)
      end
      alias :[]= :put

      def putkeep(key, value)
        db = db_for_key(key)
        db.putkeep(key, value)
      end

      def putcat(key, value)
        db = db_for_key(key)
        db.putcat(key, value)
      end

      def putshl(key, value, width)
        db = db_for_key(key)
        db.putshl(key, value, width)
      end

      def putnr(key, value)
        db = db_for_key(key)
        db.putnr(key, value)
      end

      def vsiz(key)
        db = db_for_key(key)
        db.vsiz(key)
      end

      def fetch(key, &block)
        db = db_for_key(key)
        db.fetch(key, &block)
      end

      def out(key)
        db = db_for_key(key)
        db.out(key)
      end
      alias :delete :out

      # Aggregate Methods
      def close
        @servers.all?{ |server| server.close }
      end

      def rnum
        @servers.collect{ |server| server.rnum }.inject(nil){ |sum,x| sum ? sum+x : x }
      end
      alias :count  :rnum
      alias :size   :rnum
      alias :length :rnum

      def empty?
        @servers.all?{ |server| server.empty? }
      end

      def vanish
        @servers.all?{ |server| server.vanish }
      end
      alias :clear :vanish

      def sync
        @servers.each{ |server| server.sync }
      end

      def optimize(*args)
        @servers.all?{ |server| server.optimize(*args) }
      end

      def check(key)
        @servers.any?{ |server| server.check(key) }
      end
      alias :has_key? :check
      alias :key?     :check
      alias :include? :check
      alias :member?  :check

      def set_index(name, type)
        @servers.all?{ |server| server.set_index(name, type) }
      end

      def fwmkeys(prefix, max = -1)
        @servers.collect{ |server| server.fwmkeys(prefix, max) }.flatten
      end

      def delete_keys_with_prefix(prefix, max = -1)
        @servers.each{ |server| server.delete_keys_with_prefix(prefix, max) }
        nil
      end
      alias :dfwmkeys :delete_keys_with_prefix

      def keys
        @servers.collect{ |server| server.keys }.flatten.uniq
      end

      def values
        @servers.collect{ |server| server.values }.flatten.uniq
      end
    end
  end
end

module TokyoTyrant
  module Balancer
    class DB < Base
      def klass
        TokyoTyrant::DB
      end
    end
  end
end

module TokyoTyrant
  module Balancer
    class Table < Base
      def klass
        TokyoTyrant::Table
      end

      def find(&block)
        queries = @servers.collect{ |server|
          server.prepare_query(&block)
        }
        TokyoTyrant::Query.parallel_search(*queries)
      end
    end
  end
end
