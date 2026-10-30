#ifndef TODO_H
#define TODO_H

/* =========================================================
 *  todo.h  —— 任务清单：存储 / 时间 / 过期 / 进度
 *
 *  需求对应关系：
 *    1. 退出时保存        -> save_tasks()
 *    2. 启动时读取        -> load_tasks()
 *    3. 取当前年月日      -> get_today()
 *    4. 判断过期          -> is_overdue()  + team3_report_overdue()
 *    5. 算进度条          -> today_progress() + team3_show_progress()
 *
 *  数据文件 data.txt 每行一条任务，字段用空格分开，标题放最后：
 *      编号 是否完成 截止日期 标题
 *      101 0 2026-10-30 写实验报告
 *      (done: 0=未完成, 1=已完成; 截止日期固定 YYYY-MM-DD)
 * ========================================================= */

#define MAX_TASKS   128          /* 最多存多少条任务 */
#define TITLE_MAX   100          /* 标题缓冲区大小（含 '\0'） */
#define DATE_LEN    11           /* "YYYY-MM-DD" + '\0' */

#define DATA_FILE   "data.txt"   /* 数据文件名 */

typedef struct {
    int  id;                     /* 任务编号 */
    char title[TITLE_MAX];       /* 任务标题 */
    char deadline[DATE_LEN];     /* 截止日期 "YYYY-MM-DD" */
    int  done;                   /* 0 = 未完成, 1 = 已完成 */
} Task;

/* ---------------- 1. 保存 ---------------- */
/* 把 tasks 里的 count 条任务全部写到 path（覆盖写）。
 * 返回 0 = 成功，-1 = 失败（打不开文件 / 写不进去 / fclose 失败）。 */
int save_tasks(const Task *tasks, int count, const char *path);

/* ---------------- 2. 读取 ---------------- */
/* 从 path 读任务，最多读 max_count 条。
 * 返回 实际读到的条数；-1 = 文件打不开（调用者按"第一次运行"处理）。
 * 格式不对的行会被跳过，不会把脏数据塞进数组。 */
int load_tasks(Task *tasks, int max_count, const char *path);

/* ---------------- 3. 今天的日期 ---------------- */
/* 取系统当前日期，按 "YYYY-MM-DD" 写进 buf（size 至少 11）。
 * 返回 0 = 成功，-1 = 失败。 */
int get_today(char *buf, int size);

/* ---------------- 4. 是否过期 ---------------- */
/* 任务截止日 早于 今天 -> 返回 1（过期），否则 0。
 * 注意：今天到期的不算过期（还要今天做），想改成"今天也算过期"见 README。 */
int is_overdue(const Task *task);

/* 把数组里所有过期的任务逐条交给 3号 提醒，返回过期条数。
 * 主循环里只需调这一个，不用自己写 for 循环。 */
int report_overdue(const Task *tasks, int count);

/* ---------------- 5. 今日进度 ---------------- */
/* 统计"截止日 == 今天"的任务：done_out = 已完成数，total_out = 今日总数，
 * 返回值 = 完成百分比 0.0 ~ 100.0（今日没有任务时返回 0.0）。
 * done_out / total_out 传 NULL 表示不需要。 */
double today_progress(const Task *tasks, int count, int *done_out, int *total_out);

/* =========================================================
 *   ↓↓↓ 以下两个函数由 3号 实现（他负责界面/打印）↓↓↓
 *   他只要按这两个原型写函数，本模块算完就调他。
 *   现在先由 team3_stub.c 顶着，等他写好就把那个文件从编译里去掉。
 * ========================================================= */

/* 4 号（本模块）发现某条任务过期时调用：
 *   task  = 过期的那条任务
 *   today = 今天的日期字符串 */
void team3_report_overdue(const Task *task, const char *today);

/* 5 号（本模块）算完进度后调用：
 *   percent = 0.0 ~ 100.0
 *   done    = 已完成数
 *   total   = 今日总数（可能是 0） */
void team3_show_progress(double percent, int done, int total);

#endif /* TODO_H */
