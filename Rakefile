#!/usr/bin/env ruby

GEMNAME = 'ruby-tokyotyrant'
GEMVERSION = '0.5.2'

require 'pathname'
$root = Pathname(__FILE__).dirname

require 'rubygems'
require 'rake'
require 'rake/clean'
require 'rake/packagetask'
require 'rubygems/package_task'
require 'rake/testtask'
require 'rdoc/task'

task :spec do
  load $root.join('spec', 'spec_base.rb')
end
task :default => [ :spec ]

CLEAN.include('pkg', 'tmp')

gemspec = Gem::Specification.new do |s|
  s.name = GEMNAME
  s.version = GEMVERSION
  s.authors = [ 'Flinn' ]
  s.email = 'flinn@actsasflinn.com'
  s.homepage = 'http://github.com/actsasflinn/ruby-tokyotyrant/'
  s.platform = Gem::Platform::RUBY
  s.summary = 'A C based TokyoTyrant Ruby binding'
  s.require_path = 'ext'
  s.test_file = 'spec/spec.rb'
  s.has_rdoc = true
  s.extra_rdoc_files = %w{ README.rdoc }

  s.files = ['COPYING',
             'Rakefile',
             'README.rdoc'] +
             Dir['ext/**/*.[rb|c|h]'] +
             Dir['ext/tokyo_tyrant/**/*.rb'] +
             Dir['spec/**/*'] +
             Dir['benchmarks/**/*']
  s.extensions << "ext/extconf.rb"
end

task :gemspec do
  File.open("#{GEMNAME}.gemspec", 'w') do |f|
    f.write(gemspec.to_ruby)
  end
end

Gem::PackageTask.new(gemspec) do |pkg|
  pkg.need_zip = true
  pkg.need_tar = true
end

Rake::PackageTask.new(GEMNAME, GEMVERSION) do |pkg|
  pkg.need_zip = true
  pkg.package_files = FileList[
    'COPYING',
    'Rakefile',
    'README.rdoc',
    'ext/**/*',
    'lib/**/*.[rb]',
    'spec/**/*',
    'benchmarks/**/*'
  ].to_a
  class << pkg
    def package_name
      "#{@name}-#{@version}-src"
    end
  end
end
