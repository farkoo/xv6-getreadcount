2400 #include "types.h"
2401 #include "defs.h"
2402 #include "param.h"
2403 #include "memlayout.h"
2404 #include "mmu.h"
2405 #include "x86.h"
2406 #include "proc.h"
2407 #include "spinlock.h"
2408 
2409 struct {
2410   struct spinlock lock;
2411   struct proc proc[NPROC];
2412 } ptable;
2413 
2414 static struct proc *initproc;
2415 
2416 int nextpid = 1;
2417 extern void forkret(void);
2418 extern void trapret(void);
2419 
2420 static void wakeup1(void *chan);
2421 
2422 void
2423 pinit(void)
2424 {
2425   initlock(&ptable.lock, "ptable");
2426 }
2427 
2428 // Must be called with interrupts disabled
2429 int
2430 cpuid() {
2431   return mycpu()-cpus;
2432 }
2433 
2434 // Must be called with interrupts disabled to avoid the caller being
2435 // rescheduled between reading lapicid and running through the loop.
2436 struct cpu*
2437 mycpu(void)
2438 {
2439   int apicid, i;
2440 
2441   if(readeflags()&FL_IF)
2442     panic("mycpu called with interrupts enabled\n");
2443 
2444   apicid = lapicid();
2445   // APIC IDs are not guaranteed to be contiguous. Maybe we should have
2446   // a reverse map, or reserve a register to store &cpus[i].
2447   for (i = 0; i < ncpu; ++i) {
2448     if (cpus[i].apicid == apicid)
2449       return &cpus[i];
2450   }
2451   panic("unknown apicid\n");
2452 }
2453 
2454 // Disable interrupts so that we are not rescheduled
2455 // while reading proc from the cpu structure
2456 struct proc*
2457 myproc(void) {
2458   struct cpu *c;
2459   struct proc *p;
2460   pushcli();
2461   c = mycpu();
2462   p = c->proc;
2463   popcli();
2464   return p;
2465 }
2466 
2467 
2468 // Look in the process table for an UNUSED proc.
2469 // If found, change state to EMBRYO and initialize
2470 // state required to run in the kernel.
2471 // Otherwise return 0.
2472 static struct proc*
2473 allocproc(void)
2474 {
2475   struct proc *p;
2476   char *sp;
2477 
2478   acquire(&ptable.lock);
2479 
2480   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2481     if(p->state == UNUSED)
2482       goto found;
2483 
2484   release(&ptable.lock);
2485   return 0;
2486 
2487 found:
2488   p->state = EMBRYO;
2489   p->pid = nextpid++;
2490 
2491   release(&ptable.lock);
2492 
2493   // Allocate kernel stack.
2494   if((p->kstack = kalloc()) == 0){
2495     p->state = UNUSED;
2496     return 0;
2497   }
2498   sp = p->kstack + KSTACKSIZE;
2499 
2500   // Leave room for trap frame.
2501   sp -= sizeof *p->tf;
2502   p->tf = (struct trapframe*)sp;
2503 
2504   // Set up new context to start executing at forkret,
2505   // which returns to trapret.
2506   sp -= 4;
2507   *(uint*)sp = (uint)trapret;
2508 
2509   sp -= sizeof *p->context;
2510   p->context = (struct context*)sp;
2511   memset(p->context, 0, sizeof *p->context);
2512   p->context->eip = (uint)forkret;
2513 
2514   return p;
2515 }
2516 
2517 
2518 // Set up first user process.
2519 void
2520 userinit(void)
2521 {
2522   struct proc *p;
2523   extern char _binary_initcode_start[], _binary_initcode_size[];
2524 
2525   p = allocproc();
2526 
2527   initproc = p;
2528   if((p->pgdir = setupkvm()) == 0)
2529     panic("userinit: out of memory?");
2530   inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
2531   p->sz = PGSIZE;
2532   memset(p->tf, 0, sizeof(*p->tf));
2533   p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
2534   p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
2535   p->tf->es = p->tf->ds;
2536   p->tf->ss = p->tf->ds;
2537   p->tf->eflags = FL_IF;
2538   p->tf->esp = PGSIZE;
2539   p->tf->eip = 0;  // beginning of initcode.S
2540 
2541   safestrcpy(p->name, "initcode", sizeof(p->name));
2542   p->cwd = namei("/");
2543 
2544   // this assignment to p->state lets other cores
2545   // run this process. the acquire forces the above
2546   // writes to be visible, and the lock is also needed
2547   // because the assignment might not be atomic.
2548   acquire(&ptable.lock);
2549 
2550   p->state = RUNNABLE;
2551 
2552   release(&ptable.lock);
2553 }
2554 
2555 // Grow current process's memory by n bytes.
2556 // Return 0 on success, -1 on failure.
2557 int
2558 growproc(int n)
2559 {
2560   uint sz;
2561   struct proc *curproc = myproc();
2562 
2563   sz = curproc->sz;
2564   if(n > 0){
2565     if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
2566       return -1;
2567   } else if(n < 0){
2568     if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
2569       return -1;
2570   }
2571   curproc->sz = sz;
2572   switchuvm(curproc);
2573   return 0;
2574 }
2575 
2576 // Create a new process copying p as the parent.
2577 // Sets up stack to return as if from system call.
2578 // Caller must set state of returned proc to RUNNABLE.
2579 int
2580 fork(void)
2581 {
2582   int i, pid;
2583   struct proc *np;
2584   struct proc *curproc = myproc();
2585 
2586   // Allocate process.
2587   if((np = allocproc()) == 0){
2588     return -1;
2589   }
2590 
2591   // Copy process state from proc.
2592   if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
2593     kfree(np->kstack);
2594     np->kstack = 0;
2595     np->state = UNUSED;
2596     return -1;
2597   }
2598   np->sz = curproc->sz;
2599   np->parent = curproc;
2600   *np->tf = *curproc->tf;
2601 
2602   // Clear %eax so that fork returns 0 in the child.
2603   np->tf->eax = 0;
2604 
2605   for(i = 0; i < NOFILE; i++)
2606     if(curproc->ofile[i])
2607       np->ofile[i] = filedup(curproc->ofile[i]);
2608   np->cwd = idup(curproc->cwd);
2609 
2610   safestrcpy(np->name, curproc->name, sizeof(curproc->name));
2611 
2612   pid = np->pid;
2613 
2614   acquire(&ptable.lock);
2615 
2616   np->state = RUNNABLE;
2617 
2618   release(&ptable.lock);
2619 
2620   return pid;
2621 }
2622 
2623 // Exit the current process.  Does not return.
2624 // An exited process remains in the zombie state
2625 // until its parent calls wait() to find out it exited.
2626 void
2627 exit(void)
2628 {
2629   struct proc *curproc = myproc();
2630   struct proc *p;
2631   int fd;
2632 
2633   if(curproc == initproc)
2634     panic("init exiting");
2635 
2636   // Close all open files.
2637   for(fd = 0; fd < NOFILE; fd++){
2638     if(curproc->ofile[fd]){
2639       fileclose(curproc->ofile[fd]);
2640       curproc->ofile[fd] = 0;
2641     }
2642   }
2643 
2644   begin_op();
2645   iput(curproc->cwd);
2646   end_op();
2647   curproc->cwd = 0;
2648 
2649   acquire(&ptable.lock);
2650   // Parent might be sleeping in wait().
2651   wakeup1(curproc->parent);
2652 
2653   // Pass abandoned children to init.
2654   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2655     if(p->parent == curproc){
2656       p->parent = initproc;
2657       if(p->state == ZOMBIE)
2658         wakeup1(initproc);
2659     }
2660   }
2661 
2662   // Jump into the scheduler, never to return.
2663   curproc->state = ZOMBIE;
2664   sched();
2665   panic("zombie exit");
2666 }
2667 
2668 // Wait for a child process to exit and return its pid.
2669 // Return -1 if this process has no children.
2670 int
2671 wait(void)
2672 {
2673   struct proc *p;
2674   int havekids, pid;
2675   struct proc *curproc = myproc();
2676 
2677   acquire(&ptable.lock);
2678   for(;;){
2679     // Scan through table looking for exited children.
2680     havekids = 0;
2681     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2682       if(p->parent != curproc)
2683         continue;
2684       havekids = 1;
2685       if(p->state == ZOMBIE){
2686         // Found one.
2687         pid = p->pid;
2688         kfree(p->kstack);
2689         p->kstack = 0;
2690         freevm(p->pgdir);
2691         p->pid = 0;
2692         p->parent = 0;
2693         p->name[0] = 0;
2694         p->killed = 0;
2695         p->state = UNUSED;
2696         release(&ptable.lock);
2697         return pid;
2698       }
2699     }
2700     // No point waiting if we don't have any children.
2701     if(!havekids || curproc->killed){
2702       release(&ptable.lock);
2703       return -1;
2704     }
2705 
2706     // Wait for children to exit.  (See wakeup1 call in proc_exit.)
2707     sleep(curproc, &ptable.lock);  //DOC: wait-sleep
2708   }
2709 }
2710 
2711 
2712 
2713 
2714 
2715 
2716 
2717 
2718 
2719 
2720 
2721 
2722 
2723 
2724 
2725 
2726 
2727 
2728 
2729 
2730 
2731 
2732 
2733 
2734 
2735 
2736 
2737 
2738 
2739 
2740 
2741 
2742 
2743 
2744 
2745 
2746 
2747 
2748 
2749 
2750 // Per-CPU process scheduler.
2751 // Each CPU calls scheduler() after setting itself up.
2752 // Scheduler never returns.  It loops, doing:
2753 //  - choose a process to run
2754 //  - swtch to start running that process
2755 //  - eventually that process transfers control
2756 //      via swtch back to the scheduler.
2757 void
2758 scheduler(void)
2759 {
2760   struct proc *p;
2761   struct cpu *c = mycpu();
2762   c->proc = 0;
2763 
2764   for(;;){
2765     // Enable interrupts on this processor.
2766     sti();
2767 
2768     // Loop over process table looking for process to run.
2769     acquire(&ptable.lock);
2770     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2771       if(p->state != RUNNABLE)
2772         continue;
2773 
2774       // Switch to chosen process.  It is the process's job
2775       // to release ptable.lock and then reacquire it
2776       // before jumping back to us.
2777       c->proc = p;
2778       switchuvm(p);
2779       p->state = RUNNING;
2780 
2781       swtch(&(c->scheduler), p->context);
2782       switchkvm();
2783 
2784       // Process is done running for now.
2785       // It should have changed its p->state before coming back.
2786       c->proc = 0;
2787     }
2788     release(&ptable.lock);
2789 
2790   }
2791 }
2792 
2793 
2794 
2795 
2796 
2797 
2798 
2799 
2800 // Enter scheduler.  Must hold only ptable.lock
2801 // and have changed proc->state. Saves and restores
2802 // intena because intena is a property of this
2803 // kernel thread, not this CPU. It should
2804 // be proc->intena and proc->ncli, but that would
2805 // break in the few places where a lock is held but
2806 // there's no process.
2807 void
2808 sched(void)
2809 {
2810   int intena;
2811   struct proc *p = myproc();
2812 
2813   if(!holding(&ptable.lock))
2814     panic("sched ptable.lock");
2815   if(mycpu()->ncli != 1)
2816     panic("sched locks");
2817   if(p->state == RUNNING)
2818     panic("sched running");
2819   if(readeflags()&FL_IF)
2820     panic("sched interruptible");
2821   intena = mycpu()->intena;
2822   swtch(&p->context, mycpu()->scheduler);
2823   mycpu()->intena = intena;
2824 }
2825 
2826 // Give up the CPU for one scheduling round.
2827 void
2828 yield(void)
2829 {
2830   acquire(&ptable.lock);  //DOC: yieldlock
2831   myproc()->state = RUNNABLE;
2832   sched();
2833   release(&ptable.lock);
2834 }
2835 
2836 
2837 
2838 
2839 
2840 
2841 
2842 
2843 
2844 
2845 
2846 
2847 
2848 
2849 
2850 // A fork child's very first scheduling by scheduler()
2851 // will swtch here.  "Return" to user space.
2852 void
2853 forkret(void)
2854 {
2855   static int first = 1;
2856   // Still holding ptable.lock from scheduler.
2857   release(&ptable.lock);
2858 
2859   if (first) {
2860     // Some initialization functions must be run in the context
2861     // of a regular process (e.g., they call sleep), and thus cannot
2862     // be run from main().
2863     first = 0;
2864     iinit(ROOTDEV);
2865     initlog(ROOTDEV);
2866   }
2867 
2868   // Return to "caller", actually trapret (see allocproc).
2869 }
2870 
2871 // Atomically release lock and sleep on chan.
2872 // Reacquires lock when awakened.
2873 void
2874 sleep(void *chan, struct spinlock *lk)
2875 {
2876   struct proc *p = myproc();
2877 
2878   if(p == 0)
2879     panic("sleep");
2880 
2881   if(lk == 0)
2882     panic("sleep without lk");
2883 
2884   // Must acquire ptable.lock in order to
2885   // change p->state and then call sched.
2886   // Once we hold ptable.lock, we can be
2887   // guaranteed that we won't miss any wakeup
2888   // (wakeup runs with ptable.lock locked),
2889   // so it's okay to release lk.
2890   if(lk != &ptable.lock){  //DOC: sleeplock0
2891     acquire(&ptable.lock);  //DOC: sleeplock1
2892     release(lk);
2893   }
2894   // Go to sleep.
2895   p->chan = chan;
2896   p->state = SLEEPING;
2897 
2898   sched();
2899 
2900   // Tidy up.
2901   p->chan = 0;
2902 
2903   // Reacquire original lock.
2904   if(lk != &ptable.lock){  //DOC: sleeplock2
2905     release(&ptable.lock);
2906     acquire(lk);
2907   }
2908 }
2909 
2910 
2911 
2912 
2913 
2914 
2915 
2916 
2917 
2918 
2919 
2920 
2921 
2922 
2923 
2924 
2925 
2926 
2927 
2928 
2929 
2930 
2931 
2932 
2933 
2934 
2935 
2936 
2937 
2938 
2939 
2940 
2941 
2942 
2943 
2944 
2945 
2946 
2947 
2948 
2949 
2950 // Wake up all processes sleeping on chan.
2951 // The ptable lock must be held.
2952 static void
2953 wakeup1(void *chan)
2954 {
2955   struct proc *p;
2956 
2957   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2958     if(p->state == SLEEPING && p->chan == chan)
2959       p->state = RUNNABLE;
2960 }
2961 
2962 // Wake up all processes sleeping on chan.
2963 void
2964 wakeup(void *chan)
2965 {
2966   acquire(&ptable.lock);
2967   wakeup1(chan);
2968   release(&ptable.lock);
2969 }
2970 
2971 // Kill the process with the given pid.
2972 // Process won't exit until it returns
2973 // to user space (see trap in trap.c).
2974 int
2975 kill(int pid)
2976 {
2977   struct proc *p;
2978 
2979   acquire(&ptable.lock);
2980   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2981     if(p->pid == pid){
2982       p->killed = 1;
2983       // Wake process from sleep if necessary.
2984       if(p->state == SLEEPING)
2985         p->state = RUNNABLE;
2986       release(&ptable.lock);
2987       return 0;
2988     }
2989   }
2990   release(&ptable.lock);
2991   return -1;
2992 }
2993 
2994 
2995 
2996 
2997 
2998 
2999 
3000 // Print a process listing to console.  For debugging.
3001 // Runs when user types ^P on console.
3002 // No lock to avoid wedging a stuck machine further.
3003 void
3004 procdump(void)
3005 {
3006   static char *states[] = {
3007   [UNUSED]    "unused",
3008   [EMBRYO]    "embryo",
3009   [SLEEPING]  "sleep ",
3010   [RUNNABLE]  "runble",
3011   [RUNNING]   "run   ",
3012   [ZOMBIE]    "zombie"
3013   };
3014   int i;
3015   struct proc *p;
3016   char *state;
3017   uint pc[10];
3018 
3019   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
3020     if(p->state == UNUSED)
3021       continue;
3022     if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
3023       state = states[p->state];
3024     else
3025       state = "???";
3026     cprintf("%d %s %s", p->pid, state, p->name);
3027     if(p->state == SLEEPING){
3028       getcallerpcs((uint*)p->context->ebp+2, pc);
3029       for(i=0; i<10 && pc[i] != 0; i++)
3030         cprintf(" %p", pc[i]);
3031     }
3032     cprintf("\n");
3033   }
3034 }
3035 
3036 
3037 
3038 
3039 
3040 
3041 
3042 
3043 
3044 
3045 
3046 
3047 
3048 
3049 
