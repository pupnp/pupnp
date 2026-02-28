#! /bin/bash

#if false; then
	echo "debug, no openssl dynamic, no sanitizer:"
	./bootstrap && \
	./configure --enable-debug  --enable-shared --prefix=/home/mroberto/usr/libupnp CFLAGS="-Wconversion -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute" && \
	make clean > /dev/null && \
	make -j8 > /dev/null && \
	make install > /dev/null
#fi

if false; then
	echo "debug, no openssl static:"
	./bootstrap && \
	./configure --enable-debug  --disable-shared --prefix=/home/mroberto/usr/libupnp CFLAGS="-Wconversion -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute -fsanitize=address,leak" LDFLAGS="-fsanitize=address,leak" && \
	make clean > /dev/null && \
	make -j8 > /dev/null && \
	make install > /dev/null
fi

if false; then
	echo "debug, no openssl static, no leak sanitizer, for gdb debugging:"
	./bootstrap && \
	./configure --enable-debug  --disable-shared --prefix=/home/mroberto/usr/libupnp CFLAGS="-Wconversion -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute" && \
	make clean > /dev/null && \
	make -j8 > /dev/null && \
	make install > /dev/null
fi

if false; then
	echo "debug, with shared lib:"
	./bootstrap && \
	./configure --enable-debug  --enable-open_ssl --prefix=/home/mroberto/usr/libupnp CFLAGS="-Wconversion -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute -fsanitize=address,leak" LDFLAGS="-fsanitize=address,leak" && \
	make clean > /dev/null && \
	make -j8 > /dev/null && \
	make install > /dev/null
fi

if false; then
	echo "#no debug:"
	./bootstrap && \
	./configure --prefix=/home/mroberto/usr/libupnp CFLAGS="-Wconversion -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute" && \
	make clean > /dev/null && \
	make -j8 > /dev/null && \
	make install > /dev/null
fi

if false; then
	echo "cmake build"
	rm -rf build
	cmake -DBUILD_TESTING=ON -DDOWNLOAD_AND_BUILD_DEPS=ON -DCMAKE_BUILD_TYPE=Debug -S . -B build
	cmake --build build
	cd build || exit 1
	ctest --output-on-failure
fi

exit
