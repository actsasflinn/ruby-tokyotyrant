#!/bin/bash

#
# starting the tt servers (standard + table)
#

TMP=`pwd`/tmp
SPEC=`pwd`/spec
  # so that tt doesn't complain about relative paths...

[ -d $TMP ] || mkdir $TMP

ttserver \
  -dmn \
  -port 45000 \
  -pid $TMP/t_spec.pid -rts $TMP/t_spec.rts \
  -log $TMP/t.log \
  -ext $SPEC/ext.lua \
  $TMP/tyrant.tch

ttserver \
  -dmn \
  -port 45001 \
  -pid $TMP/tt1_spec.pid -rts $TMP/tt1_spec.rts \
  -log $TMP/tt1.log \
  -ext $SPEC/ext.lua \
  $TMP/tyrant_table1.tct

ttserver \
  -dmn \
  -port 45002 \
  -pid $TMP/tt2_spec.pid -rts $TMP/tt2_spec.rts \
  -log $TMP/tt2.log \
  -ext $SPEC/ext.lua \
  $TMP/tyrant_table2.tct

