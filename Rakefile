#!/usr/bin/env ruby

require 'pathname'
$root = Pathname(__FILE__).dirname

require 'rubygems'
require 'rake'

#$author  = 'cheapRoc'
#$email   = 'justin.reagor@gmail.com'
#$gem     = 'ruby-tokyotyrant'
#$version = Tyrant::VERSION

task :default => [ :spec ]

require $root.join('tasks', 'spec')
