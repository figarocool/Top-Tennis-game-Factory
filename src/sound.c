/* Sound effects. Original: Sound Blaster single-cycle 8-bit DMA at 22050 Hz, one sound at a time
 * (1000:011b = play id 1..30, 1000:0140 = busy flag). */
#include "game.h"
#include "sound.h"
#include "pak.h"
#include "platform.h"
#include "opl.h"
#include "dsimg.h"
#include <math.h>
#include <SDL.h>
#include <stdlib.h>
#include <string.h>

#define RATE   22050          /* rate of the stored sound effects */
#define OUTRATE 44100
#define NSND   31

static const char *snd_names[NSND] = {
    0, "XARXA", "BOT2", "COP", "COPFORT", "OUT", "PUBLIC1", "PUBLIC2", "PUBLICOO", "MACHINE",
    "SC_0015", "SC_0030", "SC_0040", "SC_1500", "SC_1515", "SC_1530", "SC_1540", "SC_3000", "SC_3015",
    "SC_3030", "SC_3040", "SC_4000", "SC_4015", "SC_4030", "SC_DEUCE", "SC_ADVS", "SC_ADVR",
    "SC_GAME", "SC_SET", "SC_MATCH", "SC_TIEBK",
};

static uint8_t *data[NSND];
static size_t   len[NSND];
static SDL_AudioDeviceID dev;
static int enabled = 1, volume = 100;
static const uint8_t *cur;
static size_t cur_len, cur_pos;
static uint32_t end_tick;                  /* when the current sound is over (PIT ticks) */
static int playing;
static double sfxpos;

/* ---- music: MOD_FM files (1008:0e94 / 0d30). Header 0x3f2 bytes (+0x2d channels, +0x2e rows*2 per track,
 * instruments of 0x1f bytes at +0x26+id*0x1f holding the OPL registers), then one track per channel of
 * {instrument, note} byte pairs. A 25 Hz timer advances it: a row every 3 ticks. */
typedef struct { uint8_t *d; size_t size; } Tune;
static Tune tunes[3];
static const Tune *mus;
static int mus_pos, mus_sub, mus_enabled = 1, mus_vol = 100, mus_mask[8];
static int mus_samples_left;
static const int op_base[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };

/* Turbo Pascal Real48: exponent byte, then 39 bits of mantissa and the sign */
static double real48_to_double(const uint8_t *b)
{
    if (!b[0]) return 0;
    uint64_t m = 0;
    for (int i = 5; i >= 1; i--) m = m << 8 | b[i];
    double frac = (double)(m & ((1ull << 39) - 1)) / (double)(1ull << 39);
    double v = (1 + frac) * ldexp(1.0, b[0] - 129);
    return (m >> 39) & 1 ? -v : v;
}

static void music_note_off(int c) { opl_write(0xb0 + c, 0); }

static void music_note_on(const uint8_t *r, int oct, int note, int c)      /* 1008:0a60 */
{
    opl_write(0xb0 + c, 0);
    static double freq[100];
    if (!freq[13])
        for (int i = 0; i < 100; i++) freq[i] = real48_to_double(ds_data + 0x3bba + i * 6);
    int o = op_base[c];
    opl_write(0x20 + o, r[0]); opl_write(0x40 + o, r[2]); opl_write(0x60 + o, r[4]); opl_write(0x80 + o, r[6]); opl_write(0xe0 + o, r[8]);
    opl_write(0x23 + o, r[1]); opl_write(0x43 + o, r[3]); opl_write(0x63 + o, r[5]); opl_write(0x83 + o, r[7]); opl_write(0xe3 + o, r[9]);
    opl_write(0xc0 + c, r[10]);
    int u = (int)(freq[oct * 12 + note] * (double)(1 << (21 - oct)) / 49716.0);
    opl_write(0xa0 + c, (u >> 2) & 0xff);
    opl_write(0xb0 + c, ((oct << 2) | 0x20 | (u >> 10)) & 0xff);
}

static void music_tick(void)                                                /* 1008:0d30 */
{
    if (!mus) return;
    int nch = mus->d[0x2d], len = mus->d[0x2e] | mus->d[0x2f] << 8;
    if (mus_sub == 0 && mus_pos < len) {
        for (int c = 0; c < nch && c < 9; c++) {
            const uint8_t *t = mus->d + 0x3f2 + c * len;
            int note = t[mus_pos + 1];
            if (mus_mask[c] && note) {
                int n = note & 15, oct = (note >> 4) + 3;
                if (n >= 13) n = 12; else if (n == 0) n = 1;
                if (oct >= 9) oct = 8; else if (oct == 0) oct = 1;
                music_note_on(mus->d + 0x26 + t[mus_pos] * 0x1f, oct, n, c);
            }
        }
        mus_pos += 2;
    } else if (mus_sub == 0) {
        for (int c = 0; c < nch && c < 9; c++) music_note_off(c);
    }
    if (++mus_sub > 2) mus_sub = 0;
}

