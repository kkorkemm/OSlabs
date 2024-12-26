#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/proc_fs.h> 
#include <linux/uaccess.h> // Для copy_to_user
#include <linux/version.h>

MODULE_LICENSE("GPL");

// имя файла /proc
#define PROC_FILE_NAME "lab4"

// Указатель на файл /proc
static struct proc_dir_entry *our_proc_file;

// Функция для вычисления минут с момента погружения Титаника
static unsigned long calculate_minutes_since_titanic(void) {
    struct tm titanic_time = {
        .tm_year = 1912 - 1900, // Год (начиная с 1900)
        .tm_mon = 4 - 1,        // Месяц (апрель, начиная с 0)
        .tm_mday = 15, 
        .tm_hour = 2, 
        .tm_min = 20,
        .tm_sec = 0,            
        .tm_isdst = -1          // Автоматическое определение летнего времени
    };

    struct timespec64 now;
    ktime_get_real_ts64(&now);

    // Конвертируем время погружения Титаника в timestamp
    time64_t titanic_timestamp = mktime64(titanic_time.tm_year + 1900,
                                          titanic_time.tm_mon + 1,
                                          titanic_time.tm_mday,
                                          titanic_time.tm_hour,
                                          titanic_time.tm_min,
                                          titanic_time.tm_sec);

    // Вычисляем разницу в секундах
    time64_t diff_seconds = now.tv_sec - titanic_timestamp;

    // Переводим разницу в минуты
    return diff_seconds / 60;
}

// Функция, вызываемая при чтении файла
static ssize_t procfile_read(struct file *file_pointer, char __user *buffer,
                             size_t buffer_length, loff_t *offset) {
    unsigned long minutes = calculate_minutes_since_titanic();
    char s[128];
    int len;

    len = snprintf(s, sizeof(s), "С момента полного погружения Титаника прошло %lu минут.\n", minutes);

    if (*offset > 0) {
        return 0;
    }
    if (copy_to_user(buffer, s, len)) {
        return -EFAULT;
    }
    *offset = len;

    pr_info("procfile read %s\n", file_pointer->f_path.dentry->d_name.name);
    return len;
}

// Указатель на структуру операций с файлом
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

// Функция, вызываемая при загрузке модуля
static int __init procfs1_init(void) {
    our_proc_file = proc_create(PROC_FILE_NAME, 0644, NULL, &proc_file_fops);
    if (our_proc_file == NULL) {
        pr_err("Не удалось создать /proc/%s\n", PROC_FILE_NAME);
        return -ENOMEM;
    }
    pr_info("/proc/%s создан\n", PROC_FILE_NAME);
    return 0;
}

// Функция, вызываемая при выгрузке модуля
static void __exit procfs1_exit(void) {
    proc_remove(our_proc_file);
    pr_info("/proc/%s удален\n", PROC_FILE_NAME);
}

module_init(procfs1_init);
module_exit(procfs1_exit);