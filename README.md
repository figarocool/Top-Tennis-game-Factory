# Top Tennis — riscrittura in C (SDL2)

Riscrittura fedele di *Top Tennis* (The Game Factory, 1997, DOS / Borland Pascal 7) ottenuta dal reverse
engineering di `TENNIS.EXE`. Il codice è C puro; **gli asset non sono inclusi**: il gioco legge
`TENNIS.DAT` e i dati inizializzati di `TENNIS.EXE` dalla cartella del gioco originale.

## Compilare ed eseguire

    sudo apt install build-essential libsdl2-dev
    make
    ./toptennis [cartella_del_gioco]        # default: ./orig  (deve contenere TENNIS.DAT e TENNIS.EXE)

TENNIS.OPT, TENNIS.HAL, TOURNAMN.nnn, SEASON.nnn e REPLAY.nnn vengono letti/scritti in quella cartella.
F11 = schermo intero. Tasti di gioco come nel manuale originale (ESC esce dalla partita, F3 replay, F5 pausa, F10 boss).

## Cosa c'è

Menu, opzioni, partita amichevole singolo/doppio (fino a 4 giocatori: 2 tastiere, 2 joystick SDL), training
(servizio e macchina lanciapalle), demo, tornei a eliminazione (64 giocatori), stagione di 12 tornei con
classifica, salvataggio/caricamento di tornei, stagioni e replay, replay F3 (avanti/indietro con ←/→, S per salvare),
Hall of Fame, effetti sonori, musica FM.

## Differenze note rispetto all'originale

* **Musica**: i file MOF sono riprodotti da un sintetizzatore OPL2 scritto per l'occasione; la sequenza è identica
  ma il timbro è approssimato (non è un emulatore cycle-exact).
* **Joystick**: SDL fornisce assi già calibrati; le schermate di calibrazione percorrono gli stessi passi ma
  non memorizzano nulla.
* La schermata "LOADING ... PLEASE WAIT" non c'è: il caricamento è istantaneo.
* Formato dei file di salvataggio proprio (descrizione di 26 byte + dati), non garantito compatibile con quelli DOS.

## Strumenti

`re/` contiene note di analisi (`ANALYSIS.md`), decompilazione e script Ghidra; `tools/` estrattori
(`extract_dat.py`, `cbe.py`) e lo script per confrontare con DOSBox (`dosrun.sh`).
Variabili di test: `TT_CTRL=p1,p2,p3,p4` (forza i controlli, 5 = CPU), `TT_VJOY="frame:x:y:pulsanti,..."` (joystick virtuale), `TT_SHOT`, `TT_SHOT_FRAMES`, `TT_INPUT`, `TT_MENU`, `TT_DEBUG`.
