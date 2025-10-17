#define _GNU_SOURCE
#include <linux/dma-heap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <linux/vm_sockets.h>

#define SCM_VSOCK_SHMEM 1
#define SOL_VSOCK 287

struct vsock_shmem_desc {
	__u64 token; /* app correlation token */
	__u64 size; /* size in bytes */
	__u32 flags; /* VSOCK_SHMEM_F_* */
	__u32 region_id; /* region identifier (virtio-shmem/ivshmem/etc) */
	__u32 subop; /* VSOCK_SHMEM_SUBOP_* hint */
	__s32 fd;
};

static int create_shm_fd(size_t size)
{
	int heap_fd = open("/dev/dma_heap/system", O_RDWR);
	if (heap_fd < 0) {
		perror("open /dev/dma_heap/system");
		return -1;
	}

	struct dma_heap_allocation_data alloc = {
		.len = size,
		.fd_flags = O_RDWR | O_CLOEXEC,
		.heap_flags = 0,
	};

	if (ioctl(heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc) < 0) {
		perror("DMA_HEAP_IOCTL_ALLOC");
		close(heap_fd);
		return -1;
	}

	close(heap_fd);
	return alloc.fd;
}

// Send a file descriptor over a Unix socket
static int send_fd(int sock, int fd)
{
	struct vsock_shmem_desc desc = {
		.token = 0xdeadbeef,
		.size = 1<<20,
		.flags = 0,
		.region_id = 7,
		.subop = 0,
		.fd = fd,
	};
	char control[CMSG_SPACE(sizeof(desc))];
	memset(control, 0, sizeof(control));
	char dummy = 'X';
	struct iovec iov = { .iov_base = &dummy, .iov_len = sizeof(dummy) };
	struct msghdr msg = {
		.msg_iov = &iov, .msg_iovlen = 1,
		.msg_control = control, .msg_controllen = sizeof(control)
	};
	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

	cmsg->cmsg_level = SOL_VSOCK;
	cmsg->cmsg_type = SCM_VSOCK_SHMEM;
	cmsg->cmsg_len = CMSG_LEN(sizeof(desc));
	memcpy(CMSG_DATA(cmsg), &desc, sizeof(desc));

	if (sendmsg(sock, &msg, 0) < 0) {
		perror("sendmsg");
		return -1;
	}
	return 0;
}

// Receive a file descriptor over a Unix socket
static int recv_fd(int sock)
{
	char dummy;
	struct iovec iov = { .iov_base = &dummy, .iov_len = sizeof(dummy) };
	char cbuf[CMSG_SPACE(sizeof(struct vsock_shmem_desc))];
	int received_fd = -1;
	struct msghdr msg;
	ssize_t n;

	memset(&msg, 0, sizeof(msg));
	memset(cbuf, 0, sizeof(cbuf));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cbuf;
	msg.msg_controllen = sizeof(cbuf);

	n = recvmsg(sock, &msg, 0);
	if (n < 0) {
		perror("recvmsg");
		return -1;
	}

	for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
			cmsg != NULL;
			cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_VSOCK &&
				cmsg->cmsg_type == SCM_VSOCK_SHMEM) {
			if (cmsg->cmsg_len < CMSG_LEN(sizeof(struct vsock_shmem_desc))) {
				fprintf(stderr, "cmsg too short: %zu < %zu\n",
						(size_t)cmsg->cmsg_len, (size_t)CMSG_LEN(sizeof(struct vsock_shmem_desc)));
				continue;
			}
			struct vsock_shmem_desc *d =
				(struct vsock_shmem_desc *)CMSG_DATA(cmsg);
			received_fd = d->fd;
			break;
		}
	}

	return received_fd;

}

// Demo: write to shared memory and send FD
static void demo_shm_sender(int sock)
{
	int fd = create_shm_fd(4096);
	if (fd < 0) return;

	char *mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return;
	}
	strcpy(mem, "Hello from shared memory via FD!\n");

	if (send_fd(sock, fd) == 0) {
		printf("Sender: sent shared memory FD\n");
	}
	munmap(mem, 4096);
	close(fd);
}

