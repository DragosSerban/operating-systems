#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include "util/so_scheduler.h"

// structura pt o celula din lista
typedef struct cell {
	void *info;
	struct cell *next;
} TCell, *TList;

// structura pt fiecare task ce va rula pe un anumit thread
typedef struct {
	tid_t id;
	unsigned int priority;
	unsigned int remaining_time;
	so_handler *func;
	sem_t semaphore;
} so_task_t;

// structura pt scheduler
typedef struct {
	unsigned int time_quantum;
	unsigned int io;
	so_task_t *running_thread;
	TList *ready_threads;
	TList thread_list;
} so_scheduler;

static int scheduler_exists;
static so_scheduler *s;

// functie pt alocare de celula
static TList alloc_cell(void *info)
{
	TList cell = (TList)malloc(sizeof(TCell));

	if (!cell)
		return NULL;
	cell->info = info;
	cell->next = NULL;
	return cell;
}

// functie pt inserare in vectorul de liste al threadului nou creat
// vom folosi cate o lista pt fiecare prioritate => 6 liste
// => vector cu 6 elemente de tip pointer
static void insert_ready(so_task_t *thread)
{
	TList *priority_arr = s->ready_threads;
	unsigned int priority = thread->priority;
	TList new_cell = alloc_cell((void *)thread);

	if (!new_cell)
		return;
	if (!priority_arr[thread->priority]) {
		priority_arr[thread->priority] = new_cell;
	} else {
		TList aux = priority_arr[thread->priority];
		TList ant = NULL;

		while (aux) {
			ant = aux;
			aux = aux->next;
		}
		ant->next = new_cell;
	}
}

static void free_ready_threads(void)
{
	for (int i = 0; i < SO_MAX_PRIO + 1; i++) {
		TList aux = s->ready_threads[i];
		TList ant = NULL;

		while (aux) {
			ant = aux;
			aux = aux->next;
			free(ant->info);
			free(ant);
		}
	}
}

// functie pt obtinerea threadului din vectorul de liste
static so_task_t *get_thread(void)
{
	so_task_t *ret;
	unsigned int priority = SO_MAX_PRIO;
	TList aux = s->ready_threads[priority];

	while (!aux && priority > 0) {
		priority--;
		aux = s->ready_threads[priority];
	}
	if (!aux)
		return NULL;
	ret = (so_task_t *)aux->info;
	s->ready_threads[priority] = aux->next;
	free(aux);
	return ret;
}

// functie ce va retine thread id-ul intr-o lista (pt dezalocare)
static void remind_thread_id(tid_t id)
{
	tid_t *temp_id = (tid_t *)malloc(sizeof(tid_t));

	if (!temp_id)
		return;
	*temp_id = id;
	TList new_cell = alloc_cell(temp_id);

	if (!new_cell)
		return;
	if (!s->thread_list) {
		s->thread_list = new_cell;
	} else {
		TList aux = s->thread_list;
		TList ant = NULL;

		while (aux) {
			ant = aux;
			aux = aux->next;
		}
		ant->next = new_cell;
	}
}

// functie pe care o va rula threadul
// (o va primi ca argument la creare)
static void start_thread(so_task_t *t)
{
	sem_wait(&t->semaphore);
	// se va rula functia dorita pe thread
	t->func(t->priority);
	sem_post(&t->semaphore);

	// dupa finalizare, se elibereaza memoria threadului curent
	// si se ruleaza urmatorul thread, daca exista
	free(s->running_thread);
	s->running_thread = NULL;
	s->running_thread = get_thread();
	if (!s->running_thread)
		return;
	sem_post(&s->running_thread->semaphore);
}

// initializeaza schedulerul
int so_init(unsigned int time_quantum, unsigned int io)
{
	// verifica daca exista deja / nu are parametrii corecti
	if (scheduler_exists == 1)
		return -1;
	if (time_quantum == 0 || io > SO_MAX_NUM_EVENTS)
		return -1;

	// alocam schedulerul
	s = (so_scheduler *)malloc(sizeof(so_scheduler));
	if (!s)
		return -1;
	scheduler_exists = 1;

	// completam / alocam campurile din strucutura schedulerului
	s->time_quantum = time_quantum;
	s->io = io;
	s->thread_list = NULL;
	s->running_thread = NULL;
	s->ready_threads = (TList *)calloc(SO_MAX_PRIO + 1, sizeof(TList));
	if (!s->ready_threads) {
		free(s);
		return -1;
	}

	return 0;
}

