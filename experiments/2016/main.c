// For strdup
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define SLIM_HASH_IMPLEMENTATION
#include "slim_hash.h"

#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "reg_alloc.h"


void   fill_namespaces(node_p node, node_ns_p current_ns);
node_p expand_uops(node_p node, uint32_t level, uint32_t flags, void* private);
void   compile(node_p node, const char* filename);


int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: %s source-file\n", argv[0]);
		return 1;
	}
	
	str_t src = str_fload(argv[1]);
	token_list_p list = lex_str(src, argv[1], stderr);
	
	if (list->error_count > 0) {
		// Just output errors and exit
		for(size_t i = 0; i < list->tokens.len; i++) {
			token_p t = &list->tokens.ptr[i];
			if (t->type == T_ERROR) {
				fprintf(stderr, "%s:%d:%d: %s\n", argv[1], token_line(t), token_col(t), t->str_val.ptr);
				token_print_line(stderr, t, 0);
			}
		}
		
		lex_free(list);
		str_free(&src);
		return 1;
	}
	
	for(size_t i = 0; i < list->tokens.len; i++) {
		token_p t = &list->tokens.ptr[i];
		if (t->type == T_COMMENT || t->type == T_WS) {
			token_print(stdout, t, TP_SOURCE);
		} else if (t->type == T_WSNL || t->type == T_EOF) {
			printf(" ");
			token_print(stdout, t, TP_DUMP);
			printf(" ");
		} else {
			token_print(stdout, t, TP_DUMP);
		}
	}
	printf("\n");
	
	printf("\n");
	node_p tree = parse(list, parse_module);
	printf("\n");
	node_print(tree, stdout);
	
	
	node_ns_t global_ns;
	node_ns_new(&global_ns);
	// TODO: add builtins like "syscall" to global namespace
	fill_namespaces(tree, &global_ns);
	
	node_iterate(tree, expand_uops, NULL);
	node_print(tree, stdout);
	
	compile(tree, "main.elf");
	
	node_ns_destroy(&global_ns);
	lex_free(list);
	str_free(&src);
	
	return 0;
}


//
// Fill namespaces with links to their defining nodes
//

void fill_namespaces(node_p node, node_ns_p current_ns) {
	switch(node->type) {
		case NT_MODULE:
			for(size_t i = 0; i < node->module.defs.len; i++)
				fill_namespaces(node->module.defs.ptr[i], &node->module.ns);
			break;
		case NT_FUNC:
			node_ns_put(current_ns, node->func.name, node);
			for(size_t i = 0; i < node->func.in.len; i++)
				fill_namespaces(node->func.in.ptr[i], &node->func.ns);
			for(size_t i = 0; i < node->func.out.len; i++)
				fill_namespaces(node->func.out.ptr[i], &node->func.ns);
			for(size_t i = 0; i < node->func.body.len; i++)
				fill_namespaces(node->func.body.ptr[i], &node->func.ns);
			break;
		case NT_SCOPE:
			for(size_t i = 0; i < node->scope.stmts.len; i++)
				fill_namespaces(node->scope.stmts.ptr[i], &node->scope.ns);
			break;
		case NT_IF:
			// TODO: add variable defined in condition to true_ns so it can be
			// used only within the true_case (inspired by D).
			fill_namespaces(node->if_stmt.true_case, &node->if_stmt.true_ns);
			if (node->if_stmt.false_case)
				fill_namespaces(node->if_stmt.false_case, current_ns);
			break;
		case NT_WHILE:
			fill_namespaces(node->while_stmt.body, current_ns);
			break;
		
		case NT_ARG:
			// Don't add unnamed args to the namespace
			if (node->arg.name.len > 0)
				node_ns_put(current_ns, node->arg.name, node);
			break;
		case NT_VAR:
			node_ns_put(current_ns, node->var.name, node);
			break;
		
		default:
			for(member_spec_p member = node->spec->members; member->type != 0; member++) {
				if (member->type == MT_NS) {
					fprintf(stderr, "fill_namespaces(): found unhandled node with a namespace!\n");
					abort();
				}
			}
	}
}


//
// Transformation passes
// 

//
// Transform unordered operations (uops) into binary operator trees (op nodes).
//

