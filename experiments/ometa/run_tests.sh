#!/bin/bash
# Parses sample.num with grammer.js and outputs the AST.
node ometa_test.js sample.num
# Sends llvm_sample.ll through llc and gcc via pipes. Outputs the finished binary.
node backend_test.js llvm_sample.ll