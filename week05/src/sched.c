#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <sched.h> /* for all SCHED_/sched_ calls */
#include <sys/resource.h> /* for getpriority() */

#define NLOOP_FOR_ESTIMATION 1000000000UL
#define NSECS_PER_MSEC 1000000UL
#define NSECS_PER_SEC 1000000000UL

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

/*
    before와 after의 시간 차이를 ns 단위로 리턴
    빠른 실행을 위해 inline 키워드 사용
*/
static inline unsigned long diff_nsec(struct timespec before, struct timespec after)
{
    return ((after.tv_sec * NSECS_PER_SEC + after.tv_nsec) \
            - (before.tv_sec * NSECS_PER_SEC + before.tv_nsec));
}

// 1ms 당 반복문을 몇번 돌 수 있는지 (성능 기준치 측정)
static unsigned long loops_per_msec()
{
    struct timespec before, after;

    clock_gettime(CLOCK_MONOTONIC, &before); // Monotonic system-wide clock id 사용
    for (unsigned long i = 0; i < NLOOP_FOR_ESTIMATION; ++i)
        ;
    clock_gettime(CLOCK_MONOTONIC, &after);

    // 1ms 당 반복 횟수 = 측정 횟수 / 걸린 시간[ms]
    // return NLOOP_FOR_ESTIMATION / (diff_nsec(before, after) / NSECS_PER_MSEC);
    return NLOOP_FOR_ESTIMATION * NSECS_PER_MSEC / diff_nsec(before, after);
}

// 단순히 반복문으로 부하를 주는 함수
static inline void load(unsigned long nloop)
{
    for (unsigned long i = 0; i < nloop; ++i)
        ;
}

static void child_fn(int id, struct timespec *buf, int nrecord, \
                    unsigned long nloop_per_resol, struct timespec start)
{
    for (int i = 0; i < nrecord; ++i)
    {
        struct timespec ts;

        load(nloop_per_resol);
        clock_gettime(CLOCK_MONOTONIC, &ts);
        buf[i] = ts;
    }
    for (int i = 0; i < nrecord; ++i)
        // 프로세스고유ID(0 ~ nproc-1), 프로그램시작시점부터경과한시간, 진행도[%]
        printf("%d\t%lu\t%d\n", id, diff_nsec(start, buf[i]) / NSECS_PER_MSEC, \
            100 * (i + 1) / nrecord);

    exit(EXIT_SUCCESS);
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
    fprintf(stderr, "The current scheduling policy is: %s\n", sched_policies[policy]);
}

int main(int argc, char *argv[])
{
    if (argc > 5){
        err(EXIT_FAILURE, "Usage: sched <nproc> <total[ms]> <resolution[ms]> <scheduling algorithm>\n");
        exit(1);
    }     

    int nproc = atoi(argv[1]);
    int total = atoi(argv[2]);
    int resol = atoi(argv[3]);
    int sched = -1;

    for(int i = 0; i < SCHED_POLICY_MAX; i++){
        if (!strcmp(argv[4], sched_policies[i])){
            sched = i;
            fprintf(stderr, "Switch process to %s\n", sched_policies[sched]);
            break;
        }
    }

    if (nproc < 1 || total < 1 || resol < 1)
        err(EXIT_FAILURE, "parameters must be positive value");
    else if (total % resol)
        err(EXIT_FAILURE, "<total> must be multiple of <resolution>");
    else if (sched != 1 && sched != 2)
        err(EXIT_FAILURE, "Unsupported scheduling algorithm");

    int nrecord = total / resol;

    struct timespec *logbuf = malloc(nrecord * sizeof(struct timespec));
    if (!logbuf)
        err(EXIT_FAILURE, "malloc(logbuf) failed");

    // resolution 당 반복 횟수
    unsigned long nloop_per_msec = loops_per_msec();
    unsigned long nloop_per_resol = nloop_per_msec * resol;

    pid_t *pids = malloc(nproc * sizeof(pid_t));
    if (!pids)
        err(EXIT_FAILURE, "malloc(pids) failed");

    print_sched_type(0);

    fprintf(stderr, "Attempting to switch process to %s...\n", sched_policies[sched]);
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = 50; /* This cannot be higher than max_prio_FIFO! */
    if (sched_setscheduler(0, sched, &sp) < 0) {
        fprintf(stderr, "Problem setting scheduling policy to %s \
                        (probably need rtprio rule in /etc/security/limits.conf)", sched_policies[sched]);
        exit(1);
    }

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int ret = EXIT_SUCCESS;
    int ncreated = 0;
    for (int i = 0; i < nproc; ++i, ++ncreated)
    {
        pids[i] = fork();
        if (pids[i] < 0)
        {
            for (int j = 0; j < ncreated; ++j)
                kill(pids[j], SIGKILL);
            ret = EXIT_FAILURE;
            break ;
        }
        else if (pids[i] == 0)
            child_fn(i, logbuf, nrecord, nloop_per_resol, start);
    }

    for (int i = 0; i < ncreated; ++i)
        if (wait(NULL) < 0)
            warn("wait() failed");

    return ret;
}