// Based on http://en.cppreference.com/w/c/language/operator_precedence
// Worth a look because of bitwise and comparison ops: http://wiki.dlang.org/Operator_precedence
typedef enum { LEFT_TO_RIGHT, RIGHT_TO_LEFT } op_assoc_t;
typedef enum {
	OP_MUL, OP_DIV, OP_REM,
	OP_ADD, OP_SUB,
	OP_LT, OP_LE, OP_GT, OP_GE,
	OP_EQ, OP_NEQ,
	OP_ASSIGN
} op_idx_t;
// CAUTION: Operators with the same precedence need the same associativity!
// Otherwise it probably gets complicates... not sure.
struct { char* name; int precedence; op_assoc_t assoc; } operators[] = {
	[OP_MUL]    = { "*",  80, LEFT_TO_RIGHT },
	[OP_DIV]    = { "/",  80, LEFT_TO_RIGHT },
	[OP_REM]    = { "%",  80, LEFT_TO_RIGHT },
	
	[OP_ADD]    = { "+",  70, LEFT_TO_RIGHT },
	[OP_SUB]    = { "-",  70, LEFT_TO_RIGHT },
	
	[OP_LT]     = { "<",  50, LEFT_TO_RIGHT },
	[OP_LE]     = { "<=", 50, LEFT_TO_RIGHT },
	[OP_GT]     = { ">",  50, LEFT_TO_RIGHT },
	[OP_GE]     = { ">=", 50, LEFT_TO_RIGHT },
	
	[OP_EQ]     = { "==", 40, LEFT_TO_RIGHT },
	[OP_NEQ]    = { "!=", 40, LEFT_TO_RIGHT },
	
	[OP_ASSIGN] = { "=",   0, RIGHT_TO_LEFT },
};

/**
 * For each uops node: We find the strongest binding operator and replace it and
 * the nodes it binds to with an op node. This is repeated until no operators
 * are left to replace. The uops node should just have one op child at the end.
 */
node_p expand_uops(node_p node, uint32_t level, uint32_t flags, void* private) {
	// We only want uops nodes with children in them (there should be no uops
	// nodes without children).
	if ( !( (flags & NI_PRE) && node->type == NT_UOPS ) )
		return node;
	
	node_list_p list = &node->uops.list;
	
	while (list->len > 1) {
		// Find strongest operator (operator with highest precedence)
		int strongest_op_idx = -1;
		int strongest_op_prec = 0;
		int strongest_op_node_idx = -1;
		for(int node_idx = 1; node_idx < (int)list->len; node_idx += 2) {
			node_p op_slot = list->ptr[node_idx];
			if (op_slot->type != NT_ID) {
				fprintf(stderr, "expand_uops(): got non NT_ID in uops op slot!\n");
				abort();
			}
			
			// Find operator of the current op_slot node based on the IDs name
			ssize_t op_idx = -1;
			for(size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++) {
				if ( strncmp(operators[i].name, op_slot->id.name.ptr, op_slot->id.name.len) == 0 && (int)strlen(operators[i].name) == op_slot->id.name.len ) {
					op_idx = i;
					break;
				}
			}
			
			if (op_idx == -1) {
				fprintf(stderr, "expand_uops(): unknown operator: %.*s!",
					op_slot->id.name.len, op_slot->id.name.ptr);
				abort();
			}
			
			printf("expand_uops(): got op %.*s (idx %zd) at node idx %d\n",
				op_slot->id.name.len, op_slot->id.name.ptr, op_idx, node_idx
			);
			
			// When several ops have the highest (same) precedence:
			// For LEFT_TO_RIGHT operators we pick the first op
			// For RIGHT_TO_LEFT operators we pick the last op
			if (
				(operators[op_idx].assoc == LEFT_TO_RIGHT && operators[op_idx].precedence >  strongest_op_prec) ||
				(operators[op_idx].assoc == RIGHT_TO_LEFT && operators[op_idx].precedence >= strongest_op_prec)
			) {
				strongest_op_prec = operators[op_idx].precedence;
				strongest_op_idx = op_idx;
				strongest_op_node_idx = node_idx;
			}
		}
		
		if (strongest_op_idx == -1) {
			fprintf(stderr, "expand_uops(): found no op in uops node!");
			abort();
		}
		
		printf("expand_uops(): op %s got highest precedence: %d\n",
			operators[strongest_op_idx].name, strongest_op_prec
		);
		
		
		// Take the nodes left and right from the op_slot node and create a new op
		// node out of them. Then replace these nodes in the list with the new op
		// node.
		node_p op_node = node_alloc(NT_OP);
		op_node->parent = node;
		op_node->op.idx = strongest_op_idx;
		node_set(op_node, &op_node->op.a, list->ptr[strongest_op_node_idx - 1]);
		node_set(op_node, &op_node->op.b, list->ptr[strongest_op_node_idx + 1]);
		
		node_list_replace_n1(list, strongest_op_node_idx - 1, 3, op_node);
		
		node_print(node, stderr);
	}
	
	// By now the uops node only contains one op node child. Return that so the
	// iteration function replaces this uops node with the returned op node.
	return node->uops.list.ptr[0];
}


