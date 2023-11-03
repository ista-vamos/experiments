

#include <assert.h>
#include <immintrin.h> /* _mm_pause */
#include <limits.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <float.h>

#include "vamos-buffers/core/arbiter.h"
#include "vamos-buffers/core/monitor.h"
#include "vamos-buffers/core/utils.h"
#include "vamos-buffers/core/vector-macro.h"
#include "vamos-buffers/streams/streams.h"

#include "compiler/cfiles/compiler_utils.h"

#define __vamos_min(a, b) ((a < b) ? (a) : (b))
#define __vamos_max(a, b) ((a > b) ? (a) : (b))

#ifdef NDEBUG
#define vamos_check(cond)
#define vamos_assert(cond)
#else
#define vamos_check(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "[31m%s:%s:%d: check '" #cond "' failed![0m\n",        \
              __FILE__, __func__, __LINE__);                                   \
      print_buffers_state();                                                   \
    }                                                                          \
  } while (0)
#define vamos_assert(cond)                                                     \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "[31m%s:%s:%d: assert '" #cond "' failed![0m\n",       \
              __FILE__, __func__, __LINE__);                                   \
      print_buffers_state();                                                   \
      __work_done = 1;                                                         \
    }                                                                          \
  } while (0)
#endif

#define vamos_hard_assert(cond)                                                \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "[31m%s:%s:%d: assert '" #cond "' failed![0m\n",       \
              __FILE__, __func__, __LINE__);                                   \
      print_buffers_state();                                                   \
      __work_done = 1;                                                         \
      abort();                                                                 \
    }                                                                          \
  } while (0)

struct _EVENT_hole {
  uint64_t n;
};
typedef struct _EVENT_hole EVENT_hole;

struct _EVENT_hole_wrapper {
  vms_event head;
  union {
    EVENT_hole hole;
  } cases;
};

static void init_hole_hole(vms_event *hev) {
  struct _EVENT_hole_wrapper *h = (struct _EVENT_hole_wrapper *)hev;
  h->head.kind = vms_event_get_hole_kind();
  h->cases.hole.n = 0;
}

static void update_hole_hole(vms_event *hev, vms_event *ev) {
  (void)ev;
  struct _EVENT_hole_wrapper *h = (struct _EVENT_hole_wrapper *)hev;
  ++h->cases.hole.n;
}

// declares the structs of stream types needed for event sources
enum WaylandSource_kinds {
  WAYLANDSOURCE_CLIENT_NEW = VMS_EVENT_LAST_SPECIAL_KIND + 1,
  WAYLANDSOURCE_CLIENT_EXIT = VMS_EVENT_LAST_SPECIAL_KIND + 2,
  WAYLANDSOURCE_HOLE = VMS_EVENT_HOLE_KIND,
};

enum WaylandConnection_kinds {
  WAYLANDCONNECTION_POINTER_MOTION = VMS_EVENT_LAST_SPECIAL_KIND + 1,
  WAYLANDCONNECTION_POINTER_BUTTON = VMS_EVENT_LAST_SPECIAL_KIND + 2,
  WAYLANDCONNECTION_KEYBOARD_KEY = VMS_EVENT_LAST_SPECIAL_KIND + 3,
  WAYLANDCONNECTION_HOLE = VMS_EVENT_HOLE_KIND,
};

enum LibinputSource_kinds {
  LIBINPUTSOURCE_POINTER_MOTION = VMS_EVENT_LAST_SPECIAL_KIND + 1,
  LIBINPUTSOURCE_POINTER_BUTTON = VMS_EVENT_LAST_SPECIAL_KIND + 2,
  LIBINPUTSOURCE_KEYBOARD_KEY = VMS_EVENT_LAST_SPECIAL_KIND + 3,
  LIBINPUTSOURCE_HOLE = VMS_EVENT_HOLE_KIND,
};

enum GlobalEvent_kinds {
  GLOBALEVENT_TS = VMS_EVENT_LAST_SPECIAL_KIND + 1,
  GLOBALEVENT_HOLE = VMS_EVENT_HOLE_KIND,
};

// declare hole structs

// declare stream types structs
// event declarations for stream type WaylandSource
struct _EVENT_WaylandSource_client_new {
  int pid;
};
typedef struct _EVENT_WaylandSource_client_new EVENT_WaylandSource_client_new;
struct _EVENT_WaylandSource_client_exit {
  int pid;
};
typedef struct _EVENT_WaylandSource_client_exit EVENT_WaylandSource_client_exit;

// input stream for stream type WaylandSource
struct _STREAM_WaylandSource_in {
  vms_event head;
  union {
    EVENT_WaylandSource_client_exit client_exit;
    EVENT_WaylandSource_client_new client_new;
  } cases;
};
typedef struct _STREAM_WaylandSource_in STREAM_WaylandSource_in;

