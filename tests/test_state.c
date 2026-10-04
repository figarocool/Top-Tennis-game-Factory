/* Synthetic legacy save files; no original game files are needed. */
#include "../src/tournament.c"
#include "../src/replay.c"
#include <assert.h>

static FILE *stream(const void *bytes, size_t size)
{
    FILE *f = tmpfile();
    assert(f && fwrite(bytes, 1, size, f) == size);
    rewind(f);
    return f;
}

static void check_rejected(const uint8_t *bytes, size_t size)
{
    DbPlayer before_db[NPLAYERS + 1];
    uint8_t before_bracket[NPLAYERS + 2][8];
    memcpy(before_db, db, sizeof db); memcpy(before_bracket, bracket, sizeof bracket);
    int id = saved_id, round = saved_round;
    FILE *f = stream(bytes, size);
    assert(!read_state(f));
    fclose(f);
    assert(!memcmp(db, before_db, sizeof db));
    assert(!memcmp(bracket, before_bracket, sizeof bracket));
    assert(saved_id == id && saved_round == round);
}

int main(void)
{
    memset(db, 0, sizeof db); memset(bracket, 0, sizeof bracket);
    for (int i = 1; i <= NPLAYERS; i++) {
        snprintf(db[i].name, sizeof db[i].name, "PLAYER %d", i);
        db[i].points = i * 10; db[i].level = 1 + i % 4; db[i].ctrl = 1 + i % 5;
        for (int r = 1; r <= 6; r++) bracket[i][r] = i;
    }
    DbPlayer expected[NPLAYERS + 1];
    memcpy(expected, db, sizeof db);
    uint8_t bytes[SAVE_DESC + NPLAYERS * 24 + NPLAYERS * 6 + 2];
    for (int id = 1; id <= 12; id++) for (int r = 1; r <= 6; r++) {
        saved_id = id; saved_round = r;
        FILE *f = tmpfile(); assert(f);
        write_state(f, "SYNTHETIC SAVE"); rewind(f);
        assert(fread(bytes, 1, sizeof bytes, f) == sizeof bytes);
        rewind(f);
        assert(read_state(f)); fclose(f);
        assert(saved_id == id && saved_round == r && !memcmp(db, expected, sizeof db));
    }
    for (size_t cut = 0; cut < sizeof bytes; cut++) check_rejected(bytes, cut);
    const size_t bad_at[] = {0, SAVE_DESC, SAVE_DESC + 22, SAVE_DESC + 23,
                            SAVE_DESC + NPLAYERS * 24, sizeof bytes - 2, sizeof bytes - 1};
    for (unsigned i = 0; i < sizeof bad_at / sizeof *bad_at; i++) {
        uint8_t changed[sizeof bytes]; memcpy(changed, bytes, sizeof bytes);
        changed[bad_at[i]] = 255;
        check_rejected(changed, sizeof changed);
    }
    uint8_t empty[sizeof bytes]; memcpy(empty, bytes, sizeof bytes);
    empty[SAVE_DESC + NPLAYERS * 24 + 5] = 0; /* Missing participant of the saved final. */
    check_rejected(empty, sizeof empty);

    uint8_t replay[SAVE_DESC + 4 + 3 * 6 * 6] = {0};
    replay[SAVE_DESC] = 1; replay[SAVE_DESC + 1] = 2; replay[SAVE_DESC + 3] = 4;
    for (size_t i = SAVE_DESC + 4; i < sizeof replay; i++) replay[i] = (uint8_t)i;
    ReplayFile loaded;
    FILE *f = stream(replay, sizeof replay);
    assert(read_replay(f, &loaded)); fclose(f);
    assert(loaded.count == 2 && loaded.doubles == 1 && loaded.court == 4);
    assert(!memcmp(loaded.frames, replay + SAVE_DESC + 4, 3 * 6 * 6)); free(loaded.frames);
    for (size_t cut = 0; cut < sizeof replay; cut++) {
        f = stream(replay, cut);
        assert(!read_replay(f, &loaded) && !loaded.frames); fclose(f);
    }
    for (int field = 0; field < 4; field++) {
        uint8_t changed[sizeof replay]; memcpy(changed, replay, sizeof replay);
        changed[SAVE_DESC + field] = 255;
        f = stream(changed, sizeof changed);
        assert(!read_replay(f, &loaded) && !loaded.frames); fclose(f);
    }
    puts("save files: all tournament rounds, truncated/corrupt files and replay payloads: OK");
    return 0;
}
