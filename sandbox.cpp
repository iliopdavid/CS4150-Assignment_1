#include <sys/prctl.h>
#include<signal.h>
#include<cassert>
#include <ucontext.h>
#include <sys/syscall.h>
#include <sched.h>
#include<iostream>
#include<unistd.h>
#include<fstream>
#include<iostream>
#ifndef SYS_USER_DISPATCH
# define SYS_USER_DISPATCH 2 /* syscall user dispatch triggered */
#endif

#ifndef CLONE_CLEAR_SIGHAND
#define CLONE_CLEAR_SIGHAND 0x100000000ULL
#endif

extern "C" void* (syscall_dispatcher_start)(void);
extern "C" void* (syscall_dispatcher_end)(void);
extern "C" long enter_syscall(int64_t, int64_t, int64_t, int64_t, int64_t, int64_t, int64_t);

char sud_selector = SYSCALL_DISPATCH_FILTER_ALLOW;
bool allowed_syscall_mask[501]={false};

template<size_t NUM>
static constexpr long long SYSCALL_ARG(const gregset_t gregs) {
	static_assert(NUM > 0 && NUM <= 6);
	return (long long[]){gregs[REG_RDI], gregs[REG_RSI], gregs[REG_RDX], gregs[REG_R10], gregs[REG_R8], gregs[REG_R9]}[NUM-1];
}

void ____asm_impl(void) {
	/*
	 * enter_syscall triggers a kernel-space system call
	 */
	asm volatile (
		".globl enter_syscall \n\t"
		"enter_syscall: \n\t"
		"movq %rdi, %rax \n\t"
		"movq %rsi, %rdi \n\t"
		"movq %rdx, %rsi \n\t"
		"movq %rcx, %rdx \n\t"
		"movq %r8, %r10 \n\t"
		"movq %r9, %r8 \n\t"
		"movq 8(%rsp),%r9 \n\t"
		"syscall \n\t"
		"ret \n\t"
	);
}

static void handle_sigsys(int sig, siginfo_t* info, void* ucontextv) {
        // allow selector to redirect to the process region where sys calls can be enabled/disabled
	sud_selector = SYSCALL_DISPATCH_FILTER_ALLOW;

	assert(sig == SIGSYS);
	assert(info->si_signo == SIGSYS);
	assert(info->si_code == SYS_USER_DISPATCH);
	assert(info->si_errno == 0);
        
        const auto uctxt = (ucontext_t*)ucontextv;
	const auto gregs = uctxt->uc_mcontext.gregs;
	assert(gregs[REG_RAX] == info->si_syscall);
	fprintf(stderr, "SUD trapping (syscall number = %d)\n", (int)info->si_syscall);
	const int syscall_num = (int)info->si_syscall;
	// emulate the system call
	if(allowed_syscall_mask[syscall_num]){

	  gregs[REG_RAX] = enter_syscall(info->si_syscall, SYSCALL_ARG<1>(gregs), SYSCALL_ARG<2>(gregs), SYSCALL_ARG<3>(gregs), SYSCALL_ARG<4>(gregs), SYSCALL_ARG<5>(gregs), SYSCALL_ARG<6>(gregs));
	  // block the  selector 
          sud_selector = SYSCALL_DISPATCH_FILTER_BLOCK;
          asm volatile (
		  "movq $0xf, %%rax \n\t"
		  "leaveq \n\t"
		  "add $0x8, %%rsp \n\t"
		  ".globl syscall_dispatcher_start \n\t"
		  "syscall_dispatcher_start: \n\t"
		  "syscall \n\t"
		  "nop \n\t"
		  ".globl syscall_dispatcher_end \n\t"
		  "syscall_dispatcher_end: \n\t"
		  :
		  :
		  : "rsp", "rax", "cc" // for some stupid reason adding rsp in the clobbers is considered deprecated ... so we need to use -Wno-deprecated flag
	  );
	}else { // kill the process if it is not allowed to make the  system call.
	fprintf(stderr, "[sandbox] BLOCKED SYSCALL: %d\n", syscall_num);
	  //kill process
	  _exit(EXIT_FAILURE);
	}
}

//load allowed system calls
static void load_allowed_syscalls(const char* filepath){
  std::ifstream file(filepath); 
  if(!file.is_open()){
    fprintf(stderr,"Failed to open the file %s,\n",filepath);
    exit(EXIT_FAILURE);
  }
  std::string line;
  while(std::getline(file,line)){
    if(line.empty() || line[0]=='#') { continue;}
    int num;
    if(sscanf(line.c_str(), "%d", &num)==1){
      allowed_syscall_mask[num]=true;
    }
  }
  file.close();
}

static void setup_sandbox(){
  // setup sigaction behaviour when it receives System calls
  struct sigaction act = {};
  act.sa_sigaction = handle_sigsys;
  act.sa_flags= SA_SIGINFO;
  if( sigemptyset(&act.sa_mask)!=0){
    perror("sigemptyset error");
    exit(EXIT_FAILURE);
  }
  if(sigaction(SIGSYS, &act, NULL) !=0){
    perror("sigaction error");
    exit(EXIT_FAILURE);
  }
  if(prctl(PR_SET_SYSCALL_USER_DISPATCH,PR_SYS_DISPATCH_ON,
  syscall_dispatcher_start,((int64_t)syscall_dispatcher_end - (int64_t)syscall_dispatcher_start + 1),&sud_selector) != 0){
    perror("prctl error");
    exit(EXIT_FAILURE);
  }
  
  sud_selector = SYSCALL_DISPATCH_FILTER_BLOCK;
}


// To run : "LD_PRELOAD=./sandbox.so <program_name"

__attribute__((constructor(0xffff)))
static void __sud_init(void) {
        load_allowed_syscalls("./syscalls.txt");
	fprintf(stderr, "starting \n");
	setup_sandbox();
}
