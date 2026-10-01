#ifndef SOUND_H
#define SOUND_H
int  snd_init(void);          /* loads DATA\SOUND\*.SND (ids 1..30) and opens the audio device (optional) */
void snd_shutdown(void);
void snd_set_volume(int pct); /* sound effects volume 0..100 */
void snd_set_enabled(int on);
void snd_set_music(int pct, int on);
void music_start(int n);      /* 1000:0469: 1 = TUNE2 (menus), 2 = YOUWIN */
void music_stop(void);        /* 1000:0516 */
#endif
