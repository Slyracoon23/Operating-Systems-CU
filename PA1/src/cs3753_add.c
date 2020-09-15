#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/uaccess.h>


asmlinkage long sys_cs3753_add(int a, int b, int* ret){

    printk(KERN_ALERT "ADDING %d AND %d\n", a,b);


    int result = a + b;

    copy_to_user(ret, &result, sizeof(int);

    printk(KERN_ALERT "stored result is: %d\n", result);


    return 0;

}