//
// Compiler helper functions
//

static int32_t get_var_frame_displ(node_p node) {
	int32_t stack_offset;
	switch(node->type) {
		case NT_ARG:
			stack_offset = node->arg.frame_displ;
			break;
		case NT_VAR:
			stack_offset = node->var.frame_displ;
			break;
		default:
			fprintf(stderr, "get_var_frame_displ(): ID references unknwon variable type!\n");
			abort();
	}
	
	return stack_offset;
}

//
// Compiler stuff (tree to asm)
// 
// Compiles and assembles a function node. Also fills the address slot table
// with references to other symbols outside the function.
// 
// The same process is repeated (more or less recursivly) for all functions
// referenced by this function.
//

typedef struct {
	asm_p as;
	ra_p ra;
	list_t(node_p) compile_queue;
} compiler_ctx_t, *compiler_ctx_p;

raa_t compile_node(node_p node, compiler_ctx_p ctx, int8_t requested_result_register);
raa_t compile_func(node_p node, compiler_ctx_p ctx);
raa_t compile_scope(node_p node, compiler_ctx_p ctx);
raa_t compile_var(node_p node, compiler_ctx_p ctx);
raa_t compile_if(node_p node, compiler_ctx_p ctx);
raa_t compile_while(node_p node, compiler_ctx_p ctx);
raa_t compile_call(node_p node, compiler_ctx_p ctx, int8_t req_reg);
raa_t compile_syscall(node_p node, compiler_ctx_p ctx, int8_t req_reg);
raa_t compile_op(node_p node, compiler_ctx_p ctx, int8_t req_reg);
raa_t compile_intl(node_p node, compiler_ctx_p ctx, int8_t req_reg);
raa_t compile_strl(node_p node, compiler_ctx_p ctx, int8_t req_reg);
raa_t compile_id(node_p node, compiler_ctx_p ctx, int8_t req_reg);

void resolve_addr_slots(node_p node, compiler_ctx_p ctx);


void compile(node_p module, const char* filename) {
	printf("starting compilation pass...\n");
	
	compiler_ctx_t ctx = (compiler_ctx_t){
		.as = &(asm_t){ 0 },
		.ra = &(ra_t){ 0 },
		.compile_queue = { 0, NULL }
	};
	as_new(ctx.as);
	ra_new(ctx.ra);
	list_new(&ctx.compile_queue);
	
	node_p main_func_node = ns_lookup(module, str_from_c("main"));
	if (main_func_node == NULL) {
		fprintf(stderr, "compile(): Failed to find main func!\n");
		abort();
	}
	list_append(&ctx.compile_queue, main_func_node);
	
	while (ctx.compile_queue.len > 0) {
		node_p node_to_compile = ctx.compile_queue.ptr[0];
		list_shift(&ctx.compile_queue, 1);
		
		/*
		fprintf(stderr, "COMPILE %.*s %s\n",
			node_to_compile->func.name.len, node_to_compile->func.name.ptr,
			(node_to_compile->func.compiled) ? "already compiled" : ""
		);
		*/
		// Skip compiled functions that got added multiple times (e.g. by
		// multiple calls to it).
		if (node_to_compile->type == NT_FUNC && node_to_compile->func.compiled == true)
			continue;
		
		raa_t a = compile_node(node_to_compile, &ctx, -1);
		ra_free_reg(ctx.ra, ctx.as, a);
	}
	
	// TODO: call linker pass before we throw the asm code away!
	// Then save the linked code into a binary.
	printf("compilation pass done...\n");
	node_print(module, stdout);
	resolve_addr_slots(main_func_node, &ctx);
	
	as_save_elf(ctx.as, filename);
	
	list_free(&ctx.compile_queue);
	ra_destroy(ctx.ra);
	as_destroy(ctx.as);
}

raa_t compile_node(node_p node, compiler_ctx_p ctx, int8_t requested_result_register) {
	switch(node->type) {
		case NT_FUNC:
			return compile_func(node, ctx);
		case NT_SCOPE:
			return compile_scope(node, ctx);
		case NT_VAR:
			return compile_var(node, ctx);
		case NT_IF:
			return compile_if(node, ctx);
		case NT_WHILE:
			return compile_while(node, ctx);
		case NT_CALL:
			return compile_call(node, ctx, requested_result_register);
		case NT_INTL:
			return compile_intl(node, ctx, requested_result_register);
		case NT_STRL:
			return compile_strl(node, ctx, requested_result_register);
		case NT_OP:
			return compile_op(node, ctx, requested_result_register);
		case NT_ID:
			return compile_id(node, ctx, requested_result_register);
		
		case NT_MODULE:
			fprintf(stderr, "compile_node(): module nodes are just contains for definitions and can't be compiled!\n");
			abort();
		case NT_UOPS:
			fprintf(stderr, "compile_node(): uops nodes have to be reordered to op nodes before compilation!\n");
			abort();
		case NT_ARG:
			fprintf(stderr, "compile_node(): arg nodes should be handled by their parent func node and not compiled separatly!\n");
			abort();
		
		case NT_UNARY_OP:
			fprintf(stderr, "compile_node(): TODO\n");
			abort();
	}
	
	return ra_empty();
}

