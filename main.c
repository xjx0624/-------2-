/* =========================================================
 *  main.c  —— 演示 / 自测用
 *
 *  完整流程：
 *      启动 -> load_tasks() 读 data.txt
 *      运行 -> 列出任务 / 报过期 / 显示今日进度
 *      退出 -> atexit 触发 save_tasks() 写回 data.txt
 * ========================================================= */

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS     /* 让 MSVC 别对 strcpy/localtime 报警告 */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "todo.h"

static Task g_tasks[MAX_TASKS];      /* 全局任务数组：atexit 里要能拿到 */
static int  g_count = 0;

/* ---------------- 退出时保存 ------------- */
/* atexit 注册的函数：main 正常 return、或中途调用 exit() 都会执行 */
static void save_on_exit(void)
{
    int rc = save_tasks(g_tasks, g_count, DATA_FILE);

    if (rc == 0) {
        printf("\n[保存] %d 条任务已写入 %s\n", g_count, DATA_FILE);
    } else {
        printf("\n[保存] 写 %s 失败！检查目录写权限\n", DATA_FILE);
    }
}

/* ---------------- 演示数据用的小工具 ------------- */
/* 把 date 往后挪 days 天（负数就是往前挪） */
static void add_days(char *date, int days)
{
    struct tm t;
    time_t tt;

    memset(&t, 0, sizeof(t));
    if (sscanf(date, "%d-%d-%d", &t.tm_year, &t.tm_mon, &t.tm_mday) != 3) {
        return;
    }
    t.tm_year -= 1900;
    t.tm_mon  -= 1;
    t.tm_hour  = 12;
    t.tm_isdst = -1;

    tt = mktime(&t);
    if (tt == (time_t)-1) {
        return;
    }
    tt += (time_t)days * (time_t)86400;

    t = *localtime(&tt);
    strftime(date, DATE_LEN, "%Y-%m-%d", &t);
}

static void add_task(int id, const char *title, const char *deadline, int done)
{
    if (g_count >= MAX_TASKS) {
        return;
    }
    g_tasks[g_count].id = id;
    strncpy(g_tasks[g_count].title, title, TITLE_MAX - 1);
    g_tasks[g_count].title[TITLE_MAX - 1] = '\0';
    strncpy(g_tasks[g_count].deadline, deadline, DATE_LEN - 1);
    g_tasks[g_count].deadline[DATE_LEN - 1] = '\0';
    g_tasks[g_count].done = done;
    g_count++;
}

/* 第一次运行（没有 data.txt）时放 4 条示例，覆盖三种情况：
 * 今天完成 / 今天没完成 / 未来到期 / 已经过期 */
static void seed_demo(void)
{
    char today[DATE_LEN];
    char future[DATE_LEN];

    if (get_today(today, DATE_LEN) != 0) {
        return;
    }

    add_task(101, "整理笔记", today, 1);

    strcpy(future, today);
    add_days(future, 3);
    add_task(102, "交课程设计文档", future, 0);

    strcpy(future, today);
    add_days(future, -1);
    add_task(103, "复习指针和数组", future, 0);   /* 这条会触发过期提醒 */

    add_task(104, "写实验报告", today, 0);
}

/* ---------------- 主程序 ---------------- */
int main(void)
{
    char today[DATE_LEN];
    int i;
    int done = 0;
    int total = 0;
    int overdue;
    double percent;

    /* 1) 先挂上"退出时保存"，这样后面任何返回路径都会存盘 */
    if (atexit(save_on_exit) != 0) {
        printf("[警告] atexit 注册失败，退出时不会自动保存\n");
    }

    /* 2) 启动时读取 */
    g_count = load_tasks(g_tasks, MAX_TASKS, DATA_FILE);
    if (g_count < 0) {
        printf("[读取] 没找到 %s，按空清单启动\n", DATA_FILE);
        g_count = 0;
        seed_demo();
        printf("[读取] 已放入 %d 条示例任务，退出后就会写进 %s\n", g_count, DATA_FILE);
    } else {
        printf("[读取] 从 %s 读入 %d 条任务\n", DATA_FILE, g_count);
    }

    /* 3) 今天的日期 */
    if (get_today(today, DATE_LEN) != 0) {
        strcpy(today, "????-??-??");
        printf("[今天] 取系统时间失败\n");
    } else {
        printf("[今天] %s\n", today);
    }

    printf("\n任务清单：\n");
    for (i = 0; i < g_count; i++) {
        printf("  [%s] #%-4d %-24s 截止 %s\n",
               g_tasks[i].done ? "x" : " ",
               g_tasks[i].id,
               g_tasks[i].title,
               g_tasks[i].deadline);
    }

    /* 4) 过期检查：过期的交给 3号 提醒 */
    printf("\n过期检查：\n");
    overdue = report_overdue(g_tasks, g_count);
    if (overdue == 0) {
        printf("  没有过期任务\n");
    }

    /* 5) 今日进度：算完交给 3号 画进度条 */
    percent = today_progress(g_tasks, g_count, &done, &total);
    printf("\n今日完成情况：\n");
    team3_show_progress(percent, done, total);

    /* 演示一下勾掉一条后进度会变（真实程序里这是用户按的那个键） */
    for (i = 0; i < g_count; i++) {
        if (g_tasks[i].done == 0 && strcmp(g_tasks[i].deadline, today) == 0) {
            g_tasks[i].done = 1;
            printf("\n[演示] 把 #%d \"%s\" 标记为已完成\n", g_tasks[i].id, g_tasks[i].title);
            break;
        }
    }
    percent = today_progress(g_tasks, g_count, &done, &total);
    team3_show_progress(percent, done, total);

    printf("\n准备退出（atexit 会自动保存）...\n");
    return 0;
}
