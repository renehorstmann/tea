#include <SDL2/SDL_mixer.h>
#include "e/input.h"
#include "mathc/sca/float.h"
#include "sound.h"

static struct {
    Mix_Chunk *alarm;

    bool active;
} L;


static void init() {

    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1) {
        log_warn("sound not working");
        return;;
    }

    L.alarm = Mix_LoadWAV("res/sound_alarm.wav");
    if (!L.alarm) {
        log_warn("failed to load activate");
        return;
    }
    log_info("sound activated");
    L.active = true;
}

static void pointer_event(ePointer_s pointer, void *user_data) {
    // wait for first user pointer action

    // init SDL_Mixer
    // on web, sound will be muted, if created before an user action....
    init();

    e_input_unregister_pointer_event(pointer_event);
}


void sound_init() {
    e_input_register_pointer_event(pointer_event, NULL);
}


void sound_play_alarm() {
    if (!L.active)
        return;
    if (Mix_PlayChannel(-1, L.alarm, 0) == -1) {
        log_warn("failed to play");
        return;
    }
}
