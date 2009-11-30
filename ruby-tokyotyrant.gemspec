# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{ruby-tokyotyrant}
  s.version = "0.4"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Flinn"]
  s.date = %q{2009-11-29}
  s.email = %q{flinn@actsasflinn.com}
  s.extensions = ["ext/extconf.rb"]
  s.extra_rdoc_files = ["README.rdoc"]
  s.files = ["COPYING", "Rakefile", "README.rdoc", "ext/tokyo_tyrant.c", "ext/tokyo_tyrant.h", "ext/tokyo_tyrant_db.c", "ext/tokyo_tyrant_db.h", "ext/tokyo_tyrant_module.c", "ext/tokyo_tyrant_module.h", "ext/tokyo_tyrant_query.c", "ext/tokyo_tyrant_query.h", "ext/tokyo_tyrant_table.c", "ext/tokyo_tyrant_table.h", "ext/tokyo_utils.c", "ext/tokyo_utils.h", "lib/tokyo_tyrant/balancer.rb", "spec/ext.lua", "spec/plu_db.rb", "spec/spec.rb", "spec/spec_base.rb", "spec/start_tyrants.sh", "spec/stop_tyrants.sh", "spec/tokyo_tyrant_balancer_db_spec.rb", "spec/tokyo_tyrant_balancer_table_spec.rb", "spec/tokyo_tyrant_query_spec.rb", "spec/tokyo_tyrant_spec.rb", "spec/tokyo_tyrant_table_spec.rb", "benchmarks/balancer.rb", "benchmarks/bulk_db.rb", "benchmarks/bulk_table.rb", "benchmarks/db.rb", "benchmarks/table.rb", "ext/extconf.rb"]
  s.homepage = %q{http://github.com/actsasflinn/ruby-tokyotyrant/}
  s.require_paths = ["ext"]
  s.rubygems_version = %q{1.3.5}
  s.summary = %q{A C based TokyoTyrant Ruby binding}
  s.test_files = ["spec/spec.rb"]

  if s.respond_to? :specification_version then
    current_version = Gem::Specification::CURRENT_SPECIFICATION_VERSION
    s.specification_version = 3

    if Gem::Version.new(Gem::RubyGemsVersion) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<fast_hash_ring>, [">= 0.1.2"])
      s.add_runtime_dependency(%q<fast_hash_ring>, [">= 0.1.1"])
    else
      s.add_dependency(%q<fast_hash_ring>, [">= 0.1.2"])
      s.add_dependency(%q<fast_hash_ring>, [">= 0.1.1"])
    end
  else
    s.add_dependency(%q<fast_hash_ring>, [">= 0.1.2"])
    s.add_dependency(%q<fast_hash_ring>, [">= 0.1.1"])
  end
end
