#include <iostream>
#include <cstdint>
#include <set>
#include <vector>

#include "monitor.hpp"

#define TAU 1000
#define DELTA 20

bool waiting_for_segment = true;

struct AwaitValue {
    double time;
    uint32_t value;

    bool operator<(const AwaitValue& rhs) const {
        return time < rhs.time && value < rhs.value;
    }
};

std::set<AwaitValue> waiting_keys;
std::set<AwaitValue> waiting_buttons;

static void check_state(double time) {
    for (auto it = waiting_keys.begin(), et = waiting_keys.end(); it != et;) {
        if (time - it->time > TAU) {
            auto tmp = it++;
            //std::cerr << "ERROR: a key press was not found\n";
            waiting_keys.erase(tmp);
        } else {
            ++it;
        }
    }

    for (auto it = waiting_buttons.begin(), et = waiting_buttons.end(); it != et;) {
        if (time - it->time > TAU) {
            auto tmp = it++;
            //std::cerr << "ERROR: a button press was not found\n";
            waiting_buttons.erase(tmp);
        } else {
            ++it;
        }
    }
}

struct Point {
    double x;
    double y;

    bool operator<(const Point& rhs) const {
        return x < rhs.x && y < rhs.y;
    }

    bool close_to(const Point& rhs, double D = 10) const {
        return ((rhs.x - x)*(rhs.x - x) + (rhs.y - y)*(rhs.y -y)) <= D*D;
    }
};

struct PointerState {
    bool fresh = true;
    Point way_p;
    Point lib_p;

    std::vector<Point> surrounding;
} pointer_state;

void wayland_motion(double time, double x, double y) {
    if (waiting_for_segment)
        return;

    //std::cout << "way.motion(" << time << ", " << x << "," <<  y << ")\n";

    if (pointer_state.fresh) {
        pointer_state.fresh = false;
        pointer_state.way_p = pointer_state.lib_p = Point{x, y};
        return;
    }

    if (!pointer_state.lib_p.close_to(Point{x, y}, DELTA)) {
        std::cerr << "ERROR: Pointer motion diverged: "
            << "{" << pointer_state.way_p.x << ", " << pointer_state.way_p.y << "}"
            << " -> "
            << "{" << pointer_state.lib_p.x << ", " << pointer_state.lib_p.y << "}"
            << " not close to "
            << "{" << x << ", " << y << "}\n";
    }

    pointer_state.way_p = pointer_state.lib_p = Point{x, y};

    check_state(time);
}


void libinput_motion(double time, double dx, double dy) {
    if (waiting_for_segment)
        return;

    //std::cout << "lib.motion(" << time << ", " << dx << "," <<  dy << ")\n";

    pointer_state.lib_p.x += dx;
    pointer_state.lib_p.y += dy;

    check_state(time);
}

void wayland_button(double time, uint32_t button, uint8_t pressed) {
    if (waiting_for_segment)
        return;

    waiting_buttons.insert(AwaitValue{time, button});
    check_state(time);
}

void libinput_button(double time, uint32_t button, uint8_t pressed) {
    if (waiting_for_segment)
        return;

    auto it = waiting_buttons.begin();
    for (auto et = waiting_buttons.end(); it != et;) {
        if (button == it->value) {
            waiting_buttons.erase(it);
            break;
        } else {
            ++it;
        }
    }

    if (it == waiting_buttons.end()) {
        //std::cerr << "ERROR: a button press was not found\n";
    }
    check_state(time);
}

void wayland_key(double time, uint32_t key, uint8_t pressed) {
    if (waiting_for_segment)
        return;

    waiting_keys.insert(AwaitValue{time, key});

    check_state(time);
}



void libinput_key(double time, uint32_t key, uint8_t pressed) {
    if (waiting_for_segment)
        return;

    auto it = waiting_keys.begin();
    for (auto et = waiting_keys.end(); it != et;) {
        if (key == it->value) {
            waiting_keys.erase(it);
            break;
        } else {
            ++it;
        }
    }

    if (it == waiting_keys.end()) {
        //std::cerr << "ERROR: a key press was not found\n";
    }


    check_state(time);
}


void segment_start() {
    waiting_for_segment = false;
    pointer_state.fresh = true;

    std::cout << "--- segment start ---\n";
}


void segment_end() {
    if (waiting_for_segment)
        return;
    std::cout << "--- segment end ---\n";
}

void hole() {
    waiting_for_segment = true;
    waiting_keys.clear();
    waiting_buttons.clear();

    std::cout << "--- hole ---\n";
}

void end() {
    if (!waiting_keys.empty()) {
        //std::cerr << "ERROR: a key press was not found\n";
    }

    if (!waiting_buttons.empty()) {
        //std::cerr << "ERROR: a button press was not found\n";
    }
}
