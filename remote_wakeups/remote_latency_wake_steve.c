/*
 * Remote Wakeup Cpufreq Latency Testcase
 *
 * A thread which was formerly low demand wakes up and starts running nonstop on
 * CPU0. This task is known in this testcase as the "framework thread."
 *
 * Immediately after that, a task on CPU1 causes a new task to be created on
 * CPU0. This new task also wants to run for a long time.
 *
 * Because the new task was created remotely a scheduler callback not
 * immediately invoked. It may be a full tick before the scheduler callback
 * fires.
 *
 * Affinity is used in this test case to generate the conditions necessary to
 * expose potentially high latency for a schedutil response. These conditions
 * should be observable without affinity however.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

int do_some_work(void)
{
	int scratch;
	int i = 0;

	while (i < 10000)
		scratch = ((scratch + 34) *3) % 7 + i++;
	return scratch;
}

void do_busy(unsigned int usec)
{
	long long usec_passed;
	struct timeval start, tv;

	gettimeofday(&start, NULL);

	while (1) {
		gettimeofday(&tv, NULL);
		usec_passed = tv.tv_sec - start.tv_sec;
		usec_passed *= 1000000;
		usec_passed += tv.tv_usec - start.tv_usec;
		if (usec_passed >= usec)
			return;
		else
			do_some_work();
	}
}

/*
 * The framework thread is the thread that will have a low demand but start
 * running nonstop on CPU0 just before the woken_thread is woken on CPU0.
 */
pthread_t framework_thread;
void *framework_fn(void *arg)
{
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask)) {
		printf("Could not set main thread affinity!\n");
		return NULL;
	}

	usleep(398000);

	do_busy(200000);

	return NULL;
}

/*
 * This thread gets woken up on CPU0. It is a new thread so it should have high
 * demand.
 */
pthread_t woken_thread;
void *woken_fn(void *arg)
{
	/* Busy work! */
	do_busy(200000);

	return NULL;
}

int main(int argc, char **argv)
{
	cpu_set_t mask;
	pthread_attr_t attrs;

	/* The main task runs on CPU 1. */
	CPU_ZERO(&mask);
	CPU_SET(1, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask)) {
		printf("Could not set main thread affinity!\n");
		return -1;
	}

	/* The "framework" and new tasks run on CPU0. */
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	pthread_attr_init(&attrs);
	pthread_attr_setaffinity_np(&attrs, sizeof(mask), &mask);
	if (pthread_create(&framework_thread, &attrs, framework_fn,
			   NULL)) {
		printf("Error creating framework thread!\n");
		return -1;
	}

	/* Sleep just a little longer than the framework thread. */
	usleep(400000);

	/* Wake up the new task. */
	if (pthread_create(&woken_thread, &attrs, woken_fn,
			   NULL)) {
		printf("Error creating framework thread!\n");
		return -1;
	}

	pthread_join(woken_thread, NULL);
	pthread_join(framework_thread, NULL);

	printf("Current process: %u, framework: %u, woken: %u\n", getpid(),
		framework_thread, woken_thread);

	return 0;
}
