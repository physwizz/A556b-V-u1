#include <kunit/test.h>

#include <pcie_scsc/scsc_log_collector.h>

extern int (*fp_scsc_log_collector_mmap_open)(struct inode *inode, struct file *filp);
extern int (*fp_scsc_log_collector_release)(struct inode *inode, struct file *filp);
extern int (*fp_scsc_log_collector_mmap)(struct file *filp, struct vm_area_struct *vma);

static void test_all(struct kunit *test)
{
	struct vm_area_struct vma;

	fp_scsc_log_collector_mmap_open(NULL, NULL);

	fp_scsc_log_collector_release(NULL, NULL);

	vma.vm_start = 1;
	vma.vm_end = vma.vm_start + SCSC_LOG_COLLECT_MAX_SIZE;
	vma.vm_pgoff = 0;

	fp_scsc_log_collector_mmap(NULL, &vma);

	KUNIT_EXPECT_STREQ(test, "OK", "OK");
}

static int test_init(struct kunit *test)
{
	return 0;
}

static void test_exit(struct kunit *test)
{
}

static struct kunit_case test_cases[] = {
	KUNIT_CASE(test_all),
	{}
};

static struct kunit_suite test_suite[] = {
	{
		.name = "test_scsc_log_collector_mmap",
		.test_cases = test_cases,
		.init = test_init,
		.exit = test_exit,
	}
};

kunit_test_suites(test_suite);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yongjin.lim@samsung.com>");

