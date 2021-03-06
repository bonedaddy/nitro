#include "test.h"

#include "frame.h"

typedef struct frame_state {
    int done;
} frame_state;

void my_free(void *reg, void *st) {
    frame_state *s = (frame_state *)st;
    s->done = 1;

    free(reg);
}

int main(int argc, char **argv) {

    char foo[8];

    strcpy(foo, "yope");

    nitro_frame_t *fr = nitro_frame_new_copy(
    foo, 5);

    TEST("_new_copy returns non-null", fr);

    TEST("_new_copy right length",
    nitro_frame_size(fr) == 5);
    TEST("_new_copy copied correct",
    !strcmp(nitro_frame_data(fr), foo));

    foo[0] = 'n';

    TEST("_new_copy mutate orig",
    strcmp(nitro_frame_data(fr), foo));
    TEST("_new_copy mutate unchange",
    !strcmp(nitro_frame_data(fr), "yope"));

    nitro_frame_t *fr2 = nitro_frame_copy_partial(fr, NULL);

    TEST("_copy frames are different",
    fr2 != fr);
    TEST("_copy data regions are the same",
    nitro_frame_data(fr2) == nitro_frame_data(fr));

    nitro_frame_destroy(fr);
    TEST("_copy data good after destroy",
    !strcmp(nitro_frame_data(fr2), "yope"));
    nitro_frame_destroy(fr2);


    char *reg = malloc(5);
    strcpy(reg, "foo");

    frame_state fst = {0};

    fr = nitro_frame_new(reg, 4,
    my_free, &fst);

    fr2 = nitro_frame_copy_partial(fr, NULL);

    TEST("custom free unmarked 1",
    !fst.done);

    nitro_frame_destroy(fr);

    TEST("custom free unmarked 2",
    !fst.done);

    nitro_frame_destroy(fr2);

    TEST("custom free marked",
    fst.done);

    char *big_output =
    "this is a big frame of data we're going to send";

    int big_length = strlen(big_output) + 1;

    fr = nitro_frame_new_copy(
    big_output, big_length);

    int num;
    struct iovec *ios = nitro_frame_iovs(fr, &num);
    TEST("iovs, initially two", num == 2);

    // test 1.  let's "consume" part of first
    int s1 = ios[0].iov_len;
    uint8_t *p1 = ios[0].iov_base;
    int done;

    int r = nitro_frame_iovs_advance(fr, fr->iovs, 0, 3, &done);
    
    ios = nitro_frame_iovs(fr, &num);
    int s2 = ios[0].iov_len;
    uint8_t *p2 = ios[0].iov_base;
    TEST("iovs advance (1) size",
    s2 == (s1 - 3));
    TEST("iovs advance (1) ptrs",
    p2 - p1 == 3);

    // test 2. more of first
    r = nitro_frame_iovs_advance(fr, fr->iovs, 0, 3, &done);
    ios = nitro_frame_iovs(fr, &num);
    s2 = ios[0].iov_len;
    p2 = ios[0].iov_base;
    TEST("iovs advance (2) size",
    s2 == (s1 - 6));
    TEST("iovs advance (2) ptrs",
    p2 - p1 == 6);


    // test 3. start of second
    r = nitro_frame_iovs_advance(fr, fr->iovs, 0, 10, &done);
    TEST("iovs advance (3) only took two, !done",
    !done && (r == 2));
    r = nitro_frame_iovs_advance(fr, fr->iovs, 1, 0, &done);
    ios = nitro_frame_iovs(fr, &num);
    TEST("iovs advance (3) still two iov",
    num == 2)
    s1 = ios[1].iov_len;
    p1 = ios[1].iov_base;

    r = nitro_frame_iovs_advance(fr, fr->iovs, 1, 4, &done);
    ios = nitro_frame_iovs(fr, &num);
    s2 = ios[1].iov_len;
    p2 = ios[1].iov_base;
    TEST("iovs advance (4) size",
    s2 == (s1 - 4));
    TEST("iovs advance (4) ptrs",
    p2 - p1 == 4);
    TEST("iovs advance (4) empty iov1 size",
    ios[0].iov_len == 0);
    
    r = nitro_frame_iovs_advance(fr, fr->iovs, 1, 40000, &done);
    TEST("iovs advance final clip test",
    (r == big_length - 4) && done);

    nitro_frame_iovs_reset(fr);
    ios = nitro_frame_iovs(fr, &num);
    TEST("iovs reset, iov1 size back",
    ios[0].iov_len > 0);


    nitro_frame_destroy(fr);

    SUMMARY(0);
    return 1;
}
