Gem::Specification.new do |s|
  s.name = 'ruby-tokyotyrant'
  s.version = '0.1.3'
  s.authors = [ 'Flinn' ]
  s.email = 'flinn@actsasflinn.com'
  s.homepage = 'http://github.com/actsasflinn/ruby-tokyotyrant/'
  s.platform = Gem::Platform::RUBY
  s.summary = 'A C based TokyoTyrant Ruby binding'

  s.require_path = 'ext'
  s.test_file = 'spec/spec.rb'
  s.has_rdoc = true
  s.extra_rdoc_files = %w{ README.rdoc }

  # What a fucking pain in the ass github
  s.files = ['COPYING',
             'Rakefile',
             'README.rdoc',
             'ext/extconf.rb',
             'ext/tokyo_tyrant.c',
             'ext/tokyo_tyrant.h',
             'ext/tokyo_tyrant_db.c',
             'ext/tokyo_tyrant_db.h',
             'ext/tokyo_tyrant_module.c',
             'ext/tokyo_tyrant_module.h',
             'ext/tokyo_tyrant_query.c',
             'ext/tokyo_tyrant_query.h',
             'ext/tokyo_tyrant_table.c',
             'ext/tokyo_tyrant_table.h',
             'spec/plu_db.rb',
             'spec/spec.rb',
             'spec/spec_base.rb',
             'spec/start_tyrant.sh',
             'spec/stop_tyrant.sh',
             'spec/tokyo_tyrant_query_spec.rb',
             'spec/tokyo_tyrant_spec.rb',
             'spec/tokyo_tyrant_table_spec.rb',
             'benchmarks/bulk_db.rb',
             'benchmarks/bulk_table.rb',
             'benchmarks/db.rb',
             'benchmarks/table.rb']
  s.extensions << "ext/extconf.rb"
end
