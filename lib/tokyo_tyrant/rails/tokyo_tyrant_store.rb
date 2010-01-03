require 'tokyo_tyrant/balancer'

module ActiveSupport
  module Cache
    # A cache store implementation which stores data in Tokyo Tyrant:
    # http://1978th.net/tokyotyrant/
    #
    # Special features:
    # - Clustering and load balancing. One can specify multiple servers,
    #   and TokyoTyrantStore will load balance between all available servers.
    # - Per-request in memory cache for all communication with the Tokyo Tyrant server(s).
    class TokyoTyrantStore < Store
      def self.build_servers(*addresses)
        addresses = addresses.flatten
        options = addresses.extract_options!
        addresses = ["127.0.0.1:1978"] if addresses.empty?
        TokyoTyrant::Balancer::DB.new(addresses, options)
      end

      # Creates a new TokyoTyrantStore object, with the given server
      # addresses. Each address is either a host name, or a host-with-port string
      # in the form of "host_name:port". For example:
      #
      #   ActiveSupport::Cache::TokyoTyrantStore.new("localhost", "server-downstairs.localnetwork:8229")
      #
      # If no addresses are specified, then TokyoTyrantStore will connect to
      # localhost port 1978 (the default tokyo tyrant port).
      def initialize(*addresses)
        if addresses.first.respond_to?(:get)
          @data = addresses.first
        else
          @data = self.class.build_servers(*addresses)
        end

        extend Strategy::LocalCache
      end

      def read(key, options = nil) # :nodoc:
        super
        @data.get(key)
      rescue TokyoTyrantError => e
        logger.error("TokyoTyrantError (#{e}): #{e.message}")
        nil
      end

      # Writes a value to the cache.
      #
      # Possible options:
      # - +:unless_exist+ - set to true if you don't want to update the cache
      #   if the key is already set.
      #   the cache. See ActiveSupport::Cache::Store#write for an example.
      def write(key, value, options = nil)
        super
        method = options && options[:unless_exist] ? :putkeep : :put
        @data.send(method, key, value)
      rescue TokyoTyrantError => e
        logger.error("TokyoTyrantError (#{e}): #{e.message}")
        false
      end

      def delete(key, options = nil) # :nodoc:
        super
        @data.delete(key)
      rescue TokyoTyrantError => e
        logger.error("TokyoTyrantError (#{e}): #{e.message}")
        false
      end

      def exist?(key, options = nil) # :nodoc:
        @data.has_key?(key)
      end

      def increment(key, amount = 1) # :nodoc:
        log("incrementing", key, amount)
        @data.add_int(key, amount)
      rescue TokyoTyrantError
        nil
      end

      def decrement(key, amount = 1) # :nodoc:
        log("decrement", key, amount)
        @data.add_int(key, amount * -1)
      rescue TokyoTyrantError
        nil
      end

      def integer(key) # :nodoc:
        log("integer", key)
        @data.get_int(integer)
      rescue TokyoTyrantError
        nil
      end

      def delete_matched(matcher, options = nil) # :nodoc:
        super
        @data.delete_keys_with_prefix(matcher)
      rescue TokyoTyrantError
        nil
      end

      def clear
        @data.clear
      end
    end
  end
end