raa_t compile_func(node_p node, compiler_ctx_p ctx) {
	// Mark this function as compiled and set this functions offset into the
	// assembler code to the place where we're going to generate the
	// instructions.
	// If we don't do this at the start calls to our own function (recursion)
	// would add this function to the compile queue over and over again.
	node->func.compiled = true;
	node->func.as_offset = as_target(ctx->as);
	
	// Prologue: preserver callee saved regs rbx, rsp, rbp, r12, r13, r14, r15
	as_push(ctx->as, RBX);
	as_push(ctx->as, R12);
	as_push(ctx->as, R13);
	as_push(ctx->as, R14);
	as_push(ctx->as, R15);
	// Prologue: preserve callers base pointer and use stack pointer as out base pointer
	as_push(ctx->as, RBP);
	as_mov(ctx->as, RBP, RSP);
	ssize_t frame_size_offset = as_sub(ctx->as, RSP, imm(0));
	
	// Wire up frame offsets of input and output arguments. The BP points to the
	// saved BP, so we don't have to skip that, thats what the -1 is for.
	// Remember, stack grows downwards, so the SP always contains the address of
	// the highest byte allocated on the stack.
	// We want to skip 7 64 bit values (1 saved BP, 5 callee saved regs and 1
	// return address). The -1 offset accounts for the stack growing towards
	// smaller addresses.
	// in(n)  = [RBP + (7-1 + in.argc - n)*8]
	// out(n) = [RBP + (7-1 + in.argc + out.argc - n)*8]
	for(size_t i = 0; i < node->func.in.len; i++)
		node->func.in.ptr[i]->arg.frame_displ = (7-1 + node->func.in.len - i) * 8;
	for(size_t i = 0; i < node->func.out.len; i++)
		node->func.out.ptr[i]->arg.frame_displ = (7-1 + node->func.in.len + node->func.out.len - i) * 8;
	
	// Compile function body
	raa_t last_stmt_result;
	for(size_t i = 0; i < node->func.body.len; i++) {
		raa_t allocation = compile_node(node->func.body.ptr[i], ctx, -1);
		// Free allocations in case the statement is an expr. If the statement
		// returns no allocation this does nothing. The result of the last
		// statement is not freed but stored so we can use it as return value.
		if (i != node->func.body.len - 1) {
			ra_free_reg(ctx->ra, ctx->as, allocation);
			// Make sure that after the statement no registers are spilled on
			// the stack.
			ra_ensure(ctx->ra, 0, 0);
		} else {
			last_stmt_result = allocation;
		}
	}
	
	if (last_stmt_result.reg_index != -1 && node->func.out.len >= 1) {
		// The last statement actually returned a result, write it into the first
		// output arg of the function (if the func has at least one out arg).
		as_mov(ctx->as, memrd(RBP, node->func.out.ptr[0]->arg.frame_displ), reg(last_stmt_result.reg_index));
	}
	ra_free_reg(ctx->ra, ctx->as, last_stmt_result);
	
	// Now we know the size of the functions stack frame so patch the stack
	// frame allocation in the prologue.
	uint32_t* frame_size_ptr = (uint32_t*)(ctx->as->code_ptr + frame_size_offset);
	*frame_size_ptr = node->func.stack_frame_size;
	
	// Epilogue: restore callers stack and base pointer
	as_mov(ctx->as, RSP, RBP);
	as_pop(ctx->as, RBP);
	// Epilogue: restore callee saved registeres
	as_pop(ctx->as, R15);
	as_pop(ctx->as, R14);
	as_pop(ctx->as, R13);
	as_pop(ctx->as, R12);
	as_pop(ctx->as, RBX);
	
	as_ret(ctx->as, 0);
	
	return ra_empty();
}

