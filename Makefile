.PHONY: test

testHarness: testHarness.cpp ethers-cpp/*.h
	g++ -std=c++2a -O2 -g -Wall -I. -Iexternal/json/include -Iexternal/PEGTL/include -Iexternal/hoytech-cpp testHarness.cpp -lsecp256k1 -lgmp -lgmpxx -o testHarness

test: artifacts/TestContract.abi testHarness
	node tests.js

artifacts/TestContract.abi: TestContract.sol
	solc TestContract.sol --abi --overwrite -o artifacts/
