# Top Tennis (The Game Factory, 1997) — analisi del reverse engineering

Stato: decompilazione completa (734 funzioni, `re/decomp.c`), lettura del motore di gioco, formati dati chiariti.
Strumenti: Ghidra 12.1.4 (`~/tools`), progetto in `ghidra_proj/`, script in `re/scripts/`,
`re/clean.py` (ripulisce la decompilazione → `re/decomp_clean.c`), `re/show.py FUN_xxxx_yyyy` (vista compatta).

## 1. Binario
- `TENNIS.EXE`: NE 16 bit, **Borland Pascal 7** in modalità protetta (DPMI via `RTM.EXE` + `DPMI16BI.OVL`).
- 8 segmenti NE → in Ghidra `1000..1040`; `1030` = segmento dati (DS). Dati inizializzati estratti in `re/ds.bin`
  (file offset 0x23900, 19876 byte). Molte tabelle di gioco stanno lì (vedi §6).
- Entry (`1000:ed2b`) = blocco principale del programma Pascal.

| Segmento | Contenuto |
|---|---|
| 1000 | **Programma del gioco** (198 funzioni, 62 KB): menu, partita, fisica, IA, punteggio, replay, torneo/stagione |
| 1008 | Unit di supporto: sprite manager, timer (PIT), audio (OPL 0x388, SB DMA), mouse (int 33h), tastiera, caricamento CBE/PBM |
| 1010 | Unit stringhe/Crt-like, palette VGA (3C7/3C8/3C9), vsync, util di memoria |
| 1018 | **Libreria grafica Mode X** in assembly (blit, fill, scroll CRTC, palette, font glyph) |
| 1020 | Unit file/PAK (`TENNIS.DAT`) + joystick (porta 201h) |
| 1028 | `System` di Borland Pascal (runtime: GetMem `035c`, FreeMem `0376`, StackCheck `05eb`, Random `1815`, Write...) |

Inizializzazione (da `entry`): unit init 1028:0007, 1008:3e9b, 1020:0c28, 1008:3a43, 1010:3c8a, 1008:391a,
1008:29fb, 1008:2784, 1010:34dd, 1008:1d43/1c28/1009/0feb/0989/0857 → apertura `TENNIS.DAT` (obj a DS:951E) →
config audio (`SOUND.CFG`) → mouse → carica palette menu (`SORRA, MENUOPT, MENUTRAI, ...`) e font (slot 5..8:
MYFONT, SCORE, FONT5x5, DIALOG1) → crea sprite palla/ombra/giocatori (`e889`) → menu principale → partita.

## 2. TENNIS.DAT
`u16 count(=644) ; u32 dir_pos` poi dati; directory a fine file: `count × 64 byte`:
`char path[56]` (NUL-term, spazzatura dopo il NUL) + `u32 size` + `u32 offset`. Nessuna compressione.
Estrattore: `tools/extract_dat.py` → `data/`. Lookup nel gioco: `1020:0a2b` (ricerca lineare per nome, confronto stringhe Pascal).

## 3. Formati asset
- **PBM** (66): `u8 w/4, u8 h` + 4 piani Mode X (80×200 ciascuno). Palette: `.PAL` omonimo, 256×RGB a 6 bit.
- **CBE** (492): sprite compilati = codice x86 che scrive nei 4 piani VGA. 2 byte header (`w_bytes, h`),
  poi `mov byte/word [si+disp],imm` per piano, separati da `rol al,1; adc si,0; out dx,al`; stride 104 byte/riga
  (schermo virtuale 416 px). Decoder: `tools/cbe.py`. Il gioco li carica con `1008:359a`, li tratta come "sprite def" (800 slot).
- **SND**: PCM 8-bit unsigned (frequenza da verificare). **MOF**: "MOD_FM by John Dolph" (musica mod con strumenti FM).
- **FNT**: font bitmap (4); il glifo 1bpp viene disegnato da `1018:29d1`. Layout da analizzare.
- File utente: `TENNIS.OPT` (76 byte opzioni), `TENNIS.HAL` (hall of fame), `SOUND.CFG` (5 byte), `REPLAY.000`,
  `SEASON.000`, `TOURNAMN.000` (salvataggi).

## 4. Sprite animati dei giocatori
`PLYR_WHI` e `PLYR_BLU` × varianti `_D` (giocatore lato vicino, di fronte) e `_U` (lato lontano, di spalle).
Animazioni (n. frame): BOTAR(3 palleggio), QUIET(8 fermo), CAMIA/CAMID/CAMIE (camminata avanti/destra/sinistra),
CORRD/CORRE (corsa dx/sx), DRIV1/2 (dritto), REVS1/2 (rovescio), SAKE1/2 (servizio), VOLD1/2, VOLR1/2 (volée).
`e889` assegna a ogni animazione un id sprite (passo 10: 10,20,30...; BLU = +400 rispetto a WHI; `_U` = +0xB4).
Tabella per animazione/frame → id sprite: DS `0x39e` (lato D, stride 0x14) e DS `0x88a` (lato U); numero frame: DS `0x2f3`;
offset di disegno per animazione: DS `0x333`, `0x373`. Mapping indice-anim → animazione base: DS `0xdcb`.

