#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sched.h> /* for all SCHED_/sched_ calls */
#include <sys/resource.h> /* for getpriority() */

const static int SCHED_POLICY_MAX = 7;
const char *sched_policies[] = {
    "SCHED_NORMAL",
    "SCHED_FIFO",
    "SCHED_RR",
    "SCHED_BATCH",
    "SCHED_ISO",
    "SCHED_IDLE",
    "SCHED_DEADLINE",
};

void print_process_prio(pid_t pid) {
    errno = 0;
    int this_prio = getpriority(PRIO_PROCESS, pid);
    if ((this_prio == -1) && (errno)) {
        perror("Syscall getpriority failed");
    } else {
        printf("Process priority is: %i\n", this_prio);
    }
}

void print_sched_type(pid_t pid) {
    int policy = sched_getscheduler(pid);
    if (policy < 0) {
        perror("Syscall sched_getscheduler() had problems");
        return;
    }
    if (policy > SCHED_POLICY_MAX) {
        printf("Syscall sched_getscheduler() returned a policy number greater than allowed!\n");
        return;
    }
    printf("The current scheduling policy is: %s\n", sched_policies[policy]);
}

void print_sched_priority(pid_t pid) {
    struct sched_param param;
    if (sched_getparam(pid, &param) < 0) {
        perror("Syscall sched_getparam barfed");
    } else {
        printf("The sched_param schedule priority is: %i\n", param.sched_priority);
    }
}

void hline(void) {
    char str[82];
    memset(str, '-', 80);
    str[80] = '\n';
    str[81] = '\0';
    printf("%s", str);
}

int main(int argc, char *argv[]) 
{
    int max_prio_FIFO = sched_get_priority_max(SCHED_FIFO);
    int max_prio_RR = sched_get_priority_max(SCHED_RR);
    int max_prio_OTHER = sched_get_priority_max(SCHED_OTHER);

    printf("Scheduling Priority Maximums:\n"
        " SCHED_FIFO: %i\n"
        " SCHED_RR: %i\n"
        " SCHED_OTHER: %i\n",
        max_prio_FIFO,
        max_prio_RR,
        max_prio_OTHER);

    int min_prio_FIFO = sched_get_priority_min(SCHED_FIFO);
    int min_prio_RR = sched_get_priority_min(SCHED_RR);
    int min_prio_OTHER = sched_get_priority_min(SCHED_OTHER);
    
    printf("Scheduling Priority Minimums:\n"
        " SCHED_FIFO: %i\n"
        " SCHED_RR: %i\n"
        " SCHED_OTHER: %i\n",
        min_prio_FIFO,
        min_prio_RR,
        min_prio_OTHER);
    
    print_process_prio(0);
    print_sched_priority(0);
    print_sched_type(0);
    
    hline();
    
    printf("Attempting to switch process to SCHED_FIFO...\n");
    
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = 50; /* This cannot be higher than max_prio_FIFO! */
    
    if (sched_setscheduler(0, SCHED_FIFO, &sp) < 0) {
        perror("Problem setting scheduling policy to SCHED_FIFO (probably need rtprio rule in /etc/security/limits.conf)");
        exit(1);
    }
    
    print_process_prio(0);
    print_sched_priority(0);
    print_sched_type(0);
    
    printf("\nTest completed successfully!\n");
}
