#include "common.h"

//
// Node specs
//

#define BEGIN(nn)               [ NT_##nn ] = &(node_spec_t){  \
                                    #nn, (member_spec_t[]){
#define MEMBER(nn, mn, ct, mt)  	    { mt, offsetof(node_t, nn.mn), #mn },
#define END(nn)                         { 0 }  \
                                    }  \
                                },

node_spec_p node_specs[] = {
	#include "ast_spec.h"
};

#undef BEGIN
#undef MEMBER
#undef END