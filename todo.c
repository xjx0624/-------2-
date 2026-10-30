/* todo.c —— 任务清单里"存盘 / 读盘 / 取日期 / 判断过期 / 算进度"的实现 */

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS     /* 让 MSVC 别对 fopen/strcpy/localtime 报警告 */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "todo.h"

/* =========================================================
 *  内部小工具
 * ========================================================= */

/* 去掉字符串开头的空格和制表符。
 * 因为用 "%99[^\n]" 读标题时，标题前面那个分隔空格也会被一起吃进来。 */
static void ltrim(char *s)
{
    size_t i = 0;
    size_t j = 0;

    if (s == NULL) {
        return;
    }
    while (s[i] == ' ' || s[i] == '\t') {
        i++;
    }
    if (i == 0) {
        return;
    }
    while (s[i] != '\0') {
        s[j] = s[i];
        i++;
        j++;
    }
    s[j] = '\0';
}

/* 截止日期是不是长得像 YYYY-MM-DD（挡住手改坏的数据行） */
static int date_is_valid(const char *s)
{
    int i;

    if (s == NULL || strlen(s) != 10) {
        return 0;
    }
    for (i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            if (s[i] != '-') {
                return 0;
            }
        } else if (s[i] < '0' || s[i] > '9') {
            return 0;
        }
    }
    return 1;
}

/* 把 "YYYY-MM-DD" 换算成"第几天"，直接比大小就行。
 * 解析失败返回 -1。基准点定在中午，避开时区/夏令时把日期挪一天。 */
static long date_key(const char *s)
{
    int y = 0;
    int m = 0;
    int d = 0;
    struct tm t;
    time_t tt;

    if (s == NULL || sscanf(s, "%d-%d-%d", &y, &m, &d) != 3) {
        return -1;
    }
    if (m < 1 || m > 12 || d < 1 || d > 31) {
        return -1;
    }

    memset(&t, 0, sizeof(t));
    t.tm_year  = y - 1900;
    t.tm_mon   = m - 1;
    t.tm_mday  = d;
    t.tm_hour  = 12;
    t.tm_isdst = -1;

    tt = mktime(&t);
    if (tt == (time_t)-1) {
        return -1;
    }
    return (long)(tt / 86400L);
}

/* 一条任务是否合规（读文件时用） */
static int task_is_valid(const Task *t)
{
    if (t->id < 0) {
        return 0;
    }
    if (t->done != 0 && t->done != 1) {
        return 0;
    }
    return date_is_valid(t->deadline);
}

/* =========================================================
 *  1. 退出时保存
 * ========================================================= */
int save_tasks(const Task *tasks, int count, const char *path)
{
    FILE *fp;
    int i;
    int ok = 1;

    if (path == NULL) {
        path = DATA_FILE;
    }
    if (tasks == NULL && count > 0) {
        return -1;
    }

    fp = fopen(path, "w");                  /* 写模式：文件不存在就建，存在就清空重写 */
    if (fp == NULL) {
        return -1;                          /* 没有写权限 / 磁盘满 / 路径不对 */
    }

    for (i = 0; i < count; i++) {
        /* 一行一条：编号 是否完成 截止日期 标题（标题在最后，可以带空格） */
        if (fprintf(fp, "%d %d %s %s\n",
                    tasks[i].id,
                    tasks[i].done,
                    tasks[i].deadline,
                    tasks[i].title) < 0) {
            ok = 0;
            break;
        }
    }

    /* fclose 的返回值必须看：缓冲区里的数据是在这一步才真正落盘的 */
    if (fclose(fp) != 0) {
        ok = 0;
    }
    return ok ? 0 : -1;
}

/* =========================================================
 *  2. 启动时读取
 * ========================================================= */