## 5. Modello a oggetti (record Pascal)
**TPlayer** (puntatori in DS:9B1C, 9B20, 9B24, 9B28 — 4 slot: 2 giocatori + 2 partner doppio):
`+0x15 u16 input_bits` (bit0 fuoco/colpo A, bit1 colpo B, bit2 su?, bit3 giù?, bit4 …), `+0x17 id (1/2)`,
`+0x18 handle sprite`, `+0x19 anim corrente`, `+0x1a frame`, `+0x1b tick nel frame`, `+0x1c indice personaggio/sprite-set`,
`+0x1d lato (0=D,1=U)`, `+0x1e bit di direzione CPU`, `+0x1f lato campo`, `+0x20 umano(1)/CPU(0)`,
`+0x21,+0x22 timer`, `+0x23 buffer replay (voci da 6 byte: x,y,sprite)`; `+0x11 puntatore joystick`.
**TBall** (DS:9800): `+0x00..` Bresenham (x0,y0,x1,y1,dx,dy,passi,err...), `+0x16 x`, `+0x18 y (a terra)`, `+0x24 altezza`,
`+0x22 t (0..0xB4)`, `+0x34 passi/frame`, `+0x35 stato`, `+0x36/+0x37 handle sprite palla/ombra`,
`+0x3c id sprite (0x316..0x31c = BOLA1..7 rotazione)`, `+0x40/+0x44 buffer replay`. Traiettoria = retta a passi di Bresenham
(`1afc` la imposta) × profilo di altezza a punto fisso 16.16 campionato su t (tabella a DS `0x9848 + t*4`, fuori dai dati
inizializzati: viene riempita a runtime da una init ancora da trovare).
Campo logico: x 10..405, y 10..288; rete a y≈0x95..0x9b.
**TScore** (DS:957C): `+0..0x14` nome1 (string[20]), `+0x15..0x29` nome2, `+0x2a/+0x2b` punti (0,15,30,40,50=vantaggio),
`+0x2c..0x30` game set1..5 p1, `+0x31..0x35` p2, `+0x36/+0x37` set vinti, `+0x38` set corrente, `+0x39` chi serve,
`+0x3a` flag best-of-3, `+0x3b` cambio campo, `+0x3c` tiebreak. Funzioni: `0600` punto, `0768` game, `0974` set,
`09e6` cambio campo, `0ae9` disegno tabellone, `053a` init, `05b2` set nomi.

## 6. Flusso di una partita
`73d8` (partita) → `7099` (un game) → `6cf0` (un punto: loop 1° e 2° servizio) → `681c/6a85` (servizio lato D/U) →
loop di rally per frame:
1. attesa timer (`1008:29aa`), 2. `669c` camera/scroll che segue la palla (`1018:1aca/1792`), 3. `31df/326d` rete/out,
4. `3e1f` collisione racchetta–palla → `259c` sceglie obiettivo/tipo colpo (per id colpo `0x12..0x3f`), `1afc` lancia la palla,
5. `3b15` per ogni TPlayer: avanza animazione e chiama il controller: **umano `408b`/`486a`** (da `input_bits`,
   letti da `3f85` da tastiera/joystick) o **CPU `5049`/`5a42`** (usa Random `1028:1815`, posizione palla, livello),
   ritorno = `(nuova_anim<<8)|flag`, 6. `7f11/8033` aggiornano sprite/ombra, 7. `7f57` condizioni di fine punto.
Suoni: `011b(n)` riproduce SND n (1..30; ordine in `0199`: ... vedi `data/DATA/SOUND`).
Replay: ogni frame registra (x,y,sprite) in buffer da 6 byte (`235c`, `3804`) e li riproduce (`2421`, `3870`).

## 7. Sprite manager (1008:2b06…30d5)
100 handle (0..99, `FUN_1008_2b98(x,y,dim,handle)` crea), 800 definizioni sprite (`2d62(id,handle)` assegna), `2dd8/2e0c`
mostra/nasconde, `2f96` posizione, `309f` record, `30d5` ridisegna (salva/ripristina sfondo per pagina, ordine per handle).
Handle: i primi creati in `entry` (`2b98` con id 5..10) sono sprite di sistema (macchina lanciapalle MACHINEU/D, ombra,
OUT, rete XARXA2, palla); la mappa esatta handle→oggetto è da completare (i TPlayer hanno `+0x18`, la palla `+0x36/+0x37`).

## 8. Hardware / driver
Grafica Mode X 320×200 (stride virtuale 104), 3 pagine, scroll via CRTC 0Ch/0Dh + PEL panning 3C0h; vsync `1018:1bd1`.
Audio: rilevamento OPL (388h), Sound Blaster DMA (porte 0B/0C/81..87), timer PIT 40h/43h per tempi di gioco.
Input: tastiera (ISR int 9 = `64ca/65d2/65fe`, installata con `1008:3a05` da `666b`), joystick analogico 201h (calibrazione `bb23`), mouse int 33h (solo menu).

## 9. Da fare / non ancora analizzato
Menu e opzioni (`a84d`, `8318`, `a357`), torneo/stagione/classifica (`89b5`, `8df4`, `9907`), salvataggi (`bf04`, `d305`,
`d734`, `c889`), decoder SND/MOF e driver audio (`1008:0995…1d4f`), font FNT, tabelle IA (livelli CPU), `7f11/8033/7f57/7dac/7e72`,
`774f/7924` (celebrazioni?), unit stringhe `1010`, `1008:30d5` (compositing).
