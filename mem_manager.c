/*======================================================
    > File Name: mem_manager.c
    > Author: lyh
    > E-mail:  
    > Other :  
    > Created Time: 2015年11月17日 星期二 14时22分17秒
 =======================================================*/

#include<stdio.h>
#include<stdlib.h>

#define  PROCESS_NAME_LEN  32 /*进程名称的最大长度*/
#define  MIN_SLICE         10 /*最小碎片的大小*/
#define  DEFAULT_MEM_SIZE  1024 /*默认内存的大小*/
#define  DEFAULT_MEM_START 0  /*默认内存的起始位置*/

/* 内存分配算法 */
#define  MA_FF   1
#define  MA_BF   2
#define  MA_WF   3

int  mem_size = DEFAULT_MEM_SIZE; /*内存大小*/
int  ma_algorithm = MA_FF;   /* 当前分配算法 */
static int pid = 0;          /* 初始pid */
int  flag = 0;               /* 设置内存大小标志 */

/* 描述每一个空闲块的数据结构 */
struct free_block_type
{
    int size;                //空闲块的大小
    int start_addr;          //空闲块的起始地址
    struct free_block_type  *next;
};

/* 指向内存中空闲块链表的首地址 */
struct free_block_type   *free_block;

/* 每个进程分配到的内存块的描述 */
struct allocated_block
{
    int  pid;               //进程标识号
    int  size;              //分配到的内存大小
    int  start_addr;        //起始地址
    char process_name[PROCESS_NAME_LEN];  //进程名字
    struct allocated_block  *next;
};

/* 进程分配内存块链表的首地址 */
struct allocated_block  *allocated_block_head = NULL;

/* 函数声明 */
struct free_block_type* init_free_block(int);     //初始化一块空闲区
void swap(int*,int*);                             //交换
void display_menu();                              //显示主菜单
int set_mem_size();                               //设置空闲区大小
void set_algorithm();                             //选择分配算法（菜单）
void rearrange(int);                              //按照指定的算法整理空闲区链表
void rearrange_FF();                              //最先适应算法First-fit
void rearrange_BF();                              //最佳适应算法Best-fit
void rearrange_WF();                              //最坏适应算法Worst-fit
int  new_process();                               //创建一个新的进程
int  allocate_mem(struct allocated_block*);       //分配内存模块
struct allocated_block* find_process(int);        //在已分配区中寻找进程
void kill_process();                              //销毁进程
int  free_mem(struct allocated_block*);           //将对应的已分配区归还，并进行可能的合并
int  dispose(struct allocated_block*);            //释放数据结构节点（已分配区）
int  display_mem_usage();                         //显示当前内存的使用情况（空闲区和已分配区）
void do_exit();                                   //结束前释放资源

int main()
{
    char   choice;
    pid=0;                        //初始值
    free_block = init_free_block(mem_size);//初始化空闲区
    for(;;)
    {
        display_menu();  //显示菜单
        //      fflush(stdin);
        choice = getchar(); //获取用户输入
        getchar();        //接收回车符
        switch(choice)
        {
            case '1':
                set_mem_size();
                break;   //设置内存大小
            case '2':
                set_algorithm();
                flag = 1;
                break;   //设置分配算法
            case '3':
                new_process();
                flag = 1;//以下的flag标志均为了防止在分配工作开始后再次设置空闲块大小
                break;   //创建新进程
            case '4':
                kill_process();
                flag = 1;
                break;  //删除进程
            case '5':
                display_mem_usage();
                flag = 1;
                break;  //显示内存使用
            case '0':
                do_exit();
                exit(0);  //释放链表并退出
            default:
                printf("default\n");
                break;
        }
    }
    return 0;
}

/* 交换 */
void swap(int* a,int* b)
{
    int  t;
    t = *a;
    *a = *b;
    *b = t;
}

/* 初始化空闲块，默认为一块，可以指定大小及起始地址 */
struct free_block_type* init_free_block(int mem_size)
{
    struct free_block_type  *fb;

    fb=(struct free_block_type *)malloc(sizeof(struct free_block_type));
    if(fb==NULL)
    {
        printf("No mem\n");
        return NULL;
    }
    fb->size=mem_size;   
    fb->start_addr = DEFAULT_MEM_START;   //初始化起始地址默认为0
    fb->next = NULL;
    return fb;
}

void display_menu()
{
    printf("\n");
    printf("-------------------------------------------\n");
    printf("1-Set memory size(default=%d)\n",DEFAULT_MEM_SIZE);
    printf("2-Select memory allocation ma_algorithm\n");
    printf("3-New process\n");
    printf("4-Terminate a process\n");
    printf("5-Display memory usage\n");
    printf("0-Exit\n");
    printf("-------------------------------------------\n");
}