// output stream for stream processor WaylandSource
struct _STREAM_WaylandSource_out {
  vms_event head;
  union {
    EVENT_hole hole;
    EVENT_WaylandSource_client_exit client_exit;
    EVENT_WaylandSource_client_new client_new;
  } cases;
};
typedef struct _STREAM_WaylandSource_out STREAM_WaylandSource_out;
// event declarations for stream type WaylandConnection
struct _EVENT_WaylandConnection_pointer_motion {
  double time;
  uint32_t way_time;
  uint32_t x;
  uint32_t y;
};
typedef struct _EVENT_WaylandConnection_pointer_motion
    EVENT_WaylandConnection_pointer_motion;
struct _EVENT_WaylandConnection_pointer_button {
  double time;
  uint32_t serial;
  uint32_t way_time;
  uint32_t button;
  uint32_t pressed;
};
typedef struct _EVENT_WaylandConnection_pointer_button
    EVENT_WaylandConnection_pointer_button;
struct _EVENT_WaylandConnection_keyboard_key {
  double time;
  uint32_t serial;
  uint32_t way_time;
  uint32_t key;
  uint8_t pressed;
};
typedef struct _EVENT_WaylandConnection_keyboard_key
    EVENT_WaylandConnection_keyboard_key;

// input stream for stream type WaylandConnection
struct _STREAM_WaylandConnection_in {
  vms_event head;
  union {
    EVENT_WaylandConnection_pointer_motion pointer_motion;
    EVENT_WaylandConnection_keyboard_key keyboard_key;
    EVENT_WaylandConnection_pointer_button pointer_button;
  } cases;
};
typedef struct _STREAM_WaylandConnection_in STREAM_WaylandConnection_in;

// output stream for stream processor WaylandConnection
struct _STREAM_WaylandConnection_out {
  vms_event head;
  union {
    EVENT_hole hole;
    EVENT_WaylandConnection_pointer_motion pointer_motion;
    EVENT_WaylandConnection_keyboard_key keyboard_key;
    EVENT_WaylandConnection_pointer_button pointer_button;
  } cases;
};
typedef struct _STREAM_WaylandConnection_out STREAM_WaylandConnection_out;
// event declarations for stream type LibinputSource
struct _EVENT_LibinputSource_pointer_motion {
  double time;
  double dx;
  double dy;
  double unacc_dx;
  double unacc_dy;
};
typedef struct _EVENT_LibinputSource_pointer_motion
    EVENT_LibinputSource_pointer_motion;
struct _EVENT_LibinputSource_pointer_button {
  double time;
  uint32_t button;
  uint8_t pressed;
};
typedef struct _EVENT_LibinputSource_pointer_button
    EVENT_LibinputSource_pointer_button;
struct _EVENT_LibinputSource_keyboard_key {
  double time;
  uint32_t key;
  uint8_t pressed;
};
typedef struct _EVENT_LibinputSource_keyboard_key
    EVENT_LibinputSource_keyboard_key;

// input stream for stream type LibinputSource
struct _STREAM_LibinputSource_in {
  vms_event head;
  union {
    EVENT_LibinputSource_pointer_motion pointer_motion;
    EVENT_LibinputSource_keyboard_key keyboard_key;
    EVENT_LibinputSource_pointer_button pointer_button;
  } cases;
};
typedef struct _STREAM_LibinputSource_in STREAM_LibinputSource_in;

// output stream for stream processor LibinputSource
struct _STREAM_LibinputSource_out {
  vms_event head;
  union {
    EVENT_hole hole;
    EVENT_LibinputSource_pointer_motion pointer_motion;
    EVENT_LibinputSource_keyboard_key keyboard_key;
    EVENT_LibinputSource_pointer_button pointer_button;
  } cases;
};
typedef struct _STREAM_LibinputSource_out STREAM_LibinputSource_out;
// event declarations for stream type GlobalEvent
struct _EVENT_GlobalEvent_ts {
  double timestamp;
};
typedef struct _EVENT_GlobalEvent_ts EVENT_GlobalEvent_ts;

// input stream for stream type GlobalEvent
struct _STREAM_GlobalEvent_in {
  vms_event head;
  union {
    EVENT_GlobalEvent_ts ts;
  } cases;
};
typedef struct _STREAM_GlobalEvent_in STREAM_GlobalEvent_in;

// output stream for stream processor GlobalEvent
struct _STREAM_GlobalEvent_out {
  vms_event head;
  union {
    EVENT_hole hole;
    EVENT_GlobalEvent_ts ts;
  } cases;
};
typedef struct _STREAM_GlobalEvent_out STREAM_GlobalEvent_out;

// functions that create a hole and update holes

// Declare structs that store the data of streams shared among events in the
// struct

typedef struct _STREAM_WaylandConnection_ARGS {
  int pid;

} STREAM_WaylandConnection_ARGS;

typedef struct _BG_data {
	vms_arbiter_buffer *buffer;

	STREAM_WaylandConnection_ARGS args;
} BG_data;

VEC(WaylandConnections, BG_data *);



