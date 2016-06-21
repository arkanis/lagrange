#!/bin/bash
node runner.js $1
llc-2.9 out.ll
gcc out.s -o out
