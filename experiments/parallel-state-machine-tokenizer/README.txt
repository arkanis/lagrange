# Idea

The tokenizer should evaluate multiple state machines simultaneously. One state
machine for each token that can still match the input. The user supplies new
characters, one at a time, and for each the tokenizer advances the state
machines.

If at the end one active state machine remains and matches all is fine. If there
are other active state machine when one matches there's a conflict.

This approach allows runtime inspection into what the input can possibly mean in
the language. Might be useful for an text editor. Part of the idea was the hope
to reuse the same approach and code for the parser later on. With the same
benefits for a possible IDE. But I have no idea how realistic this is.

# Status

The project was abandoned after a few hours. To much overkill.

# How to run

gcc -std=c99 tokenizer.c -o tokenizer
./tokenizer sample.txt