GCC_OPTS = -std=gnu99

01_run_one_rule: 01_run_one_rule.c rule.o
	gcc $(GCC_OPTS) 01_run_one_rule.c rule.o -o 01_run_one_rule

02_run_rules: 02_run_rules.c rule.o
	gcc $(GCC_OPTS) 02_run_rules.c rule.o -o 02_run_rules

rule.o: rule.h rule.c
	gcc $(GCC_OPTS) -c rule.c