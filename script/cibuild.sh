mkdir libgit2/build
cd libgit2/build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_CLAR=OFF 
-DCMAKE_C_FLAGS=-fPIC ..
cmake --build .
cd ../../

phpize
./configure
make
make test
