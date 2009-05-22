# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{ruby-tokyotyrant}
  s.version = "0.1.6"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Flinn"]
  s.date = %q{2009-05-21}
  s.email = %q{flinn@actsasflinn.com}
  s.extensions = ["ext/extconf.rb"]
  s.extra_rdoc_files = ["README.rdoc"]
  s.files = ["COPYING", "Rakefile", "README.rdoc", "ext/extconf.rb", "ext/tokyo_tyrant.c", "ext/tokyo_tyrant.h", "ext/tokyo_tyrant_db.c", "ext/tokyo_tyrant_db.h", "ext/tokyo_tyrant_module.c", "ext/tokyo_tyrant_module.h", "ext/tokyo_tyrant_query.c", "ext/tokyo_tyrant_query.h", "ext/tokyo_tyrant_table.c", "ext/tokyo_tyrant_table.h", "spec/plu_db.rb", "spec/spec.rb", "spec/spec_base.rb", "spec/start_tyrants.sh", "spec/stop_tyrants.sh", "spec/tokyo_tyrant_query_spec.rb", "spec/tokyo_tyrant_spec.rb", "spec/tokyo_tyrant_table_spec.rb", "benchmarks/bulk_db.rb", "benchmarks/bulk_table.rb", "benchmarks/db.rb", "benchmarks/table.rb"]
  s.has_rdoc = true
  s.homepage = %q{http://github.com/actsasflinn/ruby-tokyotyrant/}
  s.require_paths = ["ext"]
  s.rubygems_version = %q{1.3.1}
  s.summary = %q{A C based TokyoTyrant Ruby binding}
  s.test_files = ["spec/spec.rb"]

  if s.respond_to? :specification_version then
    current_version = Gem::Specification::CURRENT_SPECIFICATION_VERSION
    s.specification_version = 2

    if Gem::Version.new(Gem::RubyGemsVersion) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
