#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>

uint32_t g_sleep_ms = 0; 
uint32_t g_threadcnt = 0;
uint32_t g_running_threadcnt = 0;
uint64_t g_SleepIoCount = 0;
int32_t  g_main_bind = -1;
int32_t  g_task_bind = -1;

#define USE_CORE_BIND 1
#define MSLEEP(x) usleep(1000 * (x))
#define ATOMIC_FETCH_AND_ADD(ptr,value)    __sync_fetch_and_add((ptr), (value))


void *sleep_task(void* para)
{

    if (g_task_bind >= 0)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(g_task_bind, &mask);
        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        {		
            printf("Bind to Core Error !\n");
        return NULL;
        }
    }  
    
    ATOMIC_FETCH_AND_ADD(&g_running_threadcnt, 1);
    while(1)
    {
        MSLEEP(g_sleep_ms);
        ATOMIC_FETCH_AND_ADD(&g_SleepIoCount, 1);
    }
}

static inline pid_t gettid(void){
    return syscall(SYS_gettid);
}

void execute_cmd(const char *cmd, char *result)   
{   
    char buf_ps[1024];   
    char ps[1024]={0};   
    FILE *ptr;   
    strcpy(ps, cmd);   
    if((ptr=popen(ps, "r"))!=NULL)   
    {   
        while(fgets(buf_ps, 1024, ptr)!=NULL)   
        {   
           strcat(result, buf_ps);   
           if(strlen(result)>1024)   
               break;   
        }   
        pclose(ptr);   
        ptr = NULL;   
    }   
    else  
    {   
        printf("popen %s error\n", ps);   
    }   
}  

void print_process_info(void)
{
    char cmd_string[128] = {0};
    char cmd_result[128] = {0};
    
    pid_t my_pid = gettid();
    memset(cmd_string, 0, sizeof(cmd_string));
    memset(cmd_result, 0, sizeof(cmd_result));
    sprintf(cmd_string, "cat /proc/%u/status | grep VmRSS | cut -d : -f 2 | tr -cd \"[0-9]\"", (uint32_t)my_pid); 
    execute_cmd(cmd_string, cmd_result);
    printf("Current Process Used %s physical memory !!!!\n", cmd_result);    

    memset(cmd_string, 0, sizeof(cmd_string));
    memset(cmd_result, 0, sizeof(cmd_result));
    sprintf(cmd_string, "cat /proc/%u/status | grep VmSize | cut -d : -f 2 | tr -cd \"[0-9]\"", (uint32_t)my_pid);
    execute_cmd(cmd_string, cmd_result);
    printf("Current Process Used %s virtual memory !!!!\n", cmd_result);

    memset(cmd_string, 0, sizeof(cmd_string));
    memset(cmd_result, 0, sizeof(cmd_result));
    sprintf(cmd_string, "cat /proc/%u/status | grep Threads | cut -d : -f 2 | tr -cd \"[0-9]\"", (uint32_t)my_pid);
    execute_cmd(cmd_string, cmd_result);
    printf("Current Process Used %s threads !!!!\n", cmd_result);

    sleep(3);
    return ;
}

void main(int argc, void* argv[])
{
    if (argc != 5)
    {
        printf("Usage:$s thread_cnt sleep_ms main_bind task_bind \n", argv[0]);
        return;
    }
    
    g_threadcnt = atoi(argv[1]);
    g_sleep_ms = atoi(argv[2]);
    g_main_bind = atoi(argv[3]);
    g_task_bind = atoi(argv[4]);

    if (g_main_bind >= 0)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(1, &mask);
        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        {
            printf("Main Bind to Core Error !\n");
            return;
        }
    }
 
    int ret = 0;
    uint32_t i;
    pthread_t thread;
    for (i = 0; i < g_threadcnt; i++)
    {
        if (i % 5000 == 0)
        {
            printf("Already create %d threads ....\n", i);
        }
    
        ret = pthread_create(&thread, NULL, &sleep_task, NULL);
        if (0 != ret)
        {
            printf("[ERROR]Create thread error, index:%d, ret:%d!!!\n", i, ret);
            return;
        }
    }
    
    //waiting for thread all running
    while (g_running_threadcnt != g_threadcnt)
    {
        printf("Running:%d  -  Total:%d \n", g_running_threadcnt, g_threadcnt);
        sleep(1);
    }
    printf("All the %d threads is running ....\n", g_running_threadcnt);

    print_process_info();
 
    //excute the test
    uint64_t last_cnt = 0;
    int test_cnt = 0;
    for (; test_cnt < 50; test_cnt++)
    {
        sleep(3);
        if (test_cnt != 0)
            printf("Sleep Iops %d  \n",(g_SleepIoCount-last_cnt)/3);
        last_cnt = g_SleepIoCount; // maybe not accurate ...   
    }
   
    print_process_info(); 
    return;
}
