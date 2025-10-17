// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

// Define the ioctl and struct as per the header

struct vmsg_lb_dev_info {
	uint32_t dev_id;
};

#define IOCTL_VMSG_LB_ADD \
	_IOC(_IOC_NONE, 'P', 0, sizeof(struct vmsg_lb_dev_info))

int main(int argc, char *argv[])
{
	uint32_t dev_id = 0;

	int fd = open("/dev/virtio-msg-lb", O_RDWR);
	if (fd < 0) {
		perror("Failed to open /dev/virtio-msg-lb");
		return EXIT_FAILURE;
	}

	struct vmsg_lb_dev_info info = {
		.dev_id = dev_id,
	};

	if (ioctl(fd, IOCTL_VMSG_LB_ADD, &info) < 0) {
		perror("IOCTL_VMSG_LB_ADD failed");
		close(fd);
		return EXIT_FAILURE;
	}

	printf("Device with ID %u added successfully.\n", dev_id);

	close(fd);
	return EXIT_SUCCESS;
}
