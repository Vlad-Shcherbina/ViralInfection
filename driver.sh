ulimit -c unlimited
ASAN_SYMBOLIZER_PATH=$(which llvm-symbolizer-3.4) MSAN_SYMBOLIZER_PATH=$(which llvm-symbolizer-3.4) ./main

# to extract coredump:
#   gdb main core -batch -ex bt