raa_t compile_scope(node_p node, compiler_ctx_p ctx) {
	// Nested scope is created by namespace
	
	// Compile all statements
	for(size_t i = 0; i < node->scope.stmts.len; i++) {
		raa_t allocation = compile_node(node->scope.stmts.ptr[i], ctx, -1);
		// Just free in case something comes back for whatever reason
		ra_free_reg(ctx->ra, ctx->as, allocation);
	}
	
	return ra_empty();
}

raa_t compile_call(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	// "syscall" is a builtin that's compiled differently
	// TODO: Refactor this into a proper "builtin" thing that can be added to
	// the namespace.
	if ( str_eqc(&node->call.name, "syscall") )
		return compile_syscall(node, ctx, req_reg);
	
	// Check that argument count is what the function expects
	node_p target = ns_lookup(node, node->call.name);
	if (node->call.args.len != target->func.in.len) {
		fprintf(stderr, "compile_call(): function %.*s expected %zu arguments but %zu given!\n",
			target->func.name.len, target->func.name.ptr, target->func.in.len, node->call.args.len);
		abort();
	}
	
	// Add target function to compile queue if it's not already compiled
	if (!target->func.compiled)
		list_append(&ctx->compile_queue, target);
	/*
	fprintf(stderr, "ADD CALL %.*s %s\n",
		target->func.name.len, target->func.name.ptr,
		(target->func.compiled) ? "already compiled" : ""
	);
	fprintf(stderr, "  queue:");
	for(size_t i = 0; i < ctx->compile_queue.len; i++)
		fprintf(stderr, " %.*s",
			ctx->compile_queue.ptr[i]->func.name.len,
			ctx->compile_queue.ptr[i]->func.name.ptr
		);
	fprintf(stderr, "\n");
	*/
	
	// Allocate stack space for currently allocated caller saved regs (that need to be saved)
	// Caller saved regs: rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11
	asm_arg_t caller_saved_regs[] = { RAX, RDI, RSI, RDX, RCX, R8, R9, R10, R11 };
	uint8_t csr_ptr[9];
	size_t  csr_len = 0;
	for(size_t i = 0; i < sizeof(caller_saved_regs) / sizeof(caller_saved_regs[0]); i++) {
		if ( ra_reg_allocated(ctx->ra, caller_saved_regs[i].reg) )
			csr_ptr[csr_len++] = caller_saved_regs[i].reg;
	}
	as_sub(ctx->as, RSP, imm(csr_len * 8));
	
	// Allocate stack space for output arguments
	if (target->func.out.len > 0)
		as_sub(ctx->as, RSP, imm(target->func.out.len * 8));
	
	// Push input arguments
	for(size_t i = 0; i < node->call.args.len; i++) {
		raa_t a = compile_node(node->call.args.ptr[i], ctx, -1);
		as_push(ctx->as, reg(a.reg_index));
		ra_free_reg(ctx->ra, ctx->as, a);
	}
	
	// Preserve caller saved regs into the space we allocated for them
	size_t args_on_stack = node->call.args.len + target->func.out.len;
	for(size_t i = 0; i < csr_len; i++)
		as_mov(ctx->as, memrd(RSP, (args_on_stack + i)*8), reg(csr_ptr[i]));
	
	// Call the target function and remember the displacement.
	// We create an address slot with it in the enclosing function so the linker
	// can patch it to the proper value later on.
	ssize_t target_displ_offset = as_call(ctx->as, reld(0));
	
	// Find enclosing function
	node_p encl_func = node->parent;
	while (encl_func != NULL && encl_func->type != NT_FUNC)
		encl_func = encl_func->parent;
	list_append(&encl_func->func.addr_slots, ( (node_addr_slot_t){
		.offset = target_displ_offset,
		.target = target
	} ));
	
	// Cleanup input and unused output args.
	// For now we only use the first output argument (if there is one) and free the rest.
	size_t args_to_free = target->func.in.len + (target->func.out.len > 1 ? target->func.out.len - 1 : 0);
	as_add(ctx->as, RSP, imm(args_to_free * 8));
	
	int8_t out_arg_reg = -1;
	// But if it has at least one out arg pop that value into the requested output arg
	if (target->func.out.len > 0) {
		// Pop the out arg into any free and not-caller-saved register
		out_arg_reg = ra_find_free_reg(ctx->ra);
		as_pop(ctx->as, reg(out_arg_reg));
	}
	
	// Restore caller saved regs
	for(size_t i = 0; i < csr_len; i++)
		as_pop(ctx->as, reg(csr_ptr[i]));
	
	// Return nothing if the called func has no output argument
	raa_t result_allocation = ra_empty();
	// But if it has we allocate the requested register and put the out arg in there
	if (out_arg_reg != -1) {
		result_allocation = ra_alloc_reg(ctx->ra, ctx->as, req_reg);
		as_mov(ctx->as, reg(result_allocation.reg_index), reg(out_arg_reg));
	}
	
	return result_allocation;
	
	/*
	// Put arguments into R0 - R7
	size_t i;
	raa_t alloced_reg[8];
	for(i = 0; i < node->call.args.len; i++)
		alloced_reg[i] = compile_node(node->call.args.ptr[i], ctx, i);
	// Allocate remaining argument registers so the caller saves the values in
	// any unused argument registers.
	for(; i < 8; i++)
		alloced_reg[i] = ra_alloc_reg(ctx->ra, ctx->as, i);
	
	// Insert the CALL with a displacement of 0 for now and add an address slot
	// for the call. So the linker step later knows what to insert here.
	// Remember: The IP of the next instruction is used for the displ. calculation!
	// So we take the offset after assembling the call instruction.
	as_call(ctx->as, reld(0));
	list_append(&node->func.addr_slots, ( (node_addr_slot_t){
		.offset = as_target(ctx->as),
		.target = target
	} ));
	
	// Add the target function to the compile queue if necessary
	// If the target function is already compiled (has an as_offset) we could
	// assemble the target displ. directly in the call instruction and wouldn't
	// need an addr slot. But then the linker step wouldn't be able to completly
	// replace a function with a new definition later on. So we don't optimize
	// that here so the linker pass really gets addr slots for _all_ references.
	if ( ! target->func.compiled )
		list_append(&ctx->compile_queue, target);
	
	// Output registers are in R0 - R7
	// For now only take R0 and throw away (free) R1 - R7
	for(size_t i = 1; i < 8; i++)
		ra_free_reg(ctx->ra, ctx->as, alloced_reg[i]);
	
	return alloced_reg[0];
	*/
}

