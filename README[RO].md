
## Tema 2 - Protocoale de comunicatii
#### Toma Stefan-Madalin 323CC

Protocolul de nivel aplicatie implementat implica folosirea unei structuri
"Packet" ce contine un header sub forma primelor 3 campuri si o valoare de tip
"Message" (pentru o vizualizare mai buna a acestora se gasesc mai jos cele doua
datagrame aferente lor). Transmiterea de pachete are loc intr-un mod eficient
datorita receptionarii lungimii mesajului prima data apoi citirea de pe socket
a length bytes. Structura "Message" poate contine atat datele UDP nealterate,
dar poate fi folosita si pentru transmiterea de comenzi de la clientii TCP catre
server si vice-versa. Pentru trimiterea de astfel de comenzi se folosesc doar
primele doua campuri ale structurii "Message" trimitand cu 1500 de bytes mai
putin decat daca refoloseam intreaga structura. Pentru comenzi de la clientii
TCP se va omite popularea field-ului legat de sursa src.

Datagrame:

```c
Packet:

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*---------------------------------------------------------------*
|              src              | length|         id        |   |
*-----------------------------------------------------------*   *
|                                                               |
...                                                           ...
...                        message                            ...
...                                                           ...
|                                                               |
*                             *---------------------------------*
|                             |
*-----------------------------*

Message:

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------------------------------------------------------+
|                             topic                             |
+                                   +---------------------------+
|                                   |   tip_pachet  |           |
+---------------------------------------------------+           +
...                                                           ...
...                           value (dinamic)                 ...
...                                                           ...
+                                           +-------------------+
|                                           |
+-------------------------------------------+
```



Tipul pachetelor pe care server-ul le trimite iar clientii TCP le primesc:

|   Tip mesaj      | Cod  |             Continut                              |
|------------------|------|-------------------------------------------------- |
| INT              |   0  | Date legate de client UDP + valoarea intreaga     |
| SHORT_REAL       |   1  | Date legate de client UDP + valoarea reala scurta |
| FLOAT            |   2  | Date legate de client UDP + valoarea reala        |
| STRING           |   3  | Date legate de client UDP + string                |
| SUBSCRIBEREQ     |   4  | O cerere de subscribe cu sf = 1, topic si id      |
| SUBSCRIBEREQNOSF |   5  | O cerere de subscribe cu sf = 0, topic si id      |
| UNSUBSCRIBEREQ   |   6  | O cerere de unsubscribe cu id precizat            |
| EXIT             |  10  | Mesaj de iesire in cazul in care server-ul        |
|                  |      | se inchide sau conexiunea cu acesta nu se poate   |
|                  |      | realiza                                           |
| OK               | 100  | Mesaj cu rolul de a asigura clientul ca s-a       |
|                  |      | realizat cu succes conexiunea intre acesta        |
|                  |      | si server                                         |

### Modul de functionare:

### - subscriber

Pentru multiplexarea I/O se creeaza un socket TCP care se adauga impreuna cu stdin
intr-o multime de file descriptori. Inainte insa se incearca conectarea la server.
In cazul in care se reuseste se primeste un mesaj de tipul OK altfel EXIT si
programul se opreste.

Intr-o bucla while se executa la fiecare iteratie urmatoarele operatii:
- se selecteaza socketii pe care s-a trimis ceva
- daca s-a trimis ceva pe stdin atunci se verifica tipul comenzii:
   ├── subscribe - se parseaza si verifica comanda apoi se trimite
   │   un mesaj de tipul SUBSCRIBEREQ sau SUBSCRIBEREQNOSF in functie
   │   de comanda
   ├── unsubscribe - se parseaza si verifica comanda
   │   si se trimite un mesaj UNSUBSCRIBEREQ
   └── exit - se trimite un mesaj de tipul EXIT catre server si
       se iese din while
- daca se primeste un mesaj pe socket-ul TCP se va iesi din bucla
  daca acesta are tipul EXIT sau se va afisa pentru orice alt tip

La iesirea din loop se inchide socket-ul TCP.



### - server

Se creeaza doi socketi pe care sa se poata asculta: unul TCP si unul UDP.
Cei doi impreuna cu stdin-ul sunt adaugati intr-o multime de file descriptori.
Initial i se cere socket-ului TCP sa se astepte la o singura conexiune.

Variabile importante:

```c++
unordered_map<string, unordered_map<string, int>> topicClients;
            // cheia primei tabele este topic-ul apoi pentru fiecare topic
            // se gasesc tabele cu entries de felul (id client, sf)
unordered_map<string, client> clients; // tabela de clienti cu id drept cheie
```

Intr-o bucla infinita se executa urmatoarele operatii:
- se aleg socketii din set-ul curent pe care s-a primit un pachet
- daca s-a primit input pe stdin se verifica daca este vorba de comanda "exit".
  In caz afirmativ se elibereaza toti socketii pe care server-ul are conexiune
  cu clientii TCP + cei doi socketi pe care se asculta.
- intr-un for se parcurg toti socketii pe care s-a mai primit vreun pachet
  si in functie de socket si starea actuala a clientilor in baza de date interna
  a serverului se vor executa urmatoarele instructiuni
    ├── socket-ul de ascultare TCP - se primeste o cerere de conexiune cu server-ul
    │   si deci trebuie sa se retina intern noul client si socket-ul asociat lui.
    │   Daca acesta exista deja dar era offline doar se actualizeaza socket-ul
    │   aferent lui. Daca un alt client cu acelasi id ca cel din primit exista si
    │   este deja online atunci se va trimite un mesaj de tipul EXIT clientului
    │   care incearca sa se conecteze
    ├── socket-ul UDP - se primeste un pachet care trebuie redirectionat tuturor
    │   clientilor TCP care sunt abonati la topic-ul din pachet. Se iau din tabela
    │   de hashing "topicClients" clientii care sunt abonati. Pentru cei online se
    │   va trimite pachetul iar cei offline cu flag-ul sf = 1 se va stoca in coada
    │   de mesaje a clientului mesajul pentru a se livra mai tarziu.
    └── orice alt socket - s-a primit un pachet din partea unui dintre clientii TCP.
        Daca tipul este
                    ├── EXIT - se seteaza status-ul clientului ca offline si se inchide
                    │   socket-ul lui
                    ├── SUBSCRIBEREQ[NOSF] - se adauga in tabela din interiorul primei
                    │   tabele de hashing la topic-ul dorit un nou entry cu id-ul
                    │   clientului si flag-ul sf inclus in pachet
                    └ UNSUBSCRIBEREQ - se sterge din tabela de hashing in tabela
                        aferenta topic-ului din pachet entry-ul cu cheia egala cu id-ul
                        clientului
