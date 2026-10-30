/* Exercise formatting and disc-ID paths affected by compiler warning fixes. */
#define main cddbd_program_main
#include "../cddbd.c"
#undef main
#include "../bit_array.h"
#include <assert.h>
#include <limits.h>

unsigned int db_gen_discid(db_t *);


int db_cmp(db_t *, db_t *);

static void test_revision(const char *comment, long long expected, int valid)
{
    char error[CDDBBUFSIZ] = {0}, written[4096], revision[128];
    FILE *input = tmpfile(), *output;
    db_t *record;
    size_t count;
    assert(input != NULL);
    fprintf(input, "# xmcd\n#\n# Track frame offsets:\n#\t296\n#\n"
        "# Disc length: 344 seconds\n#\n%s\n#\n"
        "DISCID=03015501\nDTITLE=Artist / 月とカメレオン\n"
        "DYEAR=1997\nDGENRE=Rock\nTTITLE0=風の向くまま\n"
        "EXTD=\nEXTT0=\nPLAYORDER=\n", comment);
    rewind(input);
    record = db_read(input, error, DF_ENC_UTF8 | DF_ENC_LATIN1);
    fclose(input);
    if(!valid) {
        assert(record == NULL);
        assert(strstr(error, "revision") != NULL);
        return;
    }
    if(!record) fprintf(stderr, "Revision test failed: %s: %s\n", comment, error);
    assert(record != NULL);
    assert(record->db_rev == expected);
    output = tmpfile();
    assert(output != NULL);
    assert(db_write(output, record, MAX_PROTO_LEVEL));
    rewind(output);
    count = fread(written, 1, sizeof(written) - 1, output);
    assert(!ferror(output));
    written[count] = '\0';
    snprintf(revision, sizeof(revision), " Revision: %lld", expected);
    assert(strstr(written, revision) != NULL);
    assert(strstr(written, "DTITLE=Artist / 月とカメレオン\n") != NULL);
    assert(strstr(written, "TTITLE0=風の向くまま\n") != NULL);
    db_free(record);
    rewind(output);
    record = db_read(output, error, DF_ENC_UTF8 | DF_ENC_LATIN1);
    assert(record != NULL && record->db_rev == expected);
    db_free(record);
    fclose(output);
}

static void test_links(void)
{
    char directory[] = "/tmp/cddbd-link-XXXXXX";
    char old[256], target[256], different[256], missing[256];
    FILE *fp;
    assert(mkdtemp(directory) != NULL);
    snprintf(old, sizeof(old), "%s/old", directory);
    snprintf(target, sizeof(target), "%s/target", directory);
    snprintf(different, sizeof(different), "%s/different", directory);
    snprintf(missing, sizeof(missing), "%s/missing", directory);
    fp = fopen(old, "w"); assert(fp != NULL); fclose(fp);
    fp = fopen(different, "w"); assert(fp != NULL); fclose(fp);
    assert(cddbd_link(old, target) == 0);
    assert(cddbd_link(old, target) == 0);
    assert(cddbd_link(old, different) != 0);
    assert(cddbd_link(missing, target) != 0);
    assert(unlink(target) == 0);
    assert(unlink(different) == 0);
    assert(unlink(old) == 0);
    assert(rmdir(directory) == 0);
}

static void test_revisions(void)
{
    db_t low = {0}, high = {0}, locked = {0};
    max_lines = DEF_MAX_LINES;
    utf_as_iso = UAI_ACCEPT;
    trklen = strlen(trkstr); hdrlen = strlen(hdrstr);
    sublen = strlen(substr); prclen = strlen(prcstr);
    for(int i = 0; i < MAX_PROTO_LEVEL; i++) proto_flags[i+1] |= proto_flags[i];
    test_revision("# Revision: 0", 0, 1);
    test_revision("# Revision: 2147483647", 2147483647LL, 1);
    test_revision("# Revision: 2147483648", 2147483648LL, 1);
    test_revision("# Revision: 2856255744", 2856255744LL, 1);
    test_revision("# Revision: 9223372036854775807", LLONG_MAX, 1);
    test_revision("# Revision: 2856255744\n# Revision: 2", 2, 1);
    test_revision("# No revision supplied", 0, 1);
    test_revision("# Revision: -1", 0, 0);
    test_revision("# Revision: 9223372036854775808", 0, 0);
    test_revision("# Revision: 999999999999999999999999999999", 0, 0);
    test_revision("# Revision: 12garbage", 12, 1);
    test_revision("# Revision: 1.0", 1, 1);
    test_revision("# Revision: 1# Processed by: old cddbd", 1, 1);
    test_revision("# Revision: ", 0, 0);
    test_revision("# Revision: invalid", 0, 0);
    high.db_rev = LLONG_MAX;
    locked.db_rev = DB_MAX_REV;
    assert(db_cmp(&high, &low) > 0);
    assert(db_cmp(&low, &high) < 0);
    assert(db_cmp(&high, &high) == 0);
    assert(db_cmp(&locked, &high) > 0);
    assert(db_cmp(&high, &locked) < 0);
    assert(db_cmp(&locked, &locked) == 0);
}

