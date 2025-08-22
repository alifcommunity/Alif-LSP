#!/bin/bash

echo "Building and running Alif LSP tests..."
echo

# Compile the test executable
g++ -std=c++17 -I../src/third-party -I../src/include \
    test_completion.cpp \
    ../src/Completion.cpp \
    ../src/DocManager.cpp \
    -o test_completion

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi

echo "✅ Compilation successful!"
echo

# Run the tests
echo "Running tests..."
echo
./test_completion

# Store test result
TEST_RESULT=$?

# Cleanup
rm -f test_completion

echo
echo "Tests completed."
exit $TEST_RESULT