// Demo: receive FD and read shared memory
static void demo_shm_receiver(int sock)
{
	int fd = recv_fd(sock);
	printf("Received fd: %d\n", fd);
	if (fd < 0)
		return;

	char *mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return;
	}
	printf("Receiver: got shared memory content: %s\n", mem);
	munmap(mem, 4096);
	close(fd);
}

static void demo_msg_server(int sock)
{
	char buf[1024];
	ssize_t n = read(sock, buf, sizeof(buf)-1);
	buf[n > 0 ? n : 0] = 0;
	printf("Server received: %s\n", buf);

	write(sock, "Hello from server", 18);
}

static void demo_msg_client(int sock)
{
	write(sock, "Hello from client", 18);
	char buf[1024];
	ssize_t n = read(sock, buf, sizeof(buf)-1);
	buf[n > 0 ? n : 0] = 0;
	printf("Client received: %s\n", buf);
}

static void die(const char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	if (argc < 5) {
		fprintf(stderr,
			"Usage:\n"
			"  %s <mode: server|client> <type: tcp|unix|vsock> <address> <port/path> <msg|shm>\n"
			"\nExamples:\n"
			"  %s server tcp 0.0.0.0 5000 msg\n"
			"  %s client tcp 127.0.0.1 5000 msg\n"
			"  %s server unix /tmp/test.sock 0 msg\n"
			"  %s client unix /tmp/test.sock 0 msg\n"
			"  %s server vsock 3 1234    # listen on CID 3, port 1234 msg\n"
			"  %s client vsock 2 1234    # connect to CID 2, port 1234 msg\n",
			argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
		return 1;
	}

	int server_mode = strcmp(argv[1], "server") == 0;
	int domain, sockfd;
	const char *type_str = argv[2];
	const char *addr_str = argv[3];
	const char *port_str = argv[4];
	int send_msg = strcmp(argv[5], "msg") == 0;

	if (strcmp(type_str, "tcp") == 0) {
		domain = AF_INET;
	} else if (strcmp(type_str, "unix") == 0) {
		domain = AF_UNIX;
	} else if (strcmp(type_str, "vsock") == 0) {
		domain = AF_VSOCK;
	} else {
		fprintf(stderr, "Unknown socket type: %s\n", type_str);
		return 1;
	}

	sockfd = socket(domain, SOCK_STREAM, 0);
	if (sockfd < 0) die("socket");

	if (server_mode) {
		if (domain == AF_INET) {
			struct sockaddr_in addr = {0};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(atoi(port_str));
			addr.sin_addr.s_addr = inet_addr(addr_str);
			if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("bind");
		} else if (domain == AF_UNIX) {
			struct sockaddr_un addr = {0};
			addr.sun_family = AF_UNIX;
			strncpy(addr.sun_path, addr_str, sizeof(addr.sun_path) - 1);
			unlink(addr.sun_path); // remove existing
			if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("bind");
		} else if (domain == AF_VSOCK) {
			struct sockaddr_vm addr = {0};
			addr.svm_family = AF_VSOCK;
			addr.svm_cid = atoi(addr_str);
			addr.svm_port = atoi(port_str);
			if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("bind");
		}

		if (listen(sockfd, 1) < 0) die("listen");

		printf("Server listening...\n");
		int client_fd = accept(sockfd, NULL, NULL);
		if (client_fd < 0) die("accept");

		if (send_msg)
			demo_msg_server(client_fd);
		else
			demo_shm_receiver(client_fd);

		close(client_fd);
	} else {
		if (domain == AF_INET) {
			struct sockaddr_in addr = {0};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(atoi(port_str));
			if (inet_pton(AF_INET, addr_str, &addr.sin_addr) <= 0) die("inet_pton");
			if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("connect");
		} else if (domain == AF_UNIX) {
			struct sockaddr_un addr = {0};
			addr.sun_family = AF_UNIX;
			strncpy(addr.sun_path, addr_str, sizeof(addr.sun_path) - 1);
			if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("connect");
		} else if (domain == AF_VSOCK) {
			struct sockaddr_vm addr = {0};
			addr.svm_family = AF_VSOCK;
			addr.svm_cid = atoi(addr_str);
			addr.svm_port = atoi(port_str);
			if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("connect");
		}

		if (send_msg)
			demo_msg_client(sockfd);
		else
			demo_shm_sender(sockfd);
	}

	close(sockfd);
	return 0;
}
