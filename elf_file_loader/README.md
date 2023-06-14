- Atunci cand se produce un page fault, se va trece prin toate segmentele
structurii corespunzatoare executabilului pt a gasi segmentul in care s-a
petrecut page fault-ul; daca locul unde s-a produs page fault-ul nu se
regaseste in niciun segment => se apeleaza handlerul default => seg fault.
- Daca adresa unde s-a produs page fault se regaseste in vreun segment,
mai intai vom verifica daca zona data a structurii segment a fost alocata
(vom folosi zona data pt a stoca cate un vector binar in fiecare segment
pt a tine cont ce pagini din segmentul curent au fost mapate);
daca nu, atunci vom aloca vectorul (1 = mapped; 0 = not mapped yet);
vectorul este de lungime exec->segments[i].mem_size / SIZEOF_PAGE -
zona alocata este de fapt calculata astfel incat sa fie multiplu de SIZEOF_PAGE,
altfel nu va reusi mmap (mmap trebuie sa primeasca multiplu de SIZEOF_PAGE
ca length) - MAP_ANONYMOUS initializeaza elementele vectorului cu zero
- calculam nr paginii in cadrul segmentului
- daca am mai mapat pagina odata => handler default => seg fault
- altfel, incepem procesul de mapare a paginii si scriem 1 la pozitia
corespunzatoare paginii in vectorul descris mai sus.
- verificam acum in ce zona din memoria virtuala se afla pagina
(in zona ce trebuie zeroizata (cazul 1), in zona ce corespunde complet cu
o pagina din fisierul executabil, sau pagina contine atat o parte zeroizata,
cat si o parte din fisierul executabil (cazul 2).
- in primul caz, zeroizam pagina din segmentul virtual, se seteaza permisiunile
- in al doilea caz, folosim mmap pt a copia la adresa corespunzatoare din
memoria virtuala datele aflate in pagina corespunzatoare din fisierul executabil
- daca finalul lui file_size se afla fix in pagina mapata anterior
(desenul din enunt) => completam cu zerouri de la finalul lui file_size pana la
sfarsitul paginii
- la final adaugam permisiunile pt cazul 2.
