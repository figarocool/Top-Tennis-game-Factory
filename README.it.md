# Top Tennis — motore compatibile

🇬🇧 [English](README.md) · 🇮🇹 Italiano

**Top Tennis** è un motore di compatibilità non ufficiale in C/SDL2 per Linux, **PS Vita** e **PSP**,
che usa i file forniti dalla tua copia del gioco DOS Top Tennis (The Game Factory, 1997; John Dolph).
Il progetto è stato sviluppato mediante reverse engineering. I dati originali vengono letti localmente dai propri file.
La versione 1.4 ripristina controlli, colpi e fisica della versione 1.1. La versione 1.3 aveva controlli e colpi diversi.

## Cosa serve (importante)

Serve una copia originale che hai diritto di utilizzare. Gli eseguibili originali, la grafica, i suoni, la musica e
le tabelle estratte **non sono inclusi** nei sorgenti attuali né nei nuovi pacchetti.

Puoi scegliere:

- **Solo un DAT sulla console (consigliato):** sul PC usa lo script Python con `TENNIS.EXE` e `TENNIS.DAT` della tua
  copia originale. Copia il DAT preparato sulla console: lì non serve più l'EXE.
- **File originali direttamente:** copia insieme `TENNIS.EXE` e `TENNIS.DAT` nella cartella dati del motore.
  L'EXE viene letto come sorgente di dati; il suo codice non viene eseguito.

Scarica `TopTennis-Data-Tools.zip` dall'[ultima release](https://github.com/figarocool/Top-Tennis-game-Factory/releases/latest),
estrailo ed entra nella cartella `TopTennis`. Installa Python 3 sul PC, poi esegui:

```sh
python3 tools/prepare_data.py "/percorso/della/copia/originale" --output "prepared/TENNIS.DAT"
```

Su Windows usa `py -3` al posto di `python3`. La cartella di partenza deve contenere **sia `TENNIS.EXE` sia
`TENNIS.DAT` originali**. Lo script crea un solo nuovo `prepared/TENNIS.DAT`, conservando gli originali;
non sovrascrive un output esistente. Copia il file generato, senza l'EXE, nella cartella della piattaforma:

| Piattaforma | Destinazione del DAT preparato |
|---|---|
| Vita nativa | `ux0:/data/TopTennis/TENNIS.DAT` |
| PSP sulla Vita (Adrenaline) | `ux0:/pspemu/PSP/GAME/TopTennis/TENNIS.DAT`, accanto a `EBOOT.PBP` |
| PSP | `ms0:/PSP/GAME/TopTennis/TENNIS.DAT`, accanto a `EBOOT.PBP` |
| Linux | `prepared/TENNIS.DAT`; avvia `./toptennis prepared` |

Il DAT preparato contiene dati della tua copia: mantienilo privato e non includerlo in Git o nei pacchetti pubblici.
È supportata la versione completa del 1997 verificata dai lettori; altre versioni vengono rifiutate con un messaggio.
`TENNIS.OPT` è facoltativo. **Il DAT originale da solo non basta**: prepara il nuovo DAT oppure copia EXE e DAT insieme.
Vedi la [guida passo passo, con esempi Windows e risoluzione degli errori](DATA_SETUP.md).

## Cosa c'è nel gioco

- Partita amichevole, singolo e doppio fino a 4 giocatori (2 tastiere + 2 joystick).
- **Torneo** a eliminazione diretta con 64 giocatori e 12 città, **stagione** completa con classifica mondiale.
- Allenamento: servizio e macchina lanciapalle con 12 tiri.
- Replay dell'ultimo punto (F3), replay salvabili, Hall of Fame, salvataggio e caricamento di tornei e stagioni.
- 4 tipi di campo (terra, cemento, erba, indoor), 4 livelli della CPU, partite al meglio dei 3 o dei 5 set.
- Effetti sonori e musica FM (sintetizzatore OPL2 scritto per il progetto).
- Menu e finestre basati sui dati della tua copia, velocità di gioco regolabile.

## Compilare ed eseguire (Linux)

    sudo apt install build-essential libsdl2-dev
    make
    ./toptennis prepared                # basta il DAT preparato, senza EXE
    ./toptennis cartella_del_gioco       # alternativa: EXE + DAT originali

Opzioni, salvataggi, replay e Hall of Fame vengono scritti nella stessa cartella. F11 = schermo intero, F12 = formato dello schermo 4:3 / 16:9 (sulle console si sceglie dal menu OPTIONS, di default 16:9). Senza argomento usa `./orig`. File mancanti, versioni non supportate o DAT preparati non validi vengono segnalati a schermo.
Durante la partita: ESC esce, F3 replay, F5 pausa, F10 schermata "boss".

## Partita in rete (LAN / Wi-Fi)

Dal menu principale, **NETWORK**: uno dei due sceglie **HOST** (gioca in basso e comanda le impostazioni del menu PLAY: campo,
numero di set, velocità), l'altro **JOIN** (gioca in alto). Funziona tra PC, PS Vita e PSP in qualsiasi combinazione,
purché siano sulla stessa rete locale (stesso router/Wi-Fi): il guest cerca da solo le partite aperte, oppure si può
scrivere l'indirizzo IP che l'host vede sullo schermo. Sulla PSP si usa il primo profilo Wi-Fi salvato.