// functie care creeaza un nou thread
tid_t so_fork(so_handler *func, unsigned int priority)
{
	// verifica prioritatea / handlerul la functie
	if (!func || priority > SO_MAX_PRIO) {
		so_end();
		return INVALID_TID;
	}
	// se aloca un nou task
	so_task_t *t = (so_task_t *)malloc(sizeof(so_task_t));

	if (!t) {
		so_end();
		return INVALID_TID;
	}
	// se completeaza campurile taskului
	t->priority = priority;
	t->func = func;
	t->remaining_time = s->time_quantum;
	// initializarea semaforului cu zero; threadul nu va rula
	// cand dam pthread_create
	sem_init(&t->semaphore, 0, 0);
	// se creeaza un nou thread
	if (pthread_create(&(t->id), NULL,
	(void *)start_thread, (void *)t) != 0) {
		so_end();
		return INVALID_TID;
	}

	// se insereaza in vectorul de liste
	insert_ready(t);
	// se retine id-ul threadului curent intr-o lista
	// pt a putea da pthread_join la final
	// (altfel threadul poate fi dezalocat in start_thread
	// si se pierde id-ul)
	remind_thread_id(t->id);

	// scadem cuanta de timp a threadului curent; daca exista
	if (s->running_thread)
		s->running_thread->remaining_time -= 1;

	if (!s->running_thread) {
		// daca nu ruleaza niciun thread creat de noi, atunci se
		// extrage din vectorul de liste si se ruleaza
		s->running_thread = get_thread();
		sem_post(&s->running_thread->semaphore);
	} else if (s->running_thread->priority < t->priority) {
		// daca fork a creat un nou thread cu o prioritate mai mare
		// decat cel curent => threadul curent se readauga in
		// ready_threads si se extrage unul nou
		// (in acest caz cel nou creat)
		s->running_thread->remaining_time = s->time_quantum;
		insert_ready(s->running_thread);
		so_task_t *previous_task = s->running_thread;

		s->running_thread = get_thread();
		// se porneste rularea noului thread
		sem_post(&s->running_thread->semaphore);
		// se opreste rularea vechiului thread
		sem_wait(&previous_task->semaphore);
	} else if (s->running_thread->remaining_time <= 0) {
		// daca s-a terminat cuanta de timp
		// => threadul curent se readauga in ready_threads
		// si se extrage unul nou
		s->running_thread->remaining_time = s->time_quantum;
		insert_ready(s->running_thread);
		so_task_t *previous_task = s->running_thread;

		s->running_thread = get_thread();
		// se porneste rularea noului thread
		sem_post(&s->running_thread->semaphore);
		// se opreste rularea vechiului thread
		sem_wait(&previous_task->semaphore);
	}

	// returnam id-ul threadului
	return t->id;
}

int so_wait(unsigned int io)
{

}

int so_signal(unsigned int io)
{

}

void so_exec(void)
{
	// scadem cuanta de timp a threadului curent
	s->running_thread->remaining_time -= 1;
	if (s->running_thread->remaining_time <= 0) {
		// daca a expirat cuanta de timp, atunci threadul
		// va fi readaugat in ready_threads si vom extrage
		// un nou thread din vectorul de liste
		s->running_thread->remaining_time = s->time_quantum;
		insert_ready(s->running_thread);
		so_task_t *previous_task = s->running_thread;

		s->running_thread = get_thread();
		// pornim executia noului thread
		sem_post(&s->running_thread->semaphore);
		// oprim executia vechiului thread
		sem_wait(&previous_task->semaphore);
	}
	return;
}

void so_end(void)
{
	if (scheduler_exists) {
		TList aux0 = s->thread_list;

		while (aux0) {
			// se asteapta finalizarea tuturor threadurilor
			pthread_join(*(tid_t *)(aux0->info), NULL);
			// se elibereaza spatiul ocupat de lista
			TList temp = aux0;

			free(temp->info);
			aux0 = temp->next;
			free(temp);
		}
		free(s->running_thread);
		// se elibereaza spatiul ocupat de vectorul de threaduri
		// ready_threads
		free_ready_threads();
		free(s->ready_threads);
		free(s);
		scheduler_exists = 0;
	}
	return;
}

