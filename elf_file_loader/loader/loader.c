/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "exec_parser.h"

#define SIZEOF_PAGE 4096

static so_exec_t *exec;
int fd;

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	for (int i = 0; i < exec->segments_no; i++) {
		so_seg_t crt_seg = exec->segments[i];

		// verificam daca adresa unde s-a produs page fault
		// se regaseste in segmentul curent
		unsigned int fault_addr = (unsigned int)info->si_addr;
		if (crt_seg.vaddr <= fault_addr &&
		fault_addr < crt_seg.vaddr + crt_seg.mem_size) {
			// daca nu am mai trecut prin segmentul curent =>
			// initializam un vector in zona data a segmentului
			// actual, unde vom pastra daca am mai mapat (1)
			// sau nu (0) pagina respectiva din segmentul curent
			if (!(char*)exec->segments[i].data) {
				unsigned int length = ((crt_seg.mem_size /
				SIZEOF_PAGE) / SIZEOF_PAGE + 1) * SIZEOF_PAGE;

				exec->segments[i].data = mmap(NULL, length,
				PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

				if ((int)(exec->segments[i].data) == -1)
					exit(1);
			}

			// a fost updatat pointerul data
			crt_seg = exec->segments[i];
			int page_no = ((unsigned int)info->si_addr -
			crt_seg.vaddr) / SIZEOF_PAGE;

			// verificam daca am mai mapat zona de memorie actuala
			// daca da => seg fault
			if (((char*)(crt_seg.data))[page_no]) {
				sigaction(signum,
				(struct sigaction*)info, context);
				return;
			}

			((char*)(crt_seg.data))[page_no] = 1;
			
			// aflam in ce zona din memoria virtuala se afla pagina
			int copy_from_file = 0;
			if (crt_seg.file_size <= page_no*SIZEOF_PAGE) {
				// cazul 1: pagina se afla dupa ce se termina
				// file_size => zeroizam pagina
				int *temp = mmap((void*)(crt_seg.vaddr+
				page_no*SIZEOF_PAGE), SIZEOF_PAGE, crt_seg.perm,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

				if ((int)temp == -1)
					exit(1);
			} else {
				// cazul 2: pagina se afla inainte de a se
				// termina file_size => copiem din fisierul
				// executabil pagina corespunzatoare
				int *temp = mmap((void*)(crt_seg.vaddr+
				page_no*SIZEOF_PAGE), SIZEOF_PAGE,
				PROT_READ | PROT_WRITE, MAP_PRIVATE, fd,
				crt_seg.offset+page_no*SIZEOF_PAGE);

				if((int)temp == -1)
					exit(1);
				copy_from_file = 1;
			}

			// daca finalul lui file_size se afla fix in pagina
			// mapata anterior => completam cu zerouri de la finalul
			// lui file_size pana la finalul paginii
			if(copy_from_file &&
			crt_seg.file_size < (page_no+1)*SIZEOF_PAGE) {
				unsigned int mem_size = crt_seg.mem_size;
				unsigned int file_size = crt_seg.file_size;
				int temp = (page_no+1)*SIZEOF_PAGE-file_size;
				for(int i = 0; i < temp; i++)
					if(file_size+i < mem_size)
						*(char*)(crt_seg.vaddr+
						file_size+i) = 0;
			}

			if(copy_from_file) {
				// adaugam permisiunile corespunzatoare
				// segmenului pe zona mapata anterior
				if(mprotect((void*)(crt_seg.vaddr+
				page_no*SIZEOF_PAGE), SIZEOF_PAGE,
				crt_seg.perm) == -1)
					exit(1);
			}
			return;
		}
	}
	sigaction(signum, (struct sigaction*)info, context);
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	// deschidem fisierul executabil
	fd = open(path, O_RDONLY);
	if(fd == -1)
		return -1;
	so_start_exec(exec, argv);

	return -1;
}

