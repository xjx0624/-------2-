/* =========================================================
 *  team3_stub.c  —— 3号 的接口占位实现
 *
 *  这两个函数本来归 3号 写（他负责界面/打印）。
 *  在 3号 交代码之前，先用这个文件让程序能跑起来、能演示。
 *
 *  3号 的产品到了以后：
 *      1) 把这两个函数放到他自己的 .c 里实现；
 *      2) 编译时不要带上 team3_stub.c，否则会报"重复定义"。
 * ========================================================= */

#include <stdio.h>

#include "todo.h"

/* 需求 4：过期了要"告诉 3号"，这里就是 3号 的提醒界面 */
void team3_report_overdue(const Task *task, const char *today)
{
    if (task == NULL) {
        return;
    }
    printf("  [3号提醒] 任务 #%d \"%s\" 已于 %s 到期，今天已是 %s，过期了！\n",
           task->id, task->title, task->deadline, today);
}

/* 需求 5：进度算好了交给 3号 画进度条 */
void team3_show_progress(double percent, int done, int total)
{
    const int width = 20;       /* 进度条一共 20 个格子 */
    int filled;
    int i;

    if (percent < 0.0) {
        percent = 0.0;
    }
    if (percent > 100.0) {
        percent = 100.0;
    }

    /* 四舍五入到格子数，0.1% 的差别也画得出来 */
    filled = (int)(percent * (double)width / 100.0 + 0.5);
    if (filled > width) {
        filled = width;
    }

    if (total <= 0) {
        printf("  今日进度 [");
        for (i = 0; i < width; i++) {
            putchar('-');
        }
        printf("]   0.0%%  （今天没有到期的任务）\n");
        return;
    }

    printf("  今日进度 [");
    for (i = 0; i < width; i++) {
        putchar(i < filled ? '#' : '-');
    }
    printf("] %5.1f%%  (%d/%d)\n", percent, done, total);
}