raa_t compile_syscall(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	/*
	Taken from:
	- http://wiki.osdev.org/Calling_Conventions
	- http://man7.org/linux/man-pages/man2/syscall.2.html
	
	Input:
		RAX ← syscall_no
		RDI
		RSI
		RDX
		R10
		R8
		R9
	Scratch:
		All input regs
		R10
		R11
	Output:
		RAX
		RDX (no idea whats in there, not listed in man pages)
	*/
	
	int8_t arg_regs[7] = { RAX.reg, RDI.reg, RSI.reg, RDX.reg, R10.reg, R8.reg, R9.reg };
	raa_t arg_allocs[7];
	
	// Need at least one argument (the syscall number)
	if (node->call.args.len < 1) {
		fprintf(stderr, "compile_syscall(): need at least 1 arg, got %zu\n", node->call.args.len);
		abort();
	}
	
	// Compile args
	for(size_t i = 0; i < node->call.args.len; i++)
		arg_allocs[i] = compile_node(node->call.args.ptr[i], ctx, arg_regs[i]);
	
	// Allocate scratch registers
	// TODO: Also allocate unused args as scratch regs
	raa_t a1 = ra_alloc_reg(ctx->ra, ctx->as, R10.reg);
	raa_t a2 = ra_alloc_reg(ctx->ra, ctx->as, R11.reg);
	
	as_syscall(ctx->as);
	
	// Free scratch registers
	ra_free_reg(ctx->ra, ctx->as, a2);
	ra_free_reg(ctx->ra, ctx->as, a1);
	
	// Free argument registers (but leave RAX allocated since it's the result)
	for(size_t i = 1; i < node->call.args.len; i++)
		ra_free_reg(ctx->ra, ctx->as, arg_allocs[i]);
	
	// If the result is requested in RAX or it doesn't matter where we're done
	if (req_reg == RAX.reg || req_reg == -1)
		return arg_allocs[0];
	
	// Otherwise move it to the target register
	raa_t a = ra_alloc_reg(ctx->ra, ctx->as, req_reg);
	as_mov(ctx->as, reg(req_reg), RAX);
	ra_free_reg(ctx->ra, ctx->as, arg_allocs[0]);
	return a;
}

raa_t compile_var(node_p node, compiler_ctx_p ctx) {
	// Find containing function
	node_p func = node->parent;
	while (func != NULL && func->type != NT_FUNC)
		func = func->parent;
	
	// Get stack frame offset for this var by "allocating" it on the enclosing
	// functions stack frame. The frame displacement is negative since local
	// variables are allocated below the base pointer.
	node->var.frame_displ = -func->func.stack_frame_size;
	func->func.stack_frame_size += 8;
	
	// Compile value expr if there is one
	if (node->var.value != NULL) {
		raa_t a = compile_node(node->var.value, ctx, -1);
		as_mov(ctx->as, memrd(RBP, node->var.frame_displ), reg(a.reg_index));
		ra_free_reg(ctx->ra, ctx->as, a);
	}
	
	return ra_empty();
}