// instantiate the structs that store the variables shared among events in the
// same struct

int arbiter_counter; // int used as id for the events that the arbiter
                     // generates, it increases for every event it generates

static bool ARBITER_MATCHED_ =
    false; // in each iteration we set this to true if it mathces
static bool ARBITER_DROPPED_ =
    false; // in each iteration we set this to true if it drops the event
static size_t match_and_no_drop_num =
    0; // count of consecutive matches without dropping the event (we raise a
       // warning when we reach a certain number of this)

// monitor buffer
vms_monitor_buffer *monitor_buffer; // where we store the events that the
                                    // monitor needs to process

bool is_selection_successful;
dll_node **chosen_streams; // used in rule set for get_first/last_n
int current_size_chosen_stream =
    0; // current number of elements in  chosen_streams

void update_size_chosen_streams(const int s) {
  if (s > current_size_chosen_stream) {
    // if s is greater then we need to increase the size of chosen_streams
    free(chosen_streams);
    chosen_streams =
        (dll_node **)calloc(s, sizeof(dll_node *)); // allocate more space
    current_size_chosen_stream =
        s; // set the new number of elements in chosen_streams
  }
}

// globals code
STREAM_GlobalEvent_out *arbiter_outevent;

#include <wayland-util.h> // wl_fixed

// true if s1 has events with higher timestamp
bool fn_topts_ord(vms_arbiter_buffer *s1, vms_arbiter_buffer *s2) {
  STREAM_WaylandConnection_out *ev1, *ev2;
  if ((ev1 = (STREAM_WaylandConnection_out *)vms_arbiter_buffer_top(s1))) {
    if ((ev2 = (STREAM_WaylandConnection_out *)vms_arbiter_buffer_top(s2))) {
      /* every event starts with time field, so just access pointer_motion */
      return ev1->cases.pointer_motion.time > ev2->cases.pointer_motion.time;
    }

    return true;
  }

  return false;
}

static double last_ts = 0;

// functions for streams that determine if an event should be forwarded to the
// monitor

bool SHOULD_KEEP_forward(vms_stream *s, vms_event *e) { return true; }

atomic_int count_event_streams = 0; // number of active event sources

// declare event streams
vms_stream *EV_SOURCE_Clients;
vms_stream *EV_SOURCE_Libinput;

// event sources threads
thrd_t THREAD_Clients;
thrd_t THREAD_Libinput;

// declare arbiter thread
thrd_t ARBITER_THREAD;

// we index rule sets
const int SWITCH_TO_RULE_SET_default = 0;

static size_t RULE_SET_default_nomatch_cnt = 0;
// MAREK knows

int current_rule_set = SWITCH_TO_RULE_SET_default; // initial arbiter rule set

// Arbiter buffer for event source Clients
vms_arbiter_buffer *BUFFER_Clients;

// Arbiter buffer for event source Libinput
vms_arbiter_buffer *BUFFER_Libinput;

// buffer groups
// sorting streams functions

mtx_t LOCK_WaylandConnections;

int PERF_LAYER_forward_WaylandConnection(vms_arbiter_buffer *buffer) {
  // this function forwards everything
  atomic_fetch_add(&count_event_streams, 1);
  vms_stream *stream = vms_arbiter_buffer_stream(buffer);
  void *inevent;
  void *outevent;

  // wait for active buffer
  while ((!vms_arbiter_buffer_active(buffer))) {
    sleep_ns(10);
  }
  while (true) {
    inevent = vms_stream_fetch_dropping(stream, buffer);

    if (inevent == NULL) {
      // no more events
      break;
    }
    outevent = vms_arbiter_buffer_write_ptr(buffer);

    memcpy(outevent, inevent, sizeof(STREAM_WaylandConnection_in));
    vms_arbiter_buffer_write_finish(buffer);

    vms_stream_consume(stream, 1);
  }
  atomic_fetch_add(&count_event_streams, -1);
  return 0;
}

int PERF_LAYER_forward_LibinputSource(vms_arbiter_buffer *buffer) {
  // this function forwards everything
  atomic_fetch_add(&count_event_streams, 1);
  vms_stream *stream = vms_arbiter_buffer_stream(buffer);
  void *inevent;
  void *outevent;

  // wait for active buffer
  while ((!vms_arbiter_buffer_active(buffer))) {
    sleep_ns(10);
  }
  while (true) {
    inevent = vms_stream_fetch_dropping(stream, buffer);

    if (inevent == NULL) {
      // no more events
      break;
    }
    outevent = vms_arbiter_buffer_write_ptr(buffer);

    memcpy(outevent, inevent, sizeof(STREAM_LibinputSource_in));
    vms_arbiter_buffer_write_finish(buffer);

    vms_stream_consume(stream, 1);
  }
  atomic_fetch_add(&count_event_streams, -1);
  return 0;
}