/* 设置内存的大小 */
int set_mem_size()
{
    int  size;
    if(flag!=0)//防止重复设置
    {
        printf("Cannot set memory size again\n");
        return 0;
    }
    printf("Total memory size=");
    scanf("%d",&size); getchar();
    if(size>0)
    {
        mem_size = size;
        free_block->size = mem_size;
    }
    flag=1;
    return 1;
}

/* 设置当前的分配算法 */
void set_algorithm()
{
    int algorithm;
    printf("\t1-First Fit\n");
    printf("\t2-Best Fit\n");
    printf("\t3-Worst Fit\n");
    scanf("%d",&algorithm); getchar();
    if(algorithm>=1 && algorithm<=3)
        ma_algorithm=algorithm;
    //按指定算法重新排列空闲区链表
    rearrange(ma_algorithm);
    switch(ma_algorithm)
    {
        case MA_FF:
            printf("Rearrange free blocks for First-fit\n");
            break;
        case MA_BF:
            printf("Rearrange free blocks for Best-fit\n");
            break;
        case MA_WF:
            printf("Rearrange free blocks for Worst-fit\n");
            break;
    }
}

/* 按指定的算法整理内存空闲块链表 */
void rearrange(int algorithm)
{
    switch(algorithm)
    {
        case MA_FF:rearrange_FF();
                   break;
        case MA_BF:rearrange_BF();
                   break;
        case MA_WF:rearrange_WF();
                   break;
    }
}

/* 按FF算法重新整理内存空闲块链表 */
void rearrange_FF()
{
    struct free_block_type *tmp,*work;
    tmp = free_block;
    while(tmp!=NULL)  //选择排序
    {
        work = tmp->next;
        while(work!=NULL)   //每次找出最小的放最前面tmp指向的位置
        {
            if(work->start_addr < tmp->start_addr)
            {
                /*地址递增*/
                swap(&work->start_addr,&tmp->start_addr);
                swap(&work->size,&tmp->size);
            }
            else
            {
                work = work->next;
            }
        }
        tmp = tmp->next;
    }
}

/* 按BF算法重新整理内存空闲块链表 */
void rearrange_BF()
{
    struct free_block_type *tmp,*work;
    tmp = free_block;
    while(tmp!=NULL)
    {
        work = tmp->next;
        while(work!=NULL)
        {
            if(work->size < tmp->size)
            {
                /*内存大小递增*/
                swap(&work->start_addr,&tmp->start_addr);
                swap(&work->size,&tmp->size);
            }
            else
                work = work->next;
        }
        tmp=tmp->next;
    }
}

/* 按WF算法重新整理内存空闲块链表 */
void rearrange_WF()
{
    struct free_block_type *tmp,*work;
    tmp = free_block;
    while(tmp!=NULL)
    {
        work = tmp->next;
        while(work!=NULL)
        {
            if(work->size > tmp->size)
            {
                /*内存大小递减*/
                swap(&work->start_addr,&tmp->start_addr);
                swap(&work->size,&tmp->size);
            }
            else
                work = work->next;
        }
        tmp=tmp->next;
    }
}

/*创建新的进程，主要是获取内存的申请数量*/
int new_process()
{
    struct allocated_block *ab;
    int size;
    int ret;// 分配是否成功标志
    ab=(struct allocated_block *)malloc(sizeof(struct allocated_block));
    if(!ab) 
        exit(-5);    //exit()参数不等于0表示异常退出
    ab->next = NULL;
    pid++;//进程标识标志
    sprintf(ab->process_name, "PROCESS-%02d", pid);   //将格式化的字符串拷贝给第一个参数
    ab->pid = pid;

    printf("Memory for %s:", ab->process_name);
    scanf("%d", &size); getchar();
    if(size>0) 
        ab->size=size;
    ret = allocate_mem(ab);  /* 从空闲区分配内存，ret==1表示分配成功*/
    /*如果此时allocated_block_head尚未赋值，则赋值*/
    if((ret==1) &&(allocated_block_head == NULL))
    { 
        allocated_block_head=ab;
        rearrange(ma_algorithm);
        return 1;
    }
    /*分配成功，将该已分配块的描述插入已分配链表*/
    else if (ret==1) 
    {   
        //头插法
        rearrange(ma_algorithm);
        ab->next=allocated_block_head;
        allocated_block_head=ab;
        return 2;
    }
    else if(ret==-1)
    { /*分配不成功*/
        printf("Allocation fail\n");
        free(ab);
        return -1;
    }
    return 3;
}

