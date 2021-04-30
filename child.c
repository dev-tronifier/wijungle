#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/shm.h>
#include <sys/ipc.h>

/**
 * ch_data - Change the data of the shared memory pointed by variable pt.
 *
 * @n: Thread number. (0 - Parent, 1 - Child)
 * @pt: Pointer to the shared memory address.
 */
void ch_data(int n, int *pt)
{
	*pt += 1;
	printf("[%d]PID : %d\t%d\n", n, getpid(), *pt);
}

int main()
{
	int *a;
	int id, k, p;

	assert((id = shmget(IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT)) >= 0);
	assert((p = fork()) >= 0);

	switch(p) {
	case 0:
		a = (int *) shmat(id, 0, 0);
		ch_data(0, a);
		shmdt(a);
		shmctl(id, IPC_RMID, 0);
		break;
	default:
		assert((k = fork()) >= 0);

		a = (int *) shmat(id, 0, 0);
		ch_data(1, a);
		shmdt(a);
	}

	return 0;
}