int PERF_LAYER_forward_GlobalEvent(vms_arbiter_buffer *buffer) {
  // this function forwards everything
  atomic_fetch_add(&count_event_streams, 1);
  vms_stream *stream = vms_arbiter_buffer_stream(buffer);
  void *inevent;
  void *outevent;

  // wait for active buffer
  while ((!vms_arbiter_buffer_active(buffer))) {
    sleep_ns(10);
  }
  while (true) {
    inevent = vms_stream_fetch_dropping(stream, buffer);

    if (inevent == NULL) {
      // no more events
      break;
    }
    outevent = vms_arbiter_buffer_write_ptr(buffer);

    memcpy(outevent, inevent, sizeof(STREAM_GlobalEvent_in));
    vms_arbiter_buffer_write_finish(buffer);

    vms_stream_consume(stream, 1);
  }
  atomic_fetch_add(&count_event_streams, -1);
  return 0;
}
int PERF_LAYER_WaylandProcessor(vms_arbiter_buffer *buffer) {
  atomic_fetch_add(&count_event_streams, 1);
  vms_stream *stream = vms_arbiter_buffer_stream(buffer);
  STREAM_WaylandSource_in *inevent;
  STREAM_WaylandConnection_out *outevent;

  // wait for active buffer
  while ((!vms_arbiter_buffer_active(buffer))) {
    sleep_ns(10);
  }
  while (true) {
    inevent =
        vms_stream_fetch(stream);

    if (inevent == NULL) {
      // no more events
      break;
    }

    switch ((inevent->head).kind) {

    case WAYLANDSOURCE_CLIENT_NEW:
    {
      vms_stream_hole_handling hole_handling = {
          .hole_event_size = sizeof(STREAM_WaylandConnection_out),
          .init = &init_hole_hole,
          .update = &update_hole_hole};
      printf("Creating substream : WaylandSource\n");
      vms_stream *ev_source_temp = vms_stream_create_substream(
          stream, NULL, NULL, NULL, NULL, &hole_handling);
      if (!ev_source_temp) {
        fprintf(stderr, "Failed creating substream for 'WaylandConnection'\n");
        abort();
      }
      vms_arbiter_buffer *temp_buffer = vms_arbiter_buffer_create(
          ev_source_temp, sizeof(STREAM_WaylandConnection_out), 1024);

      // vms_stream_register_all_events(ev_source_temp);
      if (vms_stream_register_event(ev_source_temp, "pointer_motion",
                                    WAYLANDCONNECTION_POINTER_MOTION) < 0) {
        fprintf(stderr, "Failed registering event pointer_motion for stream "
                        "<dynamic> : WaylandConnection\n");
        fprintf(stderr, "Available events:\n");
        vms_stream_dump_events(ev_source_temp);
        abort();
      }
      if (vms_stream_register_event(ev_source_temp, "pointer_button",
                                    WAYLANDCONNECTION_POINTER_BUTTON) < 0) {
        fprintf(stderr, "Failed registering event pointer_button for stream "
                        "<dynamic> : WaylandConnection\n");
        fprintf(stderr, "Available events:\n");
        vms_stream_dump_events(ev_source_temp);
        abort();
      }
      if (vms_stream_register_event(ev_source_temp, "keyboard_key",
                                    WAYLANDCONNECTION_KEYBOARD_KEY) < 0) {
        fprintf(stderr, "Failed registering event keyboard_key for stream "
                        "<dynamic> : WaylandConnection\n");
        fprintf(stderr, "Available events:\n");
        vms_stream_dump_events(ev_source_temp);
        abort();
      }

      // we insert a new event source in the corresponding buffer group
      BG_data *data = malloc(sizeof(BG_data));
      if (!data) abort();

      int pid = ((STREAM_WaylandSource_in *)inevent)->cases.client_new.pid;
      data->args.pid = pid;
      data->buffer = temp_buffer;

      mtx_lock(&LOCK_WaylandConnections);
      VEC_PUSH(WaylandConnections, &data);
      mtx_unlock(&LOCK_WaylandConnections);

      thrd_t temp_thread;
      thrd_create(&temp_thread, (void *)PERF_LAYER_forward_WaylandConnection,
                  temp_buffer);
      vms_arbiter_buffer_set_active(temp_buffer, true);

      break;
      }
    }
    vms_stream_consume(stream, 1);
  }
  atomic_fetch_add(&count_event_streams, -1);
  return 0;
}

// variables used to debug arbiter
long unsigned no_consecutive_matches_limit = 1UL << 35;
int no_matches_count = 0;

bool are_there_events(vms_arbiter_buffer *b) {
  return vms_arbiter_buffer_is_done(b) > 0;
}

bool are_buffers_done() {
  if (!vms_arbiter_buffer_is_done(BUFFER_Clients))
    return 0;
  if (!vms_arbiter_buffer_is_done(BUFFER_Libinput))
    return 0;

  mtx_lock(&LOCK_WaylandConnections);
  for (int i = 0; i < VEC_SIZE(WaylandConnections); ++i) {
    if (!vms_arbiter_buffer_is_done(WaylandConnections[i]->buffer))
      return 0;
  }
  mtx_unlock(&LOCK_WaylandConnections);

  return 1;
}

