
.PHONY: ctags doxygen

all:
	scons

arm:
	scons arm

tdd:
	scons tdd

run: all
	build/tdd

clang: export CC=clang
clang: export CXX=clang++
clang: $(APP)
	scons

test: tdd
	build/tdd

valgrind: tdd
	valgrind build/tdd

clean:
	scons -c
	rm -rf build html latex

ctags:
	ctags -R --exclude=build --exclude=build_arm

doxygen:
	doxygen

# FIN
