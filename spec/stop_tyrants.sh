#!/bin/bash

# stopping the spec ttservers

ruby -e "%w{ tmp/t_spec.pid tmp/tt1_spec.pid tmp/tt2_spec.pid }.each { |pf| File.exist?(pf) && Process.kill(9, File.read(pf).strip.to_i) }"

rm tmp/t_spec.pid
rm tmp/tt1_spec.pid
rm tmp/tt2_spec.pid