static int __work_done = 0;
/* TODO: make a keywork from this */
void done() { __work_done = 1; }

static inline bool are_streams_done() {
  assert(count_event_streams >= 0);
  return (count_event_streams == 0 && are_buffers_done() &&
          !ARBITER_MATCHED_) ||
         __work_done;
}

static inline bool is_buffer_done(vms_arbiter_buffer *b) {
  return vms_arbiter_buffer_is_done(b);
}

static inline bool check_at_least_n_events(size_t count, size_t n) {
  // count is the result after calling vms_arbiter_buffer_peek
  return count >= n;
}

static bool are_events_in_head(char *e1, size_t i1, char *e2, size_t i2,
                               int count, size_t ev_size, int event_kinds[],
                               int n_events) {
  // this functions checks that a buffer have the same kind of event as the
  // array event_kinds
  assert(n_events > 0);
  if (count < n_events) {
    return false;
  }

  int i = 0;
  while (i < i1) {
    vms_event *ev = (vms_event *)(e1);
    assert(ev->kind > 0);
    if (ev->kind != event_kinds[i]) {
      return false;
    }
    if (--n_events == 0)
      return true;
    i += 1;
    e1 += ev_size;
  }

  i = 0;
  while (i < i2) {
    vms_event *ev = (vms_event *)e2;
    assert(ev->kind > 0);
    if (ev->kind != event_kinds[i1 + i]) {
      return false;
    }
    if (--n_events == 0)
      return true;
    i += 1;
    e2 += ev_size;
  }

  return true;
}

/*
static inline dump_event_data(vms_event *ev, size_t ev_size) {
    unsigned char *data = ev;
    fprintf(stderr, "[");
    for (unsigned i = sizeof(*ev); i < ev_size; ++i) {
        fprintf(stderr, "0x%x ", data[i]);
    }
    fprintf(stderr, "]");
}
*/

const char *get_event_name(int ev_src_index, int event_index) {
  if (event_index <= 0) {
    return "<invalid>";
  }

  if (event_index == VMS_EVENT_HOLE_KIND) {
    return "hole";
  }

  if (ev_src_index == 0) {

    if (event_index == WAYLANDSOURCE_CLIENT_NEW) {
      return "client_new";
    }

    if (event_index == WAYLANDSOURCE_CLIENT_EXIT) {
      return "client_exit";
    }

    if (event_index == WAYLANDSOURCE_HOLE) {
      return "hole";
    }

    fprintf(stderr,
            "No event matched! this should not happen, please report!\n");
    return "";
  }

  if (ev_src_index == 1) {

    if (event_index == WAYLANDCONNECTION_POINTER_MOTION) {
      return "pointer_motion";
    }

    if (event_index == WAYLANDCONNECTION_POINTER_BUTTON) {
      return "pointer_button";
    }

    if (event_index == WAYLANDCONNECTION_KEYBOARD_KEY) {
      return "keyboard_key";
    }

    if (event_index == WAYLANDCONNECTION_HOLE) {
      return "hole";
    }

    fprintf(stderr,
            "No event matched! this should not happen, please report!\n");
    return "";
  }

  if (ev_src_index == 2) {

    if (event_index == LIBINPUTSOURCE_POINTER_MOTION) {
      return "pointer_motion";
    }

    if (event_index == LIBINPUTSOURCE_POINTER_BUTTON) {
      return "pointer_button";
    }

    if (event_index == LIBINPUTSOURCE_KEYBOARD_KEY) {
      return "keyboard_key";
    }

    if (event_index == LIBINPUTSOURCE_HOLE) {
      return "hole";
    }

    fprintf(stderr,
            "No event matched! this should not happen, please report!\n");
    return "";
  }

  if (ev_src_index == 3) {

    if (event_index == GLOBALEVENT_TS) {
      return "ts";
    }

    if (event_index == GLOBALEVENT_HOLE) {
      return "hole";
    }

    fprintf(stderr,
            "No event matched! this should not happen, please report!\n");
    return "";
  }

  printf("Invalid event source! this should not happen, please report!\n");
  return 0;
}

