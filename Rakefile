#!/usr/bin/env ruby

require 'pathname'
$root = Pathname(__FILE__).dirname

require 'rubygems'
require 'rake'
require 'rake/clean'
require 'rake/packagetask'
require 'rake/gempackagetask'
require 'rake/testtask'
require 'rake/rdoctask'

task :spec do
  load $root.join('spec', 'spec_base.rb')
end
task :default => [ :spec ]

CLEAN.include('pkg', 'tmp')

gemspec = File.read('ruby-tokyotyrant.gemspec')
eval "gemspec = #{gemspec}"

Rake::GemPackageTask.new(gemspec) do |pkg|
  pkg.need_tar = true
end

Rake::PackageTask.new('ruby-tokyotyrant', '0.1') do |pkg|
  pkg.need_zip = true
  pkg.package_files = FileList[
    'Rakefile',
    '*.txt',
    'lib/**/*',
    'spec/**/*',
    'test/**/*'
  ].to_a
  class << pkg
    def package_name
      "#{@name}-#{@version}-src"
    end
  end
end
