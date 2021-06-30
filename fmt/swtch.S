3050 # Context switch
3051 #
3052 #   void swtch(struct context **old, struct context *new);
3053 #
3054 # Save the current registers on the stack, creating
3055 # a struct context, and save its address in *old.
3056 # Switch stacks to new and pop previously-saved registers.
3057 
3058 .globl swtch
3059 swtch:
3060   movl 4(%esp), %eax
3061   movl 8(%esp), %edx
3062 
3063   # Save old callee-saved registers
3064   pushl %ebp
3065   pushl %ebx
3066   pushl %esi
3067   pushl %edi
3068 
3069   # Switch stacks
3070   movl %esp, (%eax)
3071   movl %edx, %esp
3072 
3073   # Load new callee-saved registers
3074   popl %edi
3075   popl %esi
3076   popl %ebx
3077   popl %ebp
3078   ret
3079 
3080 
3081 
3082 
3083 
3084 
3085 
3086 
3087 
3088 
3089 
3090 
3091 
3092 
3093 
3094 
3095 
3096 
3097 
3098 
3099 