/* src_idx = -1 if unknown */
static void print_buffer_prefix(vms_arbiter_buffer *b, int src_idx,
                                size_t n_events, int cnt, char *e1, size_t i1,
                                char *e2, size_t i2) {
  if (cnt == 0) {
    fprintf(stderr, " empty\n");
    return;
  }
  const size_t ev_size = vms_arbiter_buffer_elem_size(b);
  int n = 0;
  int i = 0;
  while (i < i1) {
    vms_event *ev = (vms_event *)(e1);
    fprintf(stderr, "  %d: {id: %5lu, kind: %3lu", ++n, vms_event_id(ev),
            vms_event_kind(ev));
    if (src_idx != -1)
      fprintf(stderr, " -> %-12s", get_event_name(src_idx, vms_event_kind(ev)));
    /*dump_event_data(ev, ev_size);*/
    fprintf(stderr, "}\n");
    if (--n_events == 0)
      return;
    i += 1;
    e1 += ev_size;
  }

  i = 0;
  while (i < i2) {
    vms_event *ev = (vms_event *)e2;
    fprintf(stderr, "  %d: {id: %5lu, kind: %3lu", ++n, vms_event_id(ev),
            vms_event_kind(ev));
    if (src_idx != -1)
      fprintf(stderr, " -> %-12s", get_event_name(src_idx, vms_event_kind(ev)));
    /*dump_event_data(ev, ev_size);*/
    fprintf(stderr, "}\n");

    if (--n_events == 0)
      return;
    i += 1;
    e2 += ev_size;
  }
}

static inline vms_event *get_event_at_index(char *e1, size_t i1, char *e2,
                                            size_t i2, size_t size_event,
                                            int element_index) {
  if (element_index < i1) {
    return (vms_event *)(e1 + (element_index * size_event));
  } else {
    element_index -= i1;
    return (vms_event *)(e2 + (element_index * size_event));
  }
}

// arbiter outevent (the monitor looks at this)
STREAM_GlobalEvent_out *arbiter_outevent;

int RULE_SET_default();

void print_event_name(int ev_src_index, int event_index) {
  if (event_index <= 0) {
    printf("<invalid (%d)>\n", event_index);
    return;
  }

  if (event_index == VMS_EVENT_HOLE_KIND) {
    printf("hole\n");
    return;
  }

  if (ev_src_index == 0) {

    if (event_index == VMS_EVENT_LAST_SPECIAL_KIND + 1) {
      printf("client_new\n");
      return;
    }

    if (event_index == VMS_EVENT_LAST_SPECIAL_KIND + 2) {
      printf("client_exit\n");
      return;
    }

    if (event_index == VMS_EVENT_HOLE_KIND) {
      printf("hole\n");
      return;
    }

    printf("No event matched! this should not happen, please report!\n");
    return;
  }

  if (ev_src_index == 1) {

    if (event_index == VMS_EVENT_LAST_SPECIAL_KIND + 1) {
      printf("pointer_motion\n");
      return;
    }

    if (event_index == VMS_EVENT_LAST_SPECIAL_KIND + 2) {
      printf("pointer_button\n");
      return;
    }

    if (event_index == VMS_EVENT_LAST_SPECIAL_KIND + 3) {
      printf("keyboard_key\n");
      return;
    }

    if (event_index == VMS_EVENT_HOLE_KIND) {
      printf("hole\n");
      return;
    }

    printf("No event matched! this should not happen, please report!\n");
    return;
  }

  printf("Invalid event source! this should not happen, please report!\n");
}

int get_event_at_head(vms_arbiter_buffer *b) {
  void *e1;
  size_t i1;
  void *e2;
  size_t i2;

  int count = vms_arbiter_buffer_peek(b, 0, &e1, &i1, &e2, &i2);
  if (count == 0) {
    return -1;
  }
  vms_event *ev = (vms_event *)(e1);
  assert(ev->kind > 0);
  return ev->kind;
}

void print_buffers_state() {
  int event_index;
  int count;
  void *e1, *e2;
  size_t i1, i2;

  fprintf(stderr, "Prefix of 'Clients':\n");
  count = vms_arbiter_buffer_peek(BUFFER_Clients, 10, (void **)&e1, &i1,
                                  (void **)&e2, &i2);
  print_buffer_prefix(BUFFER_Clients, 0, i1 + i2, count, e1, i1, e2, i2);
  fprintf(stderr, "Prefix of 'Libinput':\n");
  count = vms_arbiter_buffer_peek(BUFFER_Libinput, 10, (void **)&e1, &i1,
                                  (void **)&e2, &i2);
  print_buffer_prefix(BUFFER_Libinput, 1, i1 + i2, count, e1, i1, e2, i2);
}

static void print_buffer_state(vms_arbiter_buffer *buffer) {
  int count;
  void *e1, *e2;
  size_t i1, i2;
  count =
      vms_arbiter_buffer_peek(buffer, 10, (void **)&e1, &i1, (void **)&e2, &i2);
  print_buffer_prefix(buffer, -1, i1 + i2, count, e1, i1, e2, i2);
}


