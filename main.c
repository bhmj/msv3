#include <stdio.h>
#include <X11/Xlib.h>
#include <linux/input.h>
#include <alsa/asoundlib.h>

#define STEP_PRC 8

void volume_change(int delta) {
    long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";
    int err;

    err = snd_mixer_open(&handle, 0);
    if (err>0) {
        printf("err: snd_mixer_open");
        return;
    }
    err = snd_mixer_attach(handle, card);
    if (err>0) {
        printf("err: snd_mixer_attach");
        return;
    }
    err = snd_mixer_selem_register(handle, NULL, NULL);
    if (err>0) {
        printf("err: snd_mixer_selem_register");
        return;
    }
    err = snd_mixer_load(handle);
    if (err>0) {
        printf("err: snd_mixer_load");
        return;
    }

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
    fprintf(stdout,"elem=%p\n", elem);

    long vol;
    snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    fprintf(stdout, "min: %ld, max: %ld, vol: %ld\n", min, max, vol);
    vol += delta * STEP_PRC*(max-min) / 100;
    if (vol < min) vol = min;
    if (vol > max) vol = max;

    snd_mixer_selem_set_playback_volume_all(elem, vol);

    snd_mixer_close(handle);
}

int screen_boundaries(void) {
    int number_of_screens;
    int i;
    Bool result;
    Window *root_windows;
    Window window_returned;
    int root_x, root_y;
    int win_x, win_y;
    unsigned int mask_return;

    Display *display = XOpenDisplay(NULL);
    Screen*  s = DefaultScreenOfDisplay(display);
    assert(display);
    //XSetErrorHandler(_XlibErrorHandler);
    number_of_screens = XScreenCount(display);
    root_windows = malloc(sizeof(Window) * number_of_screens);
    for (i = 0; i < number_of_screens; i++) {
        root_windows[i] = XRootWindow(display, i);
    }
    int max_x, max_y;
    for (i = 0; i < number_of_screens; i++) {
        result = XQueryPointer(display, root_windows[i], &window_returned,
                &window_returned, &root_x, &root_y, &win_x, &win_y,
                &mask_return);
        if (result == True) {
            max_x = s->width;
            max_y = s->height;
            break;
        }
    }
    if (result != True) {
        fprintf(stderr, "No mouse found.\n");
        return -1;
    }
    free(root_windows);
    XCloseDisplay(display);
    
    return root_x==0 || root_x == max_x-1 || root_y==0 || root_y == max_y-1;
}

// Config:
//   - event file (TODO: determine automatically)
//   - volume OSD position
//   - volume step
//   - additional key combinations
//   - app list to treat volume wheel directly

int main(int argc, char **argv)
{
    int fid = open(argv[1], O_RDONLY);
    if (fid == 0) {
        fprintf(stderr, "Could not open %s device\n", argv[1]);
        return 1;
    }
    fprintf(stdout, "Opened %s device\n", argv[1]);

    int nbytes;
    struct input_event event;
    while(1) {
        nbytes = read(fid, &event, sizeof(event));
        if (event.code == REL_WHEEL) {
            if (screen_boundaries()) {
                volume_change(event.value);
            }
        }
    }
    return 0;
}