Come funziona: protocollo UDP sulla porta 5757; le due macchine simulano la stessa partita e a ogni fotogramma si
scambiano solo i tasti premuti (2 fotogrammi di ritardo per nascondere la latenza). Un controllo periodico dello stato
della partita avverte se le due simulazioni divergono. ESC chiude la partita per entrambi. Sul PC può servire aprire la
porta UDP 5757 nel firewall. Al momento sono solo partite amichevoli uno contro uno.

Entrambi devono usare la stessa build del motore e dati originali compatibili. La versione 1.4 usa il protocollo di rete 3 ed è incompatibile con la versione 1.3.

## PS Vita

Scarica `TopTennis.vpk` dalle [Release](../../releases) (oppure compilalo, vedi sotto).

1. Installa il VPK con VitaShell (serve il modo "unsafe homebrew" di HENkaku abilitato).
2. Copia il tuo `TENNIS.DAT` **preparato** in `ux0:data/TopTennis/`. In alternativa copia lì entrambi i file originali.
3. Avvia "Top Tennis" dalla LiveArea.

Comandi: croce direzionale o levetta = movimento, croce = fuoco/conferma, cerchio = ESC, triangolo = replay (F3),
quadrato = S, Start = pausa, L/R = Y/N. Nei menu la levetta sinistra muove un puntatore (croce = click) e si può
toccare lo schermo; per scrivere i nomi compare una tastiera touch. Se qualcosa non va, il gioco scrive
`ux0:data/TopTennis/log.txt`.

Per compilarlo serve [VitaSDK](https://vitasdk.org):

    export VITASDK=/usr/local/vitasdk
    cd vita && mkdir build && cd build && cmake .. && make      # produce TopTennis.vpk

## PSP

Scarica `TopTennis-PSP.zip` dalle [Release](../../releases) ed estrailo, oppure compila con PSPDEV (progetto in `psp/`):

    export PSPDEV=/usr/local/pspdev && export PATH=$PSPDEV/bin:$PATH
    cd psp && mkdir build && cd build && psp-cmake .. && make      # produce EBOOT.PBP

Copia `EBOOT.PBP` e il tuo `TENNIS.DAT` **preparato** nella stessa cartella, per esempio `ms0:/PSP/GAME/TopTennis/` (sulla Vita, nell'emulatore
PSP: `ux0:pspemu/PSP/GAME/TopTennis/`). I comandi sono quelli della Vita; non c'è il touch, quindi nei menu la levetta
analogica muove il puntatore (croce = click) e i nomi si scrivono con la tastiera a schermo. La PSP disegna direttamente
nel frame buffer. La versione 1.4 è stata compilata per Vita e PSP e verificata su Linux. Questa release non è stata provata su console fisiche.

In alternativa copia insieme i tuoi `TENNIS.EXE` e `TENNIS.DAT` originali. Le build console usano di default la
grafica realizzata per il progetto (`TT_PUBLIC=ON`); la grafica privata è una scelta esplicita per build locali.

## Come è fatto

| Cartella | Contenuto |
|---|---|
| `src/` | il gioco: fisica della palla (`ball.c`), colpi (`shot.c`), IA e controlli (`ctrl.c`), partita (`match.c`), punteggio, menu e dialoghi, torneo e stagione (`tournament.c`), replay, Hall of Fame, audio e sintetizzatore OPL2 |
| `vita/` | progetto CMake per VitaSDK |
| `psp/` | progetto CMake per PSPDEV, con la grafica pubblica dell'XMB in `art_public/` |
| `tools/` | preparazione locale del DAT, estrazione di `TENNIS.DAT`, decoder sprite, audit delle release e confronto con DOSBox |
| `re/` | strumenti tecnici per lo studio dei formati |

Variabili d'ambiente per i test: `TT_SHOT`/`TT_SHOT_FRAMES` (screenshot), `TT_INPUT` (tasti scriptati),
`TT_MENU`, `TT_CTRL` (forza i controlli, 5 = CPU), `TT_VJOY` (joystick virtuale), `TT_TURBO` (orologio più veloce),
`TT_TRACE`, `TT_DEBUG`. Sulle console, senza variabili d'ambiente, si possono mettere nella cartella del gioco i file
`tt_input.txt` (script dei tasti) e `tt_env.txt` (righe `NOME=valore`).

## Differenze note dall'originale

- Controller, colpi e fisica in sviluppo puntano al comportamento della versione 1.1. I confronti automatici
  coprono singoli stati; l’equivalenza completa con ogni situazione del gioco DOS non è ancora accertata.
- La musica usa un sintetizzatore OPL2 nuovo: note e tempi sono quelli originali, il timbro è approssimato.
- Il joystick usa assi già calibrati da SDL: le schermate di calibrazione ci sono ma non memorizzano nulla.
- La barra "LOADING" ha una durata fissa (nell'originale dipendeva dalla lettura dei file).
- Chi carica un torneo o una stagione salvati riparte dal turno salvato, come faceva l'originale.

## Licenza e attribuzioni

Progetto non ufficiale, non affiliato né approvato dai titolari del gioco originale. Usa una copia che hai diritto
di utilizzare; i file originali e il DAT preparato non sono inclusi nelle distribuzioni. La licenza del progetto
non concede diritti sui contenuti del gioco. Vedi [LICENSE.md](LICENSE.md) e
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) per le condizioni e le attribuzioni dei componenti.

## Verifiche

`make test` verifica parser e nuovi controller senza file originali. Per le verifiche locali di compatibilità:
`TT_ORIGINAL_DIR=/percorso/della/tua/copia make test`. Originali e DAT preparati non sono fixture pubbliche dei test.