int arbiter() {

  while (!are_streams_done()) {
    ARBITER_MATCHED_ = false;
    ARBITER_DROPPED_ = false;

    void *e1, *e2;
    size_t i1, i2;
    int count = vms_arbiter_buffer_peek(BUFFER_Libinput, 0,
		    			(void **)&e1, &i1,
					(void **)&e2, &i2);
    if (count > 0) {
	    STREAM_LibinputSource_out *ev = e1;
	    for (int i = 0; i < i1; ++i) {
		    ++ev;
	    }
	    ev = e2;
	    for (int i = 0; i < i2; ++i) {
		    printf("kind: %lu, id: %lu\n", ev->head.kind, ev->head.id);
		    ++ev;
	    }
	    vms_arbiter_buffer_drop(BUFFER_Libinput, count);
    }

    double least_time = DBL_MAX;
    vms_arbiter_buffer *lbuffer = NULL;

    /* send the event with the least timestamp to the monitor */
    mtx_lock(&LOCK_WaylandConnections);
    for (int i = 0; i < VEC_SIZE(WaylandConnections); ++i) {
	STREAM_WaylandConnection_out *ev
		= (STREAM_WaylandConnection_out *)
		vms_arbiter_buffer_top(WaylandConnections[i]->buffer);
	if (!ev)
	    continue;

	if (ev->cases.pointer_motion.time < least_time) {
	    least_time = ev->cases.pointer_motion.time;
	    lbuffer = WaylandConnections[i]->buffer;
	}
    }
    mtx_unlock(&LOCK_WaylandConnections);

    if (lbuffer) {
	STREAM_WaylandConnection_out *ev
		= (STREAM_WaylandConnection_out *)
		vms_arbiter_buffer_top(lbuffer);
	printf("%f\n", ev->cases.pointer_motion.time);

	vms_arbiter_buffer_drop(lbuffer, 1);
    }

    if (!ARBITER_DROPPED_ && ARBITER_MATCHED_) {
      if (++match_and_no_drop_num >= 40000) {
        if (match_and_no_drop_num == 40000) {
          fprintf(stderr, "WARNING: arbiter matched 40000 times without "
                          "consuming an event, that might suggest a problem\n");
        }

        /* do not burn CPU as we might expect another void iteration */
        sleep_ns(500);

        if (++match_and_no_drop_num > 60000) {
          fprintf(stderr, "\033[31mWARNING: arbiter matched 60000 times "
                          "without consuming an event!\033[0m\n");
          match_and_no_drop_num = 0;
          void *e1, *e2;
          size_t i1, i2;
          int count_;
          fprintf(stderr, "Prefix of 'Libinput':\n");
          count_ = vms_arbiter_buffer_peek(BUFFER_Libinput, 5, (void **)&e1,
                                           &i1, (void **)&e2, &i2);
          print_buffer_prefix(BUFFER_Libinput, 2, i1 + i2, count_, e1, i1, e2,
                              i2);
          printf("***** BUFFER GROUPS *****\n");
          printf("***** WaylandConnections *****\n");
      	  mtx_lock(&LOCK_WaylandConnections);
	  for (int i = 0; i < VEC_SIZE(WaylandConnections); ++i)
          {
              printf("WaylandConnections[%d].ARGS{", i);
              printf("pid = %d\n", WaylandConnections[i]->args.pid);

              printf("}\n");
              char *e1_BG;
              size_t i1_BG;
              char *e2_BG;
              size_t i2_BG;
              int COUNT_BG_TEMP_ =
                  vms_arbiter_buffer_peek(WaylandConnections[i]->buffer, 5, (void **)&e1_BG,
                                          &i1_BG, (void **)&e2_BG, &i2_BG);
              printf("WaylandConnections[%d].buffer{\n", i);
              print_buffer_prefix(WaylandConnections[i]->buffer, 1, i1_BG + i2_BG,
                                  COUNT_BG_TEMP_, e1_BG, i1_BG, e2_BG, i2_BG);
              printf("}\n");
          }
          mtx_unlock(&LOCK_WaylandConnections);
        }
      }
    }
  }
  vms_monitor_set_finished(monitor_buffer);
  return 0;
}

static void sig_handler(int sig) {
  printf("signal %d caught...", sig);
  vms_stream_detach(EV_SOURCE_Clients);
  vms_stream_detach(EV_SOURCE_Libinput);
  __work_done = 1;
} // MAREK knows

static void setup_signals() {
  // MAREK knows
  if (signal(SIGINT, sig_handler) == SIG_ERR) {
    perror("failed setting SIGINT handler");
  }

  if (signal(SIGABRT, sig_handler) == SIG_ERR) {
    perror("failed setting SIGINT handler");
  }

  if (signal(SIGIOT, sig_handler) == SIG_ERR) {
    perror("failed setting SIGINT handler");
  }

  if (signal(SIGSEGV, sig_handler) == SIG_ERR) {
    perror("failed setting SIGINT handler");
  }
}