raa_t compile_if(node_p node, compiler_ctx_p ctx) {
	raa_t a = compile_node(node->if_stmt.cond, ctx, -1);
	as_cmp(ctx->as, reg(a.reg_index), imm(0));
	// freeing might add a MOV or POP here to restore the previous value of a
	// register but nither effects the Flags. So it's safe to use here.
	ra_free_reg(ctx->ra, ctx->as, a);
	// We jump to the false case if the condition is equal to 0 (false)
	asm_jump_slot_t to_false_case = as_jmp_cc(ctx->as, CC_EQUAL, 0);
	
	a = compile_node(node->if_stmt.true_case, ctx, -1);
	ra_free_reg(ctx->ra, ctx->as, a);
	asm_jump_slot_t to_end = as_jmp(ctx->as, reld(0));
	
	// If there no false case we jump here anyway since the next statement will
	// continue here.
	as_mark_jmp_slot_target(ctx->as, to_false_case);
	
	if (node->if_stmt.false_case) {
		a = compile_node(node->if_stmt.false_case, ctx, -1);
		ra_free_reg(ctx->ra, ctx->as, a);
		
		as_mark_jmp_slot_target(ctx->as, to_end);
	}
	
	
	return ra_empty();
}

raa_t compile_while(node_p node, compiler_ctx_p ctx) {
	raa_t a;
	
	// Compile condition
	size_t cond_target = as_target(ctx->as);
	a = compile_node(node->while_stmt.cond, ctx, -1);
	as_cmp(ctx->as, reg(a.reg_index), imm(0));
	// freeing might add a MOV here to restore the previous value of a register
	// but a MOV doesn't effect the Flags. So it's safe to use here.
	ra_free_reg(ctx->ra, ctx->as, a);
	// We jump to the end of the loop if the condition is equal to 0 (false)
	asm_jump_slot_t to_end = as_jmp_cc(ctx->as, CC_EQUAL, 0);
	
	// Compile body, then jump back to condition
	a = compile_node(node->while_stmt.body, ctx, -1);
	ra_free_reg(ctx->ra, ctx->as, a);
	asm_jump_slot_t to_cond = as_jmp(ctx->as, reld(0));
	as_set_jmp_slot_target(ctx->as, to_cond, cond_target);
	
	as_mark_jmp_slot_target(ctx->as, to_end);
	return ra_empty();
}


