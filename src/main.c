#include "e/simple.h"
#include "r/r.h"
#include "u/pose.h"
#include "u/animator.h"
#include "u/button.h"
#include "rhc/time.h"
#include "mathc/utils/random.h"
#include "mathc/utils/color.h"

#include "camera.h"
#include "sound.h"

#define TEAS 3
#define TEA_TIMES (int[]){2*60, 3*60, 5*60}
#define TEA_TIME_TEXTS (const char*[]){"2:00", "3:00", "5:00"}
#define TEA_ARRAY_LEFT -32
#define TEA_ARRAY_OFFSET 32

static struct {
    RoText info;
    RoBatch teas;
    RoBatch btns;
    RoText btn_texts[TEAS];
    double tea_time;
    uAnimator_s tea_animator;
    bool alarm_played;

    // rhc timer: high precision, based on monotonic time
    // u timer: based on frame time, if the app sleeps, that timer sleeps, too!
    RhcTimer_s timer;
} L;


static void pointer_event(ePointer_s pointer, void *ud) {
    if(L.tea_time>0)
        return;

    pointer.pos = mat4_mul_vec(camera.matrices.p_inv, pointer.pos);

    for(int i=0; i<TEAS; i++) {
        if (u_button_clicked(&L.btns.rects[i], pointer)) {
            L.timer = rhc_timer_new();
            L.tea_time = TEA_TIMES[i];

            for(int t=0; t<TEAS; t++) {
                L.teas.rects[t].pose = u_pose_new_hidden();
            }

            L.teas.rects[i].pose = u_pose_new(0, -16, 64, 64);
        }
    }
}


// this function will be called at the start of the app
static void init() {
    log_info("init");
    // random

    // init systems
    camera_init();
    sound_init();

    // do not allow pause to let the timer run on lost focus
    // default is true (in most apps / games its easier to ignore updates on pause)
    e_window.allow_pause = false;

    // random background color via hue
    //      seed rand
    srand(time(NULL));
    //      random hue value
    vec3 hsv = {{sca_random()*360, 0.66, 0.33}};
    r_render.clear_color.rgb = vec3_hsv2rgb(hsv);

    e_input_register_pointer_event(pointer_event, NULL);

    L.info = ro_text_new_font55(32);
    ro_text_set_text(&L.info, "Tea");
    L.info.pose = u_pose_new(-16, 48, 2, 2);

    L.tea_animator = u_animator_new_fps(4, 3);

    L.teas = ro_batch_new(TEAS, r_texture_new_file(4, 4, "res/tea.png"));
    L.btns = ro_batch_new(TEAS, r_texture_new_file(2, 1, "res/btn.png"));
    for(int i=0; i<TEAS; i++) {
        L.btn_texts[i] = ro_text_new_font55(8);
        ro_text_set_text(&L.btn_texts[i], TEA_TIME_TEXTS[i]);
        ro_text_set_color(&L.btn_texts[i], R_COLOR_BLACK);

        L.teas.rects[i].sprite.y = i;
        L.teas.rects[i].pose = u_pose_new(TEA_ARRAY_LEFT+TEA_ARRAY_OFFSET*i, 0, 32, 32);
        L.btns.rects[i].pose = u_pose_new(TEA_ARRAY_LEFT+TEA_ARRAY_OFFSET*i, -32, 32, 16);
    }

}


// this functions is called either each frame or at a specific update/s info
static void update(float dtime) {
    // simulate
    camera_update();

    if(L.tea_time>0) {
        double t = L.tea_time - rhc_timer_elapsed(L.timer);
        char buf[32];
        if (t <= 0) {
            snprintf(buf, sizeof buf, "The tea\n  is ready");
            L.info.pose = u_pose_new(-64, 48, 2, 2);
            if(!L.alarm_played) {
                L.alarm_played = true;
                sound_play_alarm();
            }
        } else {
            int minutes = (int) t / 60;
            int seconds = (int) t % 60;
            snprintf(buf, sizeof buf, "%2i:%02i", minutes, seconds);
            L.info.pose = u_pose_new(-30, 32, 2, 2);
        }

        ro_text_set_text(&L.info, buf);

    }

    for(int i=0; i<TEAS; i++) {
        L.teas.rects[i].sprite.x = u_animator_frame(L.tea_animator);

        // sprite.x=0: offset -0
        // sprite.x=1: offset -2
        L.btn_texts[i].pose = u_pose_new(TEA_ARRAY_LEFT+TEA_ARRAY_OFFSET*i-11,
                                         -32+3 - L.btns.rects[i].sprite.x*2,
                                         1, 1);
    }
}


// this function is called each frame to render stuff, dtime is the info between frames
static void render(float dtime) {
    mat4 *camera_mat = &camera.matrices.vp;

    ro_text_render(&L.info, camera_mat);
    ro_batch_render(&L.teas, camera_mat, true);

    if(L.tea_time<=0) {
        ro_batch_render(&L.btns, camera_mat, true);
        for(int i=0; i<TEAS; i++) {
            ro_text_render(&L.btn_texts[i], camera_mat);
        }
    }
}


int main(int argc, char **argv) {
    e_simple_start("tea", "Horsimann",
                   1.0f,   // startup block info (the info in which "Horsimann" is displayed at startup)
                   0,      // update deltatime_ms, <=0 to turn off and use fps (5=200hz)
                   init, update, render);
    return 0;
}