int main(int argc, char **argv) {
  setup_signals();

  arbiter_counter = 10;
  // startup code

  // init. event sources streams

  {}

  // connecting event sources
  // connect to event source Clients

  vms_stream_hole_handling hh_Clients = {
      .hole_event_size = sizeof(STREAM_WaylandConnection_out),
      .init = &init_hole_hole,
      .update = &update_hole_hole};

  EV_SOURCE_Clients =
      vms_stream_create_from_argv("Clients", argc, argv, &hh_Clients);
  BUFFER_Clients = vms_arbiter_buffer_create(
      EV_SOURCE_Clients, sizeof(STREAM_WaylandConnection_out), 64);

  // register events in Clients
  if (vms_stream_register_event(EV_SOURCE_Clients, "client_new",
                                WAYLANDSOURCE_CLIENT_NEW) < 0) {
    fprintf(stderr, "Failed registering event client_new for stream Clients : "
                    "WaylandSource\n");
    fprintf(stderr, "Available events:\n");
    vms_stream_dump_events(EV_SOURCE_Clients);
    abort();
  }
  if (vms_stream_register_event(EV_SOURCE_Clients, "client_exit",
                                WAYLANDSOURCE_CLIENT_EXIT) < 0) {
    fprintf(stderr, "Failed registering event client_exit for stream Clients : "
                    "WaylandSource\n");
    fprintf(stderr, "Available events:\n");
    vms_stream_dump_events(EV_SOURCE_Clients);
    abort();
  }
  // connect to event source Libinput

  vms_stream_hole_handling hh_Libinput = {.hole_event_size =
                                              sizeof(STREAM_LibinputSource_out),
                                          .init = &init_hole_hole,
                                          .update = &update_hole_hole};

  EV_SOURCE_Libinput =
      vms_stream_create_from_argv("Libinput", argc, argv, &hh_Libinput);
  BUFFER_Libinput = vms_arbiter_buffer_create(
      EV_SOURCE_Libinput, sizeof(STREAM_LibinputSource_out), 1024);

  // register events in Libinput
  if (vms_stream_register_event(EV_SOURCE_Libinput, "pointer_motion",
                                LIBINPUTSOURCE_POINTER_MOTION) < 0) {
    fprintf(stderr, "Failed registering event pointer_motion for stream "
                    "Libinput : LibinputSource\n");
    fprintf(stderr, "Available events:\n");
    vms_stream_dump_events(EV_SOURCE_Libinput);
    abort();
  }
  if (vms_stream_register_event(EV_SOURCE_Libinput, "pointer_button",
                                LIBINPUTSOURCE_POINTER_BUTTON) < 0) {
    fprintf(stderr, "Failed registering event pointer_button for stream "
                    "Libinput : LibinputSource\n");
    fprintf(stderr, "Available events:\n");
    vms_stream_dump_events(EV_SOURCE_Libinput);
    abort();
  }
  if (vms_stream_register_event(EV_SOURCE_Libinput, "keyboard_key",
                                LIBINPUTSOURCE_KEYBOARD_KEY) < 0) {
    fprintf(stderr, "Failed registering event keyboard_key for stream Libinput "
                    ": LibinputSource\n");
    fprintf(stderr, "Available events:\n");
    vms_stream_dump_events(EV_SOURCE_Libinput);
    abort();
  }

  printf("-- creating buffers\n");
  monitor_buffer = vms_monitor_buffer_create(sizeof(STREAM_GlobalEvent_out), 4);

  // init buffer groups
  printf("-- initializing buffer groups\n");
  VEC_INIT(WaylandConnections);
  if (mtx_init(&LOCK_WaylandConnections, mtx_plain) != 0) {
    printf("mutex init has failed for WaylandConnections lock\n");
    return 1;
  }

  // create source-events threads
  printf("-- creating performance threads\n");
  thrd_create(&THREAD_Clients, (void *)PERF_LAYER_WaylandProcessor,
              BUFFER_Clients);
  thrd_create(&THREAD_Libinput, (void *)PERF_LAYER_forward_LibinputSource,
              BUFFER_Libinput);

  // create arbiter thread
  printf("-- creating arbiter thread\n");
  thrd_create(&ARBITER_THREAD, arbiter, 0);

  // activate buffers
  vms_arbiter_buffer_set_active(BUFFER_Clients, true);
  vms_arbiter_buffer_set_active(BUFFER_Libinput, true);

  // monitor
  printf("-- starting monitor code \n");
  STREAM_GlobalEvent_out *received_event;
  while (true) {
    received_event = fetch_arbiter_stream(monitor_buffer);
    if (received_event == NULL) {
      break;
    }

    if (received_event->head.kind == VMS_EVENT_LAST_SPECIAL_KIND + 1) {
      double time = received_event->cases.ts.timestamp;

      if (true) {
        vamos_assert(last_ts <= time);
        printf("TS: %f\n", time);
        last_ts = time;
      }
    }

    vms_monitor_buffer_consume(monitor_buffer, 1);
  }

  printf("-- cleaning up\n");
  VEC_DESTROY(WaylandConnections);
  mtx_destroy(&LOCK_WaylandConnections);
  vms_stream_destroy(EV_SOURCE_Clients);
  vms_stream_destroy(EV_SOURCE_Libinput);
  vms_arbiter_buffer_free(BUFFER_Clients);
  vms_arbiter_buffer_free(BUFFER_Libinput);
  free(monitor_buffer);
  free(chosen_streams);

  // BEGIN clean up code
}