int main(void)
{
    char buf[256];
    struct { char buf[5]; char guard; } small = { "", 'X' };
    db_t db = {0};
    unsigned int input[33], output[33], width, i;
    unsigned char packed[128];
    FILE *fp;
    clck_t loglock = {0};
    int saved_stderr;
    char untouched = 'Q';

    quiet = 1;
    cddbd_snprintf(buf, sizeof(buf), "%s %d %08x %ld %lu %% %.2f",
        "test", -42, 0xabcdef01U, LONG_MIN, ULONG_MAX, 1.25);
    {
        char expected[256];
        snprintf(expected, sizeof(expected), "%s %d %08x %ld %lu %% %.2f",
            "test", -42, 0xabcdef01U, LONG_MIN, ULONG_MAX, 1.25);
        assert(strcmp(buf, expected) == 0);
    }
    cddbd_snprintf(small.buf, sizeof(small.buf), "%s", "abcdef");
    assert(strcmp(small.buf, "abcd") == 0 && small.guard == 'X');
    cddbd_snprintf(small.buf, 1, "%s", "abcdef");
    assert(small.buf[0] == '\0' && small.guard == 'X');
    cddbd_snprintf(&untouched, 0, "%s", "ignored");
    cddbd_snprintf(&untouched, -1, "%s", "ignored");
    assert(untouched == 'Q');
    cddbd_snprintf(buf, sizeof(buf), "%*.*s", 6, 3, "abcdef");
    assert(strcmp(buf, "   abc") == 0);

    fp = tmpfile();
    assert(fp != NULL);
    debug = 1;
    cddbd_log_prim(fp, LOG_INFO, "no arguments\r\n");
    cddbd_log_prim(fp, LOG_INFO, "%s %d %x %.1f", "mixed", -7, 255U, 2.5);
    rewind(fp);
    assert(fgets(buf, sizeof(buf), fp) && strcmp(buf, "no arguments\n") == 0);
    assert(fgets(buf, sizeof(buf), fp) && strcmp(buf, "mixed -7 ff 2.5\n") == 0);
    fclose(fp);

    fp = tmpfile();
    assert(fp != NULL);
    saved_stderr = dup(STDERR_FILENO);
    assert(saved_stderr >= 0);
    assert(dup2(fileno(fp), STDERR_FILENO) >= 0);
    quiet = 0;
    mode = 0;
    locks[LK_LOG] = &loglock;
    cddbd_log(LOG_INFO, "%s %d %u %ld %.1f %s %d", "public", -1,
        2U, 3L, 4.5, "six", 7);
    fflush(stderr);
    assert(dup2(saved_stderr, STDERR_FILENO) >= 0);
    close(saved_stderr);
    rewind(fp);
    assert(fgets(buf, sizeof(buf), fp));
    assert(strcmp(buf, "public -1 2 3 4.5 six 7\n") == 0);
    assert(logging == 0 && loglock.lk_refcnt == 0);
    fclose(fp);
    quiet = 1;

    db.db_trks = 1;
    db.db_offset[0] = 296;
    db.db_disclen = 344;
    assert(db_gen_discid(&db) == 0x03015501U);
    db.db_trks = 2;
    db.db_offset[0] = 150;
    db.db_offset[1] = 15150;
    db.db_disclen = 402;
    assert(db_gen_discid(&db) == 0x06019002U);
    db.db_trks = 0;
    assert(db_gen_discid(&db) == 0);
    db.db_trks = 100;
    assert(db_gen_discid(&db) == 0);

    for(width = 1; width <= 31; width++) {
        unsigned int mask = (1U << width) - 1;
        for(i = 0; i < 33; i++) input[i] = (i * 1234567U) & mask;
        bit_uint2char(33, width, input, packed);
        bit_char2uint(33, width, output, packed);
        assert(memcmp(input, output, sizeof(input)) == 0);
    }
    test_revisions();
    test_links();
    puts("Formatting, logging, freedb disc-ID, bit-packing, and revision checks passed.");
    return 0;
}
