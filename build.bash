#!/usr/bin/env bash

used_libs=(
	/usr/lib/libftxui-component.so.3.0.0
	/usr/lib/libftxui-screen.so.3.0.0
	/usr/lib/libftxui-dom.so.3.0.0
	/usr/lib/libfmt.so.9
	/usr/lib/libc.so
	/usr/lib/libboost_filesystem.so.1.82.0
	/usr/lib/libboost_atomic.so.1.82.0
	/usr/lib/gcc/x86_64-gentoo-linux-musl/13/libstdc++.so.6
	/usr/lib/gcc/x86_64-gentoo-linux-musl/13/libgcc_s.so.1
)

g++ main.cpp -o chn_bsub -O2 -lfmt -lftxui-component -lftxui-screen -lftxui-dom -lfmt -lboost_filesystem -std=c++23
rm -r lib
mkdir -p lib
for i in ${used_libs[@]}; do
	cp --dereference $i lib/
done
