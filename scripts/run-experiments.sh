# Environment checks


echo "-- Checking the environment --"

RUST_LLVM=$(rustc --version --verbose | grep LLVM | cut -d ' ' -f 3)
CLANG_VER=$(clang -v 2>&1 | grep version | cut -d ' ' -f 4)

if [ "$RUST_LLVM" == "$CLANG_VER" ]; then
	echo "OK: rustc and clang are compatible"
else
	echo "ERROR: rustc and clang are not compatible (rust LLVM: $RUST_LLVM vs. clang LLVM: $CLANG_VER)"
fi


source setup-vars.sh

if [ "$vamos_buffers_BUILD_TYPE" != "Release" ]; then
	echo "The build type of VAMOS ($vamos_buffers_BUILD_TYPE) is not Release, this slows down the experiments very much."
else
	echo "OK: the build type of VAMOS"
fi

# -----------------
echo " -- Starting the experiments --"
sleep 1


pushd scalability
make experiments
popd

pushd primes
make experiments
make experiments-tessla
popd

pushd bank
make experiments
popd

pushd bank-tessla
make experiments
popd

pushd dataraces
make experiments
popd