static void callback(void *ud, Uint8 *stream, int n)
{
    (void)ud;
    int16_t *o = (int16_t *)stream;
    int cnt = n / 2;
    static float buf[2048];
    for (int done = 0; done < cnt;) {
        int chunk = cnt - done;
        if (chunk > 2048) chunk = 2048;
        if (mus) {
            if (chunk > mus_samples_left) chunk = mus_samples_left;
        }
        memset(buf, 0, chunk * sizeof(float));
        if (mus) {
            opl_mix(buf, chunk);
            mus_samples_left -= chunk;
            if (mus_samples_left <= 0) { music_tick(); mus_samples_left = OUTRATE / 25; }
        }
        for (int i = 0; i < chunk; i++) {
            float v = buf[i] * mus_vol / 100.0f;
            if (cur && (size_t)sfxpos < cur_len) {
                v += ((int)cur[(size_t)sfxpos] - 128) / 128.0f * volume / 100.0f;
                sfxpos += (double)RATE / OUTRATE;
            } else if (cur) { cur = NULL; }
            if (v > 1) v = 1; else if (v < -1) v = -1;
            o[done + i] = (int16_t)(v * 30000);
        }
        done += chunk;
        if (!cur) sfxpos = 0;
    }
}

int snd_init(void)
{
    for (int i = 1; i < NSND; i++) {
        char name[64];
        snprintf(name, sizeof name, "DATA\\SOUND\\%s.SND", snd_names[i]);
        data[i] = pak_load(name, &len[i]);
    }
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = OUTRATE; want.format = AUDIO_S16SYS; want.channels = 1; want.samples = 1024; want.callback = callback;
    opl_reset(OUTRATE);
    static const char *mof[3] = { 0, "DATA\\SOUND\\TUNE2.MOF", "DATA\\SOUND\\YOUWIN.MOF" };
    for (int i = 1; i <= 2; i++) tunes[i].d = pak_load(mof[i], &tunes[i].size);
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev) SDL_PauseAudioDevice(dev, 0);
    return 0;
}

void snd_shutdown(void)
{
    if (dev) SDL_CloseAudioDevice(dev);
    dev = 0;
    for (int i = 1; i < NSND; i++) { free(data[i]); data[i] = NULL; }
}

void snd_set_volume(int pct) { volume = pct < 0 ? 0 : pct > 100 ? 100 : pct; }
void snd_set_enabled(int on) { enabled = on; }

void snd_play(int id)
{
    if (!enabled || id < 1 || id >= NSND || !data[id]) return;
    if (dev) SDL_LockAudioDevice(dev);
    cur = data[id]; cur_len = len[id]; cur_pos = 0; sfxpos = 0;
    if (dev) SDL_UnlockAudioDevice(dev);
    end_tick = timer_ticks() + (uint32_t)((uint64_t)len[id] * PIT_HZ / RATE);
    playing = 1;
}

int snd_busy(void)
{
    if (!playing) return 0;
    if ((int32_t)(timer_ticks() - end_tick) >= 0) playing = 0;
    return playing;
}

void snd_set_music(int pct, int on) { mus_vol = pct < 0 ? 0 : pct > 100 ? 100 : pct; mus_enabled = on; if (!on) music_stop(); }

void music_stop(void)                                                      /* 1000:0516 */
{
    if (dev) SDL_LockAudioDevice(dev);
    if (mus) for (int c = 0; c < 9; c++) music_note_off(c);
    mus = NULL;
    if (dev) SDL_UnlockAudioDevice(dev);
}

void music_start(int n)                                                    /* 1000:0469 */
{
    if (!dev || !mus_enabled || n < 1 || n > 2 || !tunes[n].d) return;
    music_stop();
    SDL_LockAudioDevice(dev);
    opl_reset(OUTRATE);
    opl_write(1, 0x20);
    for (int c = 0; c < 8; c++) mus_mask[c] = !(n == 2 && c == 1);          /* YOUWIN: the 2nd track is muted */
    mus_pos = 0; mus_sub = 0; mus_samples_left = 1;
    mus = &tunes[n];
    SDL_UnlockAudioDevice(dev);
}
