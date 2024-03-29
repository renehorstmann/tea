#include "mathc/float.h"
#include "e/window.h"
#include "e/gui_nk.h"
#include "e/gui.h"
#include "e/input.h"

struct eInput_Globals e_input;

//
// protected
//

void e_window_handle_window_event_(const SDL_Event *event);

void e_gui_input_begin_();

void e_gui_handle_sdl_event_(SDL_Event *event);

void e_gui_input_end_();

typedef struct {
    SDL_FingerID id;
    bool active;
} TouchID;

typedef struct {
    ePointerEventFn cb;
    void *ud;
} RegPointer;

typedef struct {
    RegPointer array[E_MAX_POINTER_EVENTS];
    int size;
} RegPointerArray;


typedef struct {
    eWheelEventFn cb;
    void *ud;
} RegWheel;

typedef struct {
    RegWheel array[E_MAX_WHEEL_EVENTS];
    int size;
} RegWheelArray;


typedef struct {
    eKeyRawEventFn cb;
    void *ud;
} RegKeyRaw;

typedef struct {
    RegKeyRaw array[E_MAX_KEY_RAW_EVENTS];
    int size;
} RegKeyRawArray;


static struct {
    eInputKeys keys;

    TouchID touch_ids[E_MAX_TOUCH_IDS];
    int touch_ids_size;

    RegPointerArray reg_pointer;
    RegPointer reg_pointer_e_vip;  // .cb==NULL if none

    RegWheelArray reg_wheel;
    RegWheel reg_wheel_e_vip;  // .cb==NULL if none

    RegKeyRawArray reg_key_raw;
    RegKeyRaw reg_key_raw_e_vip;  // .cb==NULL if none
} L;


static ePointer_s pointer_mouse(enum ePointerAction action, int btn_id) {
    ePointer_s res;
    res.action = action;
    res.id = btn_id;
    int x, y;
    SDL_GetMouseState(&x, &y);

    res.pos.x = (2.0f * x) / e_window.size.x - 1.0f;
    res.pos.y = 1.0f - (2.0f * y) / e_window.size.y;
    res.pos.z = 0;
    res.pos.w = 1;

    return res;
}

static ePointer_s pointer_finger(enum ePointerAction action, float x, float y, SDL_FingerID finger_id) {

    // check finger ids to cast to pointer id
    int id = -1;
    for (int i = 0; i < L.touch_ids_size; i++) {
        if (L.touch_ids[i].id == finger_id) {
            id = i;
            break;
        }
    }
    if (id < 0) {
        if (L.touch_ids_size >= E_MAX_TOUCH_IDS) {
            log_warn("e_input_update: to many touch ids, reset to 0!");
            L.touch_ids_size = 0;
        }

        L.touch_ids[L.touch_ids_size].id = finger_id;
        L.touch_ids[L.touch_ids_size].active = true;
        id = L.touch_ids_size++;

        if (action == E_POINTER_MOVE) {
            action = E_POINTER_DOWN;
            log_warn("e_input_update: touch got new id but action==move");
        }
        if (action == E_POINTER_UP) {
            log_error("e_input_update: touch got up but unknown id");
            id--;
            L.touch_ids_size--;
        }
    }

    if (action == E_POINTER_UP) {
        L.touch_ids[id].active = false;
        if (id + 1 >= L.touch_ids_size) {
            while (L.touch_ids_size > 0
                   && !L.touch_ids[L.touch_ids_size - 1].active) {

                L.touch_ids_size--;
            }
        }
    }


    ePointer_s res;
    res.action = action;
    res.id = id;

    res.pos.x = 2.0f * x - 1.0f;
    res.pos.y = 1.0f - 2.0f * y;
    res.pos.z = 0;
    res.pos.w = 1;

    return res;
}

static void emit_pointer_events(ePointer_s action) {
    if (L.reg_pointer_e_vip.cb) {
        L.reg_pointer_e_vip.cb(action, L.reg_pointer_e_vip.ud);
        return;
    }

    // copy to be safe to unregister in an event
    RegPointerArray array = L.reg_pointer;
    for (int i = 0; i < array.size; i++)
        array.array[i].cb(action, array.array[i].ud);
}

