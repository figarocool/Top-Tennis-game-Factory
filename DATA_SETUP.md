# Prepare your data / Prepara i tuoi dati

## Italiano

Lo strumento Python prepara **un unico `TENNIS.DAT`** per questo motore. Legge i tuoi `TENNIS.EXE` e `TENNIS.DAT`,
copia il DAT e aggiunge i dati necessari letti dall'EXE. Non esegue l'EXE, non scarica contenuti e non modifica gli
originali. Richiede Python 3, senza pacchetti aggiuntivi. Il file generato non è destinato al gioco DOS originale.

1. Recupera dalla tua copia del gioco originale i file `TENNIS.EXE` e `TENNIS.DAT` e mettili nella stessa cartella.
   Serve la versione completa del 1997 supportata dai lettori: lo script segnala le versioni incompatibili.
2. Scarica `TopTennis-Data-Tools.zip` dall'[ultima release](https://github.com/figarocool/Top-Tennis-game-Factory/releases/latest)
   ed estrailo. In alternativa usa la cartella del repository. Entrambe contengono `tools/prepare_data.py`.
3. Installa Python 3 e apri il terminale nella cartella estratta `TopTennis` (o nella radice del repository).
4. Esegui uno dei seguenti comandi, sostituendo il percorso con la tua cartella originale.

**Windows, PowerShell o Prompt dei comandi:**

```powershell
py -3 tools/prepare_data.py "C:\Giochi\Top Tennis Originale" --output "prepared\TENNIS.DAT"
```

Se il comando `py` non esiste, prova `python` al suo posto.

**Linux o macOS:**

```sh
python3 tools/prepare_data.py "$HOME/Giochi/Top Tennis Originale" --output "prepared/TENNIS.DAT"
```

5. Attendi il messaggio di successo. In `prepared` trovi il **nuovo** `TENNIS.DAT`; la cartella di output viene creata
   automaticamente. Non occorre copiare sulla console Python, lo script o l'EXE.
6. Copia il DAT preparato nella destinazione corretta:

| Sistema | Cosa copiare e dove |
|---|---|
| Vita | Installa `TopTennis.vpk` con VitaShell; copia il DAT in `ux0:/data/TopTennis/TENNIS.DAT` |
| Adrenaline sulla Vita | Estrai il pacchetto PSP; copia `EBOOT.PBP` e il DAT in `ux0:/pspemu/PSP/GAME/TopTennis/` |
| PSP | Copia `EBOOT.PBP` e il DAT in `ms0:/PSP/GAME/TopTennis/` |
| Linux | Mantieni il DAT in `prepared/` e avvia `./toptennis prepared` |

Conserva una copia degli originali prima di sostituire un DAT già presente sulla console. Opzioni e salvataggi
restano nella cartella del gioco. Il DAT preparato contiene dati della tua copia: non pubblicarlo, non aggiungerlo
al repository e non includerlo nei pacchetti del motore.

### Se qualcosa non funziona

- **File originale mancante:** il primo percorso deve essere una cartella contenente entrambi i file originali,
  non il percorso del solo EXE e non la cartella dello script.
- **Versione non supportata:** non rinominare un altro file e non modificare gli hash dello script. Usa una copia
  con il layout supportato; lo strumento non converte automaticamente altre edizioni.
- **Output già esistente:** scegli una cartella nuova, per esempio `prepared2/TENNIS.DAT`. Lo script rifiuta la
  sovrascrittura per proteggere i file.
- **Il gioco richiede ancora l'EXE:** verifica di avere copiato il DAT generato in `prepared`, invece del DAT
  originale, e di avviare la build 1.4.
- **Percorso con spazi:** racchiudilo fra virgolette come negli esempi.

Se non vuoi usare Python, puoi mettere **EXE e DAT originali insieme** nella stessa cartella dati. Il motore legge
soltanto i dati dell'EXE. Il DAT originale da solo non contiene tutto ciò che serve.

## English

The Python tool creates **one prepared `TENNIS.DAT`** for this engine. It reads your own original EXE and DAT,
preserves the DAT's contents and appends the required data from the EXE. It never executes the EXE, downloads game
content or changes the originals. Python 3 is the only requirement. The output is for this engine, not the DOS game.

1. Put your original `TENNIS.EXE` and `TENNIS.DAT` in one folder. The verified 1997 full-version layout is supported.
2. Extract `TopTennis-Data-Tools.zip` from the release and open a terminal in its `TopTennis` folder, or use the
   repository root. Both contain `tools/prepare_data.py`.
3. On Windows run `py -3` (or `python`); on Linux/macOS use `python3`:

```sh
python3 tools/prepare_data.py "/path/to/your/original/game" --output "prepared/TENNIS.DAT"
```

4. Copy the newly generated file to `ux0:/data/TopTennis/` for native Vita, beside `EBOOT.PBP` in
   `ux0:/pspemu/PSP/GAME/TopTennis/` for Adrenaline, or `ms0:/PSP/GAME/TopTennis/` for PSP.
   On Linux run `./toptennis prepared`. No EXE or Python installation is needed on the console.

Back up an existing console DAT before replacing it. Keep the output private: it contains original game data and
must not be uploaded to Git or distributed with the engine. Missing originals, unsupported layouts and existing
outputs produce explicit errors. Choose a new output directory if one already exists. If the engine still asks
for an EXE, check that you copied the prepared DAT and are running version 1.4. Alternatively supply your original
EXE and DAT together; an unprepared original DAT alone is insufficient.
