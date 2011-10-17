#!/bin/bash
rm -f generated/*
antlr4ruby -o generated Small.g
antlr4ruby -o generated -lib generated SmallTreeWalker.g