int load_tasks(Task *tasks, int max_count, const char *path)
{
    FILE *fp;
    int count = 0;
    int ret;

    if (path == NULL) {
        path = DATA_FILE;
    }
    if (tasks == NULL || max_count <= 0) {
        return -1;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;                          /* 文件还不存在 = 第一次运行 */
    }

    while (count < max_count) {
        Task t;

        memset(&t, 0, sizeof(t));

        /* 前三个字段正常解析；标题用 %[^\n] 一直取到行尾，所以标题里
         * 可以有空格。注意 "%10s%99[^\n]" 中间不能加空格，否则格式串里的
         * 空白会把下一行的内容也吞掉。 */
        ret = fscanf(fp, "%d %d %10s%99[^\n]",
                     &t.id, &t.done, t.deadline, t.title);

        if (ret == EOF) {
            break;                          /* 到文件尾 */
        }
        if (ret < 3) {
            int c;                          /* 这一行坏得没法救：整行丢掉 */

            while ((c = fgetc(fp)) != EOF && c != '\n') {
                /* 什么也不做，只是往后挪到行尾 */
            }
            if (c == EOF) {
                break;
            }
            continue;
        }
        if (ret == 3) {
            t.title[0] = '\0';              /* 只有三个字段 = 标题是空的 */
        } else {
            ltrim(t.title);                 /* 去掉分隔用的那个前导空格 */
        }

        if (!task_is_valid(&t)) {
            continue;                       /* 跳过脏数据，但不占数组位置 */
        }
        tasks[count] = t;
        count++;
    }

    fclose(fp);
    return count;
}

/* =========================================================
 *  3. 取当前年月日
 * ========================================================= */
int get_today(char *buf, int size)
{
    time_t now;
    struct tm *lt;

    if (buf == NULL || size < DATE_LEN) {
        return -1;
    }

    now = time(NULL);
    if (now == (time_t)-1) {
        return -1;
    }

#if defined(_MSC_VER)
    {
        struct tm tmv;

        if (localtime_s(&tmv, &now) != 0) {
            return -1;
        }
        lt = &tmv;
        if (strftime(buf, (size_t)size, "%Y-%m-%d", lt) == 0) {
            return -1;
        }
    }
#else
    lt = localtime(&now);
    if (lt == NULL) {
        return -1;
    }
    if (strftime(buf, (size_t)size, "%Y-%m-%d", lt) == 0) {
        return -1;
    }
#endif

    return 0;
}

/* =========================================================
 *  4. 判断过期
 * ========================================================= */
int is_overdue(const Task *task)
{
    char today[DATE_LEN];
    long due;
    long now;

    if (task == NULL) {
        return 0;
    }
    if (get_today(today, (int)sizeof(today)) != 0) {
        return 0;                           /* 时间都取不到，就别误报 */
    }

    due = date_key(task->deadline);
    now = date_key(today);
    if (due < 0 || now < 0) {
        return 0;
    }

    /* 截止日 严格早于 今天 才算过期；今天到期 = 今天还得做。 */
    return (due < now) ? 1 : 0;
}

/* 扫描整个数组，把过期的任务逐条交给 3号 去提醒用户。
 * 返回过期任务条数。 */
int report_overdue(const Task *tasks, int count)
{
    char today[DATE_LEN];
    int i;
    int found = 0;

    if (tasks == NULL || count <= 0) {
        return 0;
    }
    if (get_today(today, (int)sizeof(today)) != 0) {
        strcpy(today, "????-??-??");
    }

    for (i = 0; i < count; i++) {
        if (is_overdue(&tasks[i])) {
            team3_report_overdue(&tasks[i], today);   /* 过期了 -> 告诉 3号 */
            found++;
        }
    }
    return found;
}

/* =========================================================
 *  5. 算今日进度
 * ========================================================= */
double today_progress(const Task *tasks, int count, int *done_out, int *total_out)
{
    char today[DATE_LEN];
    int i;
    int done = 0;
    int total = 0;
    double percent = 0.0;

    if (done_out != NULL) {
        *done_out = 0;
    }
    if (total_out != NULL) {
        *total_out = 0;
    }
    if (tasks == NULL || count <= 0) {
        return 0.0;
    }
    if (get_today(today, (int)sizeof(today)) != 0) {
        return 0.0;
    }

    for (i = 0; i < count; i++) {
        if (strcmp(tasks[i].deadline, today) == 0) {   /* 只统计今天到期的 */
            total++;
            if (tasks[i].done == 1) {
                done++;
            }
        }
    }

    if (total > 0) {
        percent = (double)done * 100.0 / (double)total;  /* 已完成数 / 今日总数 * 100 */
    } else {
        percent = 0.0;                                   /* 今日没有任务，除零要挡住 */
    }

    if (done_out != NULL) {
        *done_out = done;
    }
    if (total_out != NULL) {
        *total_out = total;
    }
    return percent;
}
