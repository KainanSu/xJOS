#include <inc/assert.h>
#include <inc/arm.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

void sched_yield(void)
{
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment. Make sure curenv is not null before
	// dereferencing it.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// LAB 4: Your code here.
	envid_t i, nowid = curenv ? curenv->env_id : 0;

	for(i = 0; i < NENV; i++) {
		struct Env *nextenv = envs + ((nowid + i) % NENV);
		if(nextenv->env_status == ENV_RUNNABLE){
			env_run(nextenv); // no return
		}
	}
	if(curenv && curenv->env_status == ENV_RUNNING)
		env_run(curenv);

	panic("no environment to run.");
}