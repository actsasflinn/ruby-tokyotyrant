# ENV["RC_ARCHS"] = `uname -m`.chomp if `uname -sr` =~ /^Darwin/
#
# require 'mkmf'
#

if RUBY_PLATFORM =~ /darwin/
  ENV["RC_ARCHS"] = `uname -m`.chomp if `uname -sr` =~ /^Darwin/

  # On PowerPC the defaults are fine
  ENV["RC_ARCHS"] = '' if `uname -m` =~ /^Power Macintosh/
end

# Loads mkmf which is used to make makefiles for Ruby extensions
require 'mkmf'

# Give it a name
extension_name = 'tokyo_tyrant'

dir_config("tokyo_tyrant")

# NOTE: use GCC flags unless Visual C compiler is used
$CFLAGS << ' -Wall ' unless RUBY_PLATFORM =~ /mswin/

if RUBY_VERSION < '1.8.6'
  $CFLAGS << ' -DRUBY_LESS_THAN_186'
end

# Do the work

find_library(*['tokyotyrant', "tcrdbnew", dir_config('libtokyotyrant').last].compact) or
  raise "shared library 'libtokyotyrant' not found"

['tctdb.h',
 'tcrdb.h'].each do |header|
  find_header(*[header, dir_config('libtokyotyrant').first].compact) or
    raise "header file '#{header}' not  found"
end

create_makefile 'tokyo_tyrant'
