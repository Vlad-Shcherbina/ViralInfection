set -e -x

    #-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC \
clang++ \
    --std=c++0x -W -Wall -Wno-sign-compare \
    -O2 -pipe -mmmx -msse -msse2 -msse3 \
    -ggdb \
    -fsanitize=address,integer,undefined \
    -fno-sanitize-recover \
    main.cc -o main

java -jar tester/ViralInfectionVis.jar \
    -exec "./driver.sh" -seed 23 -scale 10
