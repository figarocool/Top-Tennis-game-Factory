# Top Tennis — riscrittura in C / Top Tennis — C rewrite

**Top Tennis** (The Game Factory, 1997, programmato da John Dolph) era un gioco di tennis per DOS scritto in
Borland Pascal 7. Questo progetto è una **riscrittura fedele in C con SDL2**, ottenuta studiando il
programma originale (reverse engineering), che gira su Linux, su altri sistemi con SDL2 e su **PS Vita**.
Non è un emulatore: la logica di gioco è riscritta, con la stessa fisica della palla, la stessa IA, gli stessi menu
e gli stessi tempi del gioco originale.

*English: a faithful C/SDL2 rewrite of the 1997 DOS tennis game Top Tennis, reverse engineered from the original
executable. It runs natively on Linux (SDL2) and on the PS Vita. You need the original game's `TENNIS.DAT`.*

## Cosa serve (importante)

Il repository contiene **solo il codice**. La grafica, i suoni e la musica sono di proprietà dei loro autori e
**non sono inclusi**: servono i file `TENNIS.DAT` (e, se vuoi, `TENNIS.OPT`) del gioco originale, che devi
possedere. Le tabelle e i testi del programma (animazioni, menu, nomi dei giocatori) sono invece compilati
nel codice (`src/dsdata.c`, generato da `tools/gen_dsdata.py`).

## Cosa c'è nel gioco

- Partita amichevole, singolo e doppio fino a 4 giocatori (2 tastiere + 2 joystick).
- **Torneo** a eliminazione diretta con 64 giocatori e 12 città, **stagione** completa con classifica mondiale.
- Allenamento: servizio e macchina lanciapalle con 12 tiri.
- Replay dell'ultimo punto (F3), replay salvabili, Hall of Fame, salvataggio e caricamento di tornei e stagioni.
- 4 tipi di campo (terra, cemento, erba, indoor), 4 livelli della CPU, partite al meglio dei 3 o dei 5 set.
- Effetti sonori e musica FM (sintetizzatore OPL2 scritto per il progetto).
- Menu e finestre identici all'originale, velocità di gioco regolabile; il ritmo non dipende dalla velocità del PC.

## Compilare ed eseguire (Linux)

    sudo apt install build-essential libsdl2-dev
    make
    ./toptennis cartella_del_gioco      # deve contenere TENNIS.DAT; senza argomento usa ./orig

Opzioni, salvataggi, replay e Hall of Fame vengono scritti nella stessa cartella. F11 = schermo intero.
Durante la partita: ESC esce, F3 replay, F5 pausa, F10 schermata "boss".

## PS Vita

Scarica `TopTennis.vpk` dalle [Release](../../releases) (oppure compilalo, vedi sotto).

1. Installa il VPK con VitaShell (serve il modo "unsafe homebrew" di HENkaku abilitato).
2. Copia `TENNIS.DAT` in `ux0:data/TopTennis/`.
3. Avvia "Top Tennis" dalla LiveArea.

Comandi: croce direzionale o levetta = movimento, croce = fuoco/conferma, cerchio = ESC, triangolo = replay (F3),
quadrato = S, Start = pausa, L/R = Y/N. Nei menu la levetta sinistra muove un puntatore (croce = click) e si può
toccare lo schermo; per scrivere i nomi compare una tastiera touch. Se qualcosa non va, il gioco scrive
`ux0:data/TopTennis/log.txt`.

Per compilarlo serve [VitaSDK](https://vitasdk.org):

    export VITASDK=/usr/local/vitasdk
    cd vita && mkdir build && cd build && cmake .. && make      # produce TopTennis.vpk

## PSP (sperimentale)

Il progetto per PSPDEV è in `psp/`:

    export PSPDEV=/usr/local/pspdev && export PATH=$PSPDEV/bin:$PATH
    cd psp && mkdir build && cd build && psp-cmake .. && make      # produce EBOOT.PBP

Copia `EBOOT.PBP` e `TENNIS.DAT` nella stessa cartella, per esempio `ms0:/PSP/GAME/TopTennis/`. I comandi sono quelli della Vita;
non c'è il touch, quindi nei menu la levetta analogica muove il puntatore (croce = click) e i nomi si scrivono con la tastiera
a schermo. Compila, ma non è stato provato né su una PSP né su PPSSPP: potrebbero servire ritocchi alle prestazioni.

## Come è fatto

| Cartella | Contenuto |
|---|---|
| `src/` | il gioco: fisica della palla (`ball.c`), colpi (`shot.c`), IA e controlli (`ctrl.c`), partita (`match.c`), punteggio, menu e dialoghi, torneo e stagione (`tournament.c`), replay, Hall of Fame, audio e sintetizzatore OPL2 |
| `vita/` | progetto CMake per VitaSDK |
| `tools/` | estrazione di `TENNIS.DAT`, decodifica degli sprite, generatore di `dsdata.c`, script per confrontare con DOSBox |
| `re/` | note di analisi (`ANALYSIS.md`) e script Ghidra usati per il reverse engineering |

Variabili d'ambiente per i test: `TT_SHOT`/`TT_SHOT_FRAMES` (screenshot), `TT_INPUT` (tasti scriptati),
`TT_MENU`, `TT_CTRL` (forza i controlli, 5 = CPU), `TT_VJOY` (joystick virtuale), `TT_DEBUG`.

## Differenze note dall'originale

- La musica usa un sintetizzatore OPL2 nuovo: note e tempi sono quelli originali, il timbro è approssimato.
- Il joystick usa assi già calibrati da SDL: le schermate di calibrazione ci sono ma non memorizzano nulla.
- La barra "LOADING" ha una durata fissa (nell'originale dipendeva dalla lettura dei file).
- Chi carica un torneo o una stagione salvati riparte dal turno salvato, come faceva l'originale.

## Note legali

Progetto non ufficiale e senza scopo di lucro, non affiliato a The Game Factory né a John Dolph. *Top Tennis* e
tutti i suoi contenuti (grafica, suoni, musica) appartengono ai rispettivi titolari e non sono distribuiti qui.
Il codice di questo repository è fornito così com'è, per studio e preservazione.
