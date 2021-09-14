#!/bin/sh

# Test all-intra coding.

set -eu

. "${0%/*}/util.sh"

common_args='256x128 10 yuv420p -p1 --preset=ultrafast --threads=0 --no-wpp --no-tmvp --no-deblock --sao=0 --pu-depth-intra 0-4'
valgrind_test $common_args --rd=1
valgrind_test $common_args --rd=2 --no-transform-skip --qp 37
valgrind_test $common_args --rd=2 --no-transform-skip --qp 37 --signhide --rdoq 
valgrind_test $common_args --alf=full --no-wpp --threads=0 --owf=0
valgrind_test $common_args --alf=full --wpp --threads=1
valgrind_test $common_args --jccr
valgrind_test $common_args --jccr --rdoq --rd=2 --mts=intra

