#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum Input_EventType
{
    INPUT_EVENT_FIRST     = 0,     /**< Unused (do not remove) */

    /* Application events */
    INPUT_EVENT_QUIT           = 0x100, /**< User-requested quit */

    /* Window events */
    INPUT_EVENT_WINDOW_RESIZED = 0x200,     /**< Window has been shown */

    /* Mouse events */
    INPUT_EVENT_POINTER_MOTION    = 0x400, /**< Mouse moved */
    INPUT_EVENT_POINTER_BUTTON, /**< Mouse button pressed or released */
    INPUT_EVENT_POINTER_ENTER,
    INPUT_EVENT_POINTER_EXIT,
    INPUT_EVENT_POINTER_AXIS,
    INPUT_EVENT_POINTER_FRAME,
    // SDL_EVENT_MOUSE_ADDED,             /**< A new mouse has been inserted into the system */
    // SDL_EVENT_MOUSE_REMOVED,           /**< A mouse has been removed */

    INPUT_EVENT_KEYBOARD_KEY = 0x800,
    INPUT_EVENT_KEYBOARD_MODIFIERS,
    INPUT_EVENT_KEYBOARD_ENTER,
    INPUT_EVENT_KEYBOARD_EXIT,
} Input_EventType;

#define WAYLAND_WHEEL_AXIS_UNIT 10


/**
 * Fields shared by every event
 *
 */
typedef struct Input_CommonEvent
{
    uint32_t type;        /**< Event type, shared with all events, Uint32 to cover user events which are not in the SDL_EventType enumeration */
    uint32_t reserved;
    uint64_t timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
} Input_CommonEvent;

typedef struct Input_QuitEvent
{
    Input_EventType type; /** SDL_EVENT_WINDOW_RESIZED */
    uint32_t reserved;
    uint32_t code; /* exit code */
} Input_QuitEvent;

typedef struct Input_WindowResizedEvent
{
    Input_EventType type; /** SDL_EVENT_WINDOW_RESIZED */
    uint32_t reserved;
    // Uint64 timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
    // SDL_WindowID windowID; /**< The associated window */
    double width;       /**< event dependent data */
    double height;       /**< event dependent data */
} Input_WindowResizedEvent;

/**
 * Keyboard device event structure (event.kdevice.*)
 *
 * \since This struct is available since SDL 3.1.3.
 */
// typedef struct SDL_KeyboardDeviceEvent
// {
//     SDL_EventType type; /**< SDL_EVENT_KEYBOARD_ADDED or SDL_EVENT_KEYBOARD_REMOVED */
//     Uint32 reserved;
//     Uint64 timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
//     SDL_KeyboardID which;   /**< The keyboard instance id */
// } SDL_KeyboardDeviceEvent;

/**
 * Mouse motion event structure (event.motion.*)
 */
typedef struct Input_PointerMotionEvent
{
    Input_EventType type; /**< INPUT_EVENT_POINTER_MOTION */
    uint32_t reserved;
    uint32_t time;   /**< In milliseconds, relative to an undefined base */
    double x;            /**< X coordinate, relative to window */
    double y;            /**< Y coordinate, relative to window */
} Input_PointerMotionEvent;

typedef struct Input_PointerButtonEvent
{
    Input_EventType type; /**< INPUT_EVENT_POINTER_BUTTON */
    uint32_t reserved;
    uint32_t time;   /**< In milliseconds, relative to an undefined base */
    uint32_t serial; /**< Wayland serial for this event (needed for drag initiation) */
    uint32_t button; /**< Button code (BTN_LEFT, BTN_RIGHT, etc.) */
    bool pressed;    /**< true if pressed, false if released */
} Input_PointerButtonEvent;

typedef struct Input_PointerAxisEvent
{
    Input_EventType type;
      uint32_t reserved;
      uint32_t time;
      uint32_t axis;
    float value;
} Input_PointerAxisEvent;

typedef struct Input_PointerFrameEvent
{
    Input_EventType type; /**< INPUT_EVENT_POINTER_MOTION */
    uint32_t reserved;
} Input_PointerFrameEvent;

typedef struct Input_PointerEnterEvent
{
    Input_EventType type; /**< INPUT_EVENT_POINTER_MOTION */
    uint32_t reserved;
    double x;            /**< X coordinate, relative to window */
    double y;            /**< Y coordinate, relative to window */
    uint32_t serial;
} Input_PointerEnterEvent;

typedef struct Input_PointerExitEvent
{
    Input_EventType type; /**< INPUT_EVENT_POINTER_MOTION */
    uint32_t reserved;
} Input_PointerExitEvent;

typedef struct Input_KeyboardKeyEvent
{
    Input_EventType type; /**< INPUT_EVENT_KEYBOARD_KEY */
    uint32_t reserved;
    uint32_t key; // XKB scancode
    uint32_t unicode; // XKB UTF-32 code point if valid
    bool pressed;
} Input_KeyboardKeyEvent;

typedef struct Input_KeyboardModifiersEvent
{
    Input_EventType type; /**< INPUT_EVENT_KEYBOARD_MODIFIERS */
    uint32_t reserved;
    uint32_t depressed;
    uint32_t latched;
    uint32_t locked;
    uint32_t group;
} Input_KeyboardModifiersEvent;

typedef struct Input_KeyboardEnterEvent
{
    Input_EventType type; /**< INPUT_EVENT_KEYBOARD_KEY */
    uint32_t reserved;
} Input_KeyboardEnterEvent;

typedef struct Input_KeyboardExitEvent
{
    Input_EventType type; /**< INPUT_EVENT_KEYBOARD_KEY */
    uint32_t reserved;
} Input_KeyboardExitEvent;

// typedef struct SDL_UserEvent
// {
//     Uint32 type;        /**< SDL_EVENT_USER through SDL_EVENT_LAST-1, Uint32 because these are not in the SDL_EventType enumeration */
//     Uint32 reserved;
//     Uint64 timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
//     SDL_WindowID windowID; /**< The associated window if any */
//     Sint32 code;        /**< User defined event code */
//     void *data1;        /**< User defined data pointer */
//     void *data2;        /**< User defined data pointer */
// } SDL_UserEvent;

typedef union Input_Event
{
    Input_EventType type;                            /**< Event type, shared with all events, Uint32 to cover user events which are not in the Input_EventType enumeration */
    Input_CommonEvent common;
    Input_QuitEvent quit;
    Input_WindowResizedEvent window_resized;
    Input_PointerMotionEvent pointer_motion;
    Input_PointerButtonEvent pointer_button;
    Input_PointerAxisEvent pointer_axis;
    Input_PointerFrameEvent pointer_frame;
    Input_PointerEnterEvent pointer_enter;
    Input_PointerExitEvent pointer_exit;
    Input_KeyboardKeyEvent keyboard_key;
    Input_KeyboardModifiersEvent keyboard_modifiers;
    Input_KeyboardEnterEvent keyboard_enter;
    Input_KeyboardExitEvent keyboard_exit;
} Input_Event;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