static void emit_wheel_events(bool up) {
    if (L.reg_wheel_e_vip.cb) {
        L.reg_wheel_e_vip.cb(up, L.reg_wheel_e_vip.ud);
        return;
    }

    // copy to be safe to unregister in an event
    RegWheelArray array = L.reg_wheel;
    for (int i = 0; i < array.size; i++)
        array.array[i].cb(up, array.array[i].ud);
}

static void input_handle_pointer_touch(SDL_Event *event) {
    switch (event->type) {
        case SDL_FINGERDOWN:
            emit_pointer_events(pointer_finger(E_POINTER_DOWN,
                                               event->tfinger.x, event->tfinger.y, event->tfinger.fingerId));
            break;
        case SDL_FINGERMOTION:
            emit_pointer_events(pointer_finger(E_POINTER_MOVE,
                                               event->tfinger.x, event->tfinger.y, event->tfinger.fingerId));
            break;
        case SDL_FINGERUP:
            emit_pointer_events(pointer_finger(E_POINTER_UP,
                                               event->tfinger.x, event->tfinger.y, event->tfinger.fingerId));
            break;
    }
}

static void input_handle_pointer_mouse(SDL_Event *event) {
    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button <= 0 || event->button.button > 3)
                break;
            emit_pointer_events(pointer_mouse(
                    E_POINTER_DOWN,
                    1 - event->button.button));
            break;
        case SDL_MOUSEMOTION:
            emit_pointer_events(pointer_mouse(E_POINTER_MOVE, 0));
            break;
        case SDL_MOUSEBUTTONUP:
            if (event->button.button <= 0 || event->button.button > 3)
                break;
            emit_pointer_events(pointer_mouse(
                    E_POINTER_UP,
                    1 - event->button.button));
            break;
    }
}

static void input_handle_wheel(SDL_Event *event) {
    // it could be possible that y==0, (e. g. x!=0)
    if (event->wheel.y == 0)
        return;
    emit_wheel_events(event->wheel.y > 0);
}

static void input_handle_keys(SDL_Event *event) {
    if (L.reg_key_raw_e_vip.cb) {
        L.reg_key_raw_e_vip.cb(event, L.reg_key_raw_e_vip.ud);
        return;
    }

    // copy to be safe to unregister ik an event
    RegKeyRawArray array = L.reg_key_raw;
    for (int i = 0; array.size; i++) {
        array.array[i].cb(event, array.array[i].ud);
    }
    bool down = event->type == SDL_KEYDOWN;
    switch (event->key.keysym.sym) {
        case SDLK_UP:
            L.keys.up = down;
            break;
        case SDLK_LEFT:
            L.keys.left = down;
            break;
        case SDLK_RIGHT:
            L.keys.right = down;
            break;
        case SDLK_DOWN:
            L.keys.down = down;
            break;
        case SDLK_RETURN:
            L.keys.enter = down;
            break;
        case SDLK_SPACE:
            L.keys.space = down;
            break;
    }
}

#ifdef OPTION_GYRO
static void input_handle_sensors(SDL_Event *event) {
    SDL_Sensor *sensor = SDL_SensorFromInstanceID(event->sensor.which);
    if (!sensor || SDL_SensorGetType(sensor) != SDL_SENSOR_ACCEL) {
        log_warn("e_input_update: Couldn't get sensor for sensor event\n");
        return;
    }

    const float *data = event->sensor.data;
    memcpy(e_input.accel.v, data, sizeof(e_input.accel));

    // log_trace("e_input_update: Gyro update: %.2f, %.2f, %.2f", data[0], data[1], data[2]);
}
#endif



//
// public
//

void e_input_init() {
    assume(!e_input.init, "should be called only once");
    e_input.init = true;

    log_info("init");

    assume(e_window.init, "needs an sdl window to get its size");

#ifdef OPTION_GYRO
    int num_sensors = SDL_NumSensors();
    bool accel_opened = false;
    for (int i = 0; i < num_sensors; i++) {
        if (SDL_SensorGetDeviceType(i) == SDL_SENSOR_ACCEL) {
            SDL_Sensor *sensor = SDL_SensorOpen(i);
            if (sensor) {
                accel_opened = true;
                break;
            }
        }
    }

    e_input.accel_active = accel_opened;
    if (accel_opened)
        log_info("e_input_init: Opened acceleration sensor");
#endif
}

void e_input_kill() {
    if (!e_input.init)
        return;

    memset(&e_input, 0, sizeof e_input);
    memset(&L, 0, sizeof L);
}


