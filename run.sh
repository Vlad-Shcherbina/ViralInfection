set -e -x


clang++ \
    --std=c++0x -W -Wall -Wno-sign-compare \
    -O2 -pipe -mmmx -msse -msse2 -msse3 \
    -ggdb \
    -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC \
    -fsanitize=address,integer,undefined \
    -fno-sanitize-recover \
    main.cc -o main

time java -jar tester/ViralInfectionVis.jar \
    -exec "./driver.sh" -seed 9112 -scale 10 -novis