/*分配内存模块*/
int allocate_mem(struct allocated_block *ab)
{
    struct free_block_type *fbt, *pre;
    int request_size=ab->size;
    fbt = pre = free_block;
    while(fbt!=NULL)
    {
        if(fbt->size>request_size)   //空闲块大于请求大小则分割，调整该空闲块大小
        {
            ab->start_addr = fbt->start_addr; // 新进程记录自己的起始地址
            fbt->start_addr +=request_size;   // 空闲块的起始地址值增加分配大小
            fbt->size = fbt->size-request_size;  //空闲块的大小减少分配大小
            return 1; 
        }
        else if(fbt->size==request_size)  //释放该空闲块
        {
            ab->start_addr = fbt->start_addr;
            pre = fbt;
            fbt = fbt->next;
            free(pre);
            return 1;
        }
        else
        {
            fbt = fbt->next;
        }
    }
    return -1;
}

/* 寻找进程 */
struct allocated_block* find_process(int pid)
{
    struct allocated_block*  fbt=NULL;
    fbt = allocated_block_head;
    while(fbt != NULL)
    {
        if(fbt->pid == pid)
            return fbt;
        fbt = fbt->next;
    }
    return NULL;
}

/*删除进程，归还分配的存储空间，并删除描述该进程内存分配的节点*/
void kill_process()
{
    struct allocated_block *ab;
    int pid;
    printf("Kill Process, pid=");
    scanf("%d", &pid); getchar();
    ab=find_process(pid);
    if(ab!=NULL)
    {
        free_mem(ab); /*释放ab所表示的分配区*/
        dispose(ab);  /*释放ab数据结构节点*/
    }
}
/*将ab所表示的已分配区归还，并进行可能的合并*/
int free_mem(struct allocated_block *ab)
{
    int algorithm = ma_algorithm;
    struct free_block_type *fbt,*work;
    //    struct free_block_type *pre;

    fbt=(struct free_block_type*) malloc(sizeof(struct free_block_type));
    if(!fbt) return -1;
    fbt->size = ab->size;
    fbt->start_addr = ab->start_addr;
    /*插入到空闲区链表的头部并将空闲区按地址递增的次序排列*/
    fbt->next = free_block;
    free_block=fbt;
    rearrange(MA_FF);  //为了能让相连的空闲区合并
    fbt=free_block;
    while(fbt!=NULL)
    {
        work = fbt->next;
        if(work!=NULL)
        {
            /*如果当前空闲区与后面的空闲区相连，则合并*/
            if(fbt->start_addr+fbt->size == work->start_addr)
            { 
                fbt->size += work->size;
                fbt->next = work->next;
                free(work);
                continue;
            }
        }
        fbt = fbt->next;
    }
    rearrange(algorithm); /*重新按当前的算法排列空闲区*/
    return 1;
}

/*释放ab数据结构节点*/
int dispose(struct allocated_block *free_ab)
{
    struct allocated_block *pre, *ab;

    if(free_ab == allocated_block_head) 
    { /*如果要释放第一个节点*/
        allocated_block_head = allocated_block_head->next;
        free(free_ab);
        return 1;
    }

    pre = allocated_block_head;  
    ab = allocated_block_head->next;

    while(ab!=free_ab)
    { 
        pre = ab;  
        ab = ab->next; 
    }
    pre->next = ab->next;
    free(ab);
    return 2;
}
/* 显示当前内存的使用情况，包括空闲区的情况和已经分配的情况 */

int display_mem_usage()
{
    struct free_block_type *fbt=free_block;
    struct allocated_block *ab=allocated_block_head;
    if(fbt==NULL) 
        return(-1);
    printf("----------------------------------------------------------\n");

    /* 显示空闲区 */
    printf("Free Memory:\n");
    printf("%20s %20s\n", "      start_addr", "       size");
    while(fbt!=NULL)
    {
        printf("%20d %20d\n", fbt->start_addr, fbt->size);
        fbt=fbt->next;
    }

    /* 显示已分配区 */
    printf("\nUsed Memory:\n");
    printf("%10s %20s %10s %10s\n", "PID", "ProcessName", "start_addr", " size");
    while(ab!=NULL)
    {
        printf("%10d %20s %10d %10d\n", ab->pid, ab->process_name, ab->start_addr, ab->size);
        ab=ab->next;
    }
    printf("----------------------------------------------------------\n");
    return 0;
}

void do_exit()
{
    if(free_block != NULL)
        free(free_block);
    if(allocated_block_head != NULL)
        free(allocated_block_head);
}
