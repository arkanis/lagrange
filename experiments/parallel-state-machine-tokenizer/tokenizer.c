#include <stdio.h>
#include <ctype.h>

char* keywords[] = {
	"function",
	"foo",
	"return"
};
const size_t token_count = sizeof(keywords) / sizeof(keywords[0]);

typedef struct {
	size_t token_idx;
	size_t state;
} active_token_t, *active_token_p;

active_token_t active_tokens[3];
size_t active_token_count = 0;

char* tok_advance(int c) {
	char* match = NULL;
	
	if (active_token_count == 0) {
		// we're at the start of a new token, check all possible tokens
		for(size_t i = 0; i < token_count; i++) {
			if (keywords[i][0] == c) {
				active_tokens[active_token_count].token_idx = i;
				active_tokens[active_token_count].state = 1;
				active_token_count++;
			}
		}
	} else {
		// we're currently parsing some tokens, just check those in the given state
		for(size_t i = 0; i < active_token_count; i++) {
			char* token = keywords[active_tokens[i].token_idx];
			size_t state = active_tokens[i].state;
			if (token[state] == c) {
				// Match, go to next state
				active_tokens[i].state++;
				
				if (token[active_tokens[i].state] == '\0') {
					// Current token finished!
					if (active_token_count == 1) {
						// We got an unabigous match of this token
						match = token;
						active_token_count = 0;
					} else {
						// Other tokens might also match if more input comes along
						printf(" ambigous match!");
					}
				}
			} else {
				// Fail, eliminate token from active token list
				// We do this by swapping the last active token state into this slot,
				// making the list 1 shorter and reexecuting the current index (now
				// with the formerly last token state in it).
				// 
				// TODO: When we need to keep the order in the active tokens list
				// (e.g. because it's priorized) we can use double-buffering with two
				// arrays: We only read all active tokens from the current one, only copying
				// matching tokens over to the second list. After that the current list is
				// cleared (length set to 0) and we swap the lists.
				active_tokens[i] = active_tokens[active_token_count-1];
				active_token_count--;
				i--;
			}
		}
	}
	
	if (active_token_count == 0) {
		printf(" no matching token found!");
	}
	
	return match;
}


int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s input-file\n", argv[0]);
		return 1;
	}
	
	FILE* f = fopen(argv[1], "rb");
	if (f == NULL) {
		perror("fopen()");
		return 1;
	}
	
	int c;
	do {
		c = getc(f);
		printf("%3d %c", c, isgraph(c) ? c : ' ');
		char* match = tok_advance(c);
		if (match)
			printf(" match: %s", match);
		printf("\n");
	} while (c != EOF);
	
	fclose(f);
	
	return 0;
}



/*
typedef struct {
	int start, end;
	size_t next_state;
} range_t, *range_p;

typedef struct {
	size_t range_count;
	range_p ranges;
} state_t, *state_p;

typedef struct {
	char* name;
	char* keyword;
	size_t state_count;
	state_p states;
} token_t, *token_p;

typedef struct {
	size_t token_count;
	token_p tokens;
} tokenizer_t, *tokenizer_p;


void tokenizer_new(tokenizer_p tok) {
}

void tokenizer_destroy(tokenizer_p tok) {
}


int main() {
	tokenizer_t tok;
	
	tokenizer_new(&tok, (token_t[]){
		(token_t){ "func", "function", NULL }
		(token_t){ "ws", NULL, (state_t[]){
			(state_t){
				(range_t){ '\t', '\t', 0 },
				(range_t){ '\n', '\n', 0 },
				(range_t){ ' ', ' ', 0 }
			}
		} }
		(token_t){ NULL, NULL, NULL }
	});
	tokenizer_destroy(&tok);
	
	return 0;
}
*/