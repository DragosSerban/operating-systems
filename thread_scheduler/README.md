- vom folosi 2 structuri: una pt scheduler (so_scheduler) si una pt threaduri (so_task_t)
- so_scheduler (contine cuanta de timp si campul io, un pointer la threadul curent,
o lista cu toate id-urile threadurilor (pt pthread_join) si un vector de prioritati -
fiecare element din vector este o lista
- so_task_t contine thread id-ul, prioritatea, timpul ramas al threadului, functia pe care
trebuie sa o ruleze si un semafor care porneste si opreste executia threadului
- vom construi liste simplu inlantuite, asa ca vom avea nevoie de o structura si de o functie
alloc_cell
- insert_ready este o functie pt inserare in vectorul de liste al threadului nou creat;
vom folosi cate o lista pt fiecare prioritate => 6 liste => vector cu 6 elemente de tip pointer
- get_thread este o functie care cauta prin vectorul de liste primul thread cu prioritatea
cea mai mare si il returneaza
- remind_thread_id este o functie ce salveaza in lista thread_list din scheduler toate id-urile
threadurilor create de noi
- functia start_thread reprezinta contextul in care va rula un thread (so_wait va opri celelalte
threaduri, asadar se va rula functia t->func; dupa ce aceasta termina rularea, se incrementeaza
variabila semaforului; apoi; se elibereaza memoria threadului curent, si se pune in executie
urmatorul thread din vectorul de prioritati, daca acesta exista
- so_init este functia ce initializeaza schedulerul; ea verifica mai intai ca acesta sa nu fi
fost deja initializat si verifica ca parametrii pe care i-a primit sa fie corecti;
se aloca schedulerul si sunt completate campurile sale; este alocat vectorul de liste
- functia so_fork creeaza un nou thread; ea verifica mai intai ca parametrii sa fie valizi;
se aloca un nou element de tipul so_task_t, se completeaza campurile cu prioritatea, functia
si cuanta de timp; se initializeaza semaforul cu zero; threadul nu va porni executia inca;
se creeaza threadul ce va rula peste functia thread_start si i se asigneaza un id; threadul
va fi inserat in ready_threads; iar id-ul va fi copiat in lista thread_list;
daca un thread deja ruleaza, i se va scadea cuanta de timp; altfel, daca nu ruleaza niciun thread
creat de noi, atunci se extrage primul thread cu prioritatea cea mai mare din ready_threads si
incepe rularea, prin apelarea functiei sem_post, care incrementeaza variabila semaforului
daca fork a creat un nou thread cu o prioritate mai mare decat cel curent => threadul curent
se readauga in ready_threads si se extrage unul nou (in acest caz cel nou creat); se porneste
rularea noului thread si fix urmatoarea instructiune opreste rularea threadului precedent, al
carui adresa a fost salvata intr-un pointer (previous_task);
daca s-a terminat cuanta de timp => threadul curent se readauga in ready_threads si se extrage
un nou thread cu prioritatea cea mai mare; de asemenea, se porneste rularea noului thread si se
opreste rularea vechiului thread, prin operatii cu semafoare;
la final, daca totul a mers perfect, se returneaza id-ul threadului creat cu functia so_fork
- functiile so_wait si so_signal nu au fost implementate
- functia so_exec va scadea cuanta de timp a threadului curent; se verifica daca a expirat
cuanta de timp, atunci threadul va fi readaugat in ready_threads si vom extrage un nou thread
din vectorul de liste; pornim executia noului thread; o oprim pe cea a vechiului thread
- functia so_end asteapta ca toate threadurile create de noi sa isi finalizeze executia;
se foloseste de lista thread_list pt a apela pthread_join pe fiecare thread in parte;
apoi se elibereaza spatiul ocupat de lista, se elibereaza spatiul ocupat de
vectorul de threaduri ready_threads si cel ocupat de structura scheduler