raa_t compile_op(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	size_t op = node->op.idx;
	node_p a = node->op.a, b = node->op.b;
	
	switch(op) {
		case OP_ADD: case OP_SUB:
		{
			raa_t a1 = compile_node(a, ctx, req_reg);
			raa_t a2 = compile_node(b, ctx, -1);
			
			switch(op) {
				case OP_ADD:  as_add(ctx->as, reg(a1.reg_index), reg(a2.reg_index));  break;
				case OP_SUB:  as_sub(ctx->as, reg(a1.reg_index), reg(a2.reg_index));  break;
			}
			
			ra_free_reg(ctx->ra, ctx->as, a2);
			return a1;
		}
		
		case OP_MUL: case OP_DIV: case OP_REM:
		{
			raa_t arg_dest = compile_node(a, ctx, RAX.reg);
			raa_t arg_src = compile_node(b, ctx, -1);
			// Reserve RDX since MUL and DIV overwrite it
			// For MUL it's the upper 64 bits of the result
			// For DIV it's the remainder of the division
			raa_t arg_up_rem = ra_alloc_reg(ctx->ra, ctx->as, RDX.reg);
			
			switch(op) {
				case OP_MUL:
					as_mul(ctx->as, reg(arg_src.reg_index));
					break;
				case OP_DIV:
				case OP_REM:
					// Set RDX to 0 since DIV takes it as input as the upper
					// 64 bits of the operand.
					as_mov(ctx->as, RDX, imm(0));
					as_div(ctx->as, reg(arg_src.reg_index));
					if (op == OP_DIV)
						break;
					
					// For REM we use RDX as destination operand since it
					// contains the remainer. Instead we free RAX. Basically
					// arg_dest and arg_up_rem switch roles here.
					{
						raa_t temp = arg_dest;
						arg_dest = arg_up_rem;
						arg_up_rem = temp;
					}
					break;
			}
			
			ra_free_reg(ctx->ra, ctx->as, arg_up_rem);
			ra_free_reg(ctx->ra, ctx->as, arg_src);
			
			if (req_reg == -1 || req_reg == arg_dest.reg_index) {
				// Caller either doesn't care in which register the result is
				// returned or he wants it in the one it's alread in. Either way
				// we're done.
				return arg_dest;
			} else {
				// Caller wants the result in a differen register. Allocate it
				// and move the result there.
				raa_t target_reg = ra_alloc_reg(ctx->ra, ctx->as, req_reg);
				as_mov(ctx->as, reg(req_reg), reg(arg_dest.reg_index));
				ra_free_reg(ctx->ra, ctx->as, arg_dest);
				return target_reg;
			}
		}
		
		case OP_LT: case OP_LE: case OP_GT: case OP_GE:
		case OP_EQ: case OP_NEQ:
		{
			uint8_t condition_code;
			switch(op) {
				case OP_LT:  condition_code = CC_LESS;             break;
				case OP_LE:  condition_code = CC_LESS_OR_EQUAL;    break;
				case OP_GT:  condition_code = CC_GREATER;          break;
				case OP_GE:  condition_code = CC_GREATER_OR_EQUAL; break;
				case OP_EQ:  condition_code = CC_EQUAL;            break;
				case OP_NEQ: condition_code = CC_NOT_EQUAL;        break;
				default:     abort();
			}
			
			raa_t a1 = compile_node(a, ctx, req_reg);
			raa_t a2 = compile_node(b, ctx, -1);
			as_cmp(ctx->as, reg(a1.reg_index), reg(a2.reg_index));
			ra_free_reg(ctx->ra, ctx->as, a2);
			
			as_set_cc(ctx->as, condition_code, reg(a1.reg_index));
			return a1;
		}
		
		case OP_ASSIGN:
		{
			if (a->type != NT_ID) {
				fprintf(stderr, "compile_op(): right side of assigment has to be an ID for now!\n");
				abort();
			}
			
			node_p target = ns_lookup(node, a->id.name);
			int32_t stack_offset = get_var_frame_displ(target);
			
			raa_t a = compile_node(b, ctx, -1);
			as_mov(ctx->as, memrd(RBP, stack_offset), reg(a.reg_index));
			return a;
			/*
			char* terminated_name = strndup(a->id.name.ptr, a->id.name.len);
			
			size_t stack_offset = scope_get(ctx->scope, terminated_name);
			raa_t alloc = compile_node(node->var.value, ctx, -1);
			as_mov(ctx->as, memrd(RSP, stack_offset), reg(alloc.reg_index));
			// keep value as result value of this operation
			//ra_free_reg(ctx->ra, ctx->as, alloc);
			
			free(terminated_name);
			return alloc;
			*/
		}
	}
	
	fprintf(stderr, "compile_op(): unknown op idx: %zu!\n", op);
	abort();
	return ra_empty();
}

raa_t compile_intl(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	raa_t a = ra_alloc_reg(ctx->ra, ctx->as, req_reg);
	as_mov(ctx->as, reg(a.reg_index), imm(node->intl.value));
	return a;
}

raa_t compile_strl(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	size_t str_vaddr = as_data(ctx->as, node->strl.value.ptr, node->strl.value.len);
	raa_t a = ra_alloc_reg(ctx->ra, ctx->as, req_reg);
	as_mov(ctx->as, reg(a.reg_index), imm(str_vaddr));
	return a;
}

raa_t compile_id(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	node_p target = ns_lookup(node, node->id.name);
	int32_t stack_offset = get_var_frame_displ(target);
	
	raa_t a = ra_alloc_reg(ctx->ra, ctx->as, req_reg);
	as_mov(ctx->as, reg(a.reg_index), memrd(RBP, stack_offset));
	
	return a;
}



//
// Linker pass
//

void resolve_addr_slots(node_p node, compiler_ctx_p ctx) {
	if (node->type != NT_FUNC) {
		fprintf(stderr, "resolve_addr_slots(): Can only link func nodes for now!\n");
		abort();
	}
	
	printf("linking %.*s\n", node->func.name.len, node->func.name.ptr);
	
	// Set the flag right at the start so we don't get an endless recursion on
	// self recursive function calls.
	node->func.linked = true;
	
	for(size_t i = 0; i < node->func.addr_slots.len; i++) {
		node_p target = node->func.addr_slots.ptr[i].target;
		size_t offset = node->func.addr_slots.ptr[i].offset;
		size_t rip_after_call = offset + 4;
		
		int32_t call_displ = target->func.as_offset - rip_after_call;
		
		int32_t* call_displ_ptr = (int32_t*)(ctx->as->code_ptr + offset);
		*call_displ_ptr = call_displ;
		
		if (!target->func.linked)
			resolve_addr_slots(target, ctx);
	}
}