void e_input_update() {
    e_input.is_touch = SDL_GetNumTouchDevices() > 0;

    e_gui_input_begin_();

    void (*input_handle_pointer)(SDL_Event *event) =
    e_input.is_touch ? input_handle_pointer_touch : input_handle_pointer_mouse;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        e_window_handle_window_event_(&event);

        e_gui_handle_sdl_event_(&event);   // NULL safe

        switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEMOTION:
            case SDL_MOUSEBUTTONUP:
            case SDL_FINGERDOWN:
            case SDL_FINGERMOTION:
            case SDL_FINGERUP:
                input_handle_pointer(&event);
                break;
            case SDL_MOUSEWHEEL:
                input_handle_wheel(&event);
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                input_handle_keys(&event);
                break;
#ifdef OPTION_GYRO
                case SDL_SENSORUPDATE:
                    input_handle_sensors(&event);
                    break;
#endif
        }
    }

    e_gui_input_end_();
}


void e_input_register_pointer_event(ePointerEventFn event, void *user_data) {
    assume(L.reg_pointer.size < E_MAX_POINTER_EVENTS, "too many registered pointer events");
    L.reg_pointer.array[L.reg_pointer.size++] = (RegPointer) {event, user_data};
}

void e_input_unregister_pointer_event(ePointerEventFn event_to_unregister) {
    bool found = false;
    for (int i = 0; i < L.reg_pointer.size; i++) {
        if (L.reg_pointer.array[i].cb == event_to_unregister) {
            found = true;
            // move to close hole
            for (int j = i; j < L.reg_pointer.size - 1; j++) {
                L.reg_pointer.array[j] = L.reg_pointer.array[j + 1];
            }
            L.reg_pointer.size--;
            i--; // check moved
        }
    }
    if (!found) {
        log_warn("failed: event not registered");
    }
}

void e_input_set_vip_pointer_event(ePointerEventFn event, void *user_data) {
    L.reg_pointer_e_vip.cb = event;
    L.reg_pointer_e_vip.ud = user_data;
}

void e_input_register_wheel_event(eWheelEventFn event, void *user_data) {
    assume(L.reg_wheel.size < E_MAX_WHEEL_EVENTS, "too many registered wheel events");
    L.reg_wheel.array[L.reg_wheel.size++] = (RegWheel) {event, user_data};
}

void e_input_unregister_wheel_event(eWheelEventFn event_to_unregister) {
    bool found = false;
    for (int i = 0; i < L.reg_wheel.size; i++) {
        if (L.reg_wheel.array[i].cb == event_to_unregister) {
            found = true;
            // move to close hole
            for (int j = i; j < L.reg_wheel.size - 1; j++) {
                L.reg_wheel.array[j] = L.reg_wheel.array[j + 1];
            }
            L.reg_wheel.size--;
            i--; // check moved
        }
    }
    if (!found) {
        log_warn("failed: event not registered");
    }
}

void e_input_set_vip_wheel_event(eWheelEventFn event, void *user_data) {
    L.reg_wheel_e_vip.cb = event;
    L.reg_wheel_e_vip.ud = user_data;
}

void e_input_register_key_raw_event(eKeyRawEventFn event, void *user_data) {
    assume(L.reg_key_raw.size < E_MAX_KEY_RAW_EVENTS, "too many registered key raw events");
    L.reg_key_raw.array[L.reg_key_raw.size++] = (RegKeyRaw) {event, user_data};
}

void e_input_unregister_key_raw_event(eKeyRawEventFn event_to_unregister) {
    bool found = false;
    for (int i = 0; i < L.reg_key_raw.size; i++) {
        if (L.reg_key_raw.array[i].cb == event_to_unregister) {
            found = true;
            // move to close hole
            for (int j = i; j < L.reg_key_raw.size - 1; j++) {
                L.reg_key_raw.array[j] = L.reg_key_raw.array[j + 1];
            }
            L.reg_key_raw.size--;
            i--; // check moved
        }
    }
    if (!found) {
        log_warn("failed: event not registered");
    }
}

void e_input_set_vip_key_raw_event(eKeyRawEventFn event, void *user_data) {
    L.reg_key_raw_e_vip.cb = event;
    L.reg_key_raw_e_vip.ud = user_data;
}
