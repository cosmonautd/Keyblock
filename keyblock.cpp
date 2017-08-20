#include <iostream>
#include <string>
#include <cstdio>
#include <memory>
#include <array>
#include <algorithm>
#include <chrono>
#include <thread>
#include <sstream>
#include <vector>
#include <glob.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/XTest.h>

// https://stackoverflow.com/questions/1485116/capturing-keystrokes-in-gnu-linux-in-c
// https://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix
// https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
// https://unix.stackexchange.com/questions/17170/disable-keyboard-mouse-input-on-unix-under-x
// https://ysonggit.github.io/coding/2014/12/16/split-a-string-using-c.html
// https://stackoverflow.com/questions/7668872/need-to-intercept-hid-keyboard-events-and-then-block-them
// https://stackoverflow.com/questions/29104304/remap-a-keyboard-with-ioctl-under-linux
// https://stackoverflow.com/questions/22209267/capture-hid-keyboard-event
// https://bharathisubramanian.wordpress.com/2010/03/14/x11-fake-key-event-generation-using-xtest-ext/
// https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
// http://www.comptechdoc.org/os/linux/howlinuxworks/linux_hlkeycodes.html
// https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c

using namespace std;

#define PRESS   true
#define RELEASE false

string shell(string command) {

    array<char, 128> buffer;
    string output;
    shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);

    if(!pipe) throw runtime_error("popen() failed!");

    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr) {
            output += buffer.data();
        }
    }

    return output;
}

vector<string> ls(const string& pattern){
    glob_t glob_result;
    glob(pattern.c_str(),GLOB_TILDE,NULL,&glob_result);
    vector<string> files;
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        files.push_back(string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    return files;
}

bool startswith(string prefix, string str) {

    size_t lenpre = strlen(prefix.c_str()),
           lenstr = strlen(str.c_str());
    return lenstr < lenpre ? false : strncmp(prefix.c_str(), str.c_str(), lenpre) == 0;
}

vector<int> get_devices() {
    vector<int> output;
    vector<string> inputdevs = ls("/dev/input/*");
    for(int i=0; i < inputdevs.size(); i++) {
        if(startswith("/dev/input/event", inputdevs[i])) {
            inputdevs[i].erase(0, 16);
            output.push_back(stoi(inputdevs[i]));
        }
    }
    return output;
}

vector<int> get_new_devices(vector<int> devlist, vector<int> prev_devlist) {

    vector<int> output;

    sort(prev_devlist.begin(), prev_devlist.end());

    for(int i=0; i < devlist.size(); i++) {
        if(!binary_search(prev_devlist.begin(), prev_devlist.end(), devlist[i])) {
            output.push_back(devlist[i]);
        }
    }

    return output;
}

inline bool exists (const string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

int lk2x11(int lk_keycode) {
    return lk_keycode + 8;
}

void monitor0(int id) {

    string device = "/dev/input/event" + to_string(id);
    char name[256] = "Unknown";
    int result = 0;

    int fd = open(device.c_str(), O_RDONLY);
    result = ioctl(fd, EVIOCGNAME(sizeof(name)), name);

    cout << "Monitoring " << device << " " << name << endl;

    struct input_event ev;

    while (true) {

        auto start = chrono::high_resolution_clock::now();
        read(fd, &ev, sizeof(struct input_event));

        if(ev.type == 1) {

            auto finish = chrono::high_resolution_clock::now();
            double interval = chrono::duration_cast<chrono::nanoseconds>(finish - start).count();

            if(interval < 1000) {
                ioctl(fd, EVIOCGRAB, 1);
                cout << "Disabled device " << id << " tried:" << endl;
                cout << "        key " << ev.code << " state " << ev.value << endl;
            }

        }

        if(!exists(device)) {
            break;
        }
    }

    ioctl(fd, EVIOCGRAB, 0);
    close(fd);
    cout << "Leaving " << device << endl;
}

void monitor1(int id) {
    
    string device = "/dev/input/event" + to_string(id);
    char name[256] = "Unknown";
    int result = 0;

    int fd = open(device.c_str(), O_RDONLY);
    result = ioctl(fd, EVIOCGNAME(sizeof(name)), name);

    cout << "Monitoring " << device << " " << name << endl;

    struct input_event ev;

    Display *display;
    display = XOpenDisplay(NULL);
    KeyCode keycode = 0;

    ioctl(fd, EVIOCGRAB, 1);

    while (true) {

        auto start = chrono::high_resolution_clock::now();
        read(fd, &ev, sizeof(struct input_event));

        if(ev.type == 1) {

            auto finish = chrono::high_resolution_clock::now();
            double interval = chrono::duration_cast<chrono::nanoseconds>(finish - start).count();

            if(interval < 1000) {

                cout << "Disabled device " << id << " tried:" << endl;
                cout << "        key " << ev.code << " state " << ev.value << endl;

            } else {

                keycode = ev.code;

                if(ev.value == 1) {
                    XTestFakeKeyEvent(display, lk2x11(keycode), PRESS, 0);
                    XFlush(display);
                } else if(ev.value == 0) {
                    XTestFakeKeyEvent(display, lk2x11(keycode), RELEASE, 0);
                    XFlush(display);
                }

            }

        }

        if(!exists(device)) {
            break;
        }
    }

    ioctl(fd, EVIOCGRAB, 0);
    close(fd);
    cout << "Leaving " << device << endl;
}

int main(int argc, char **argv) {

    int newdev_delay = 100;

    vector<int> devlist;
    vector<int> prev_devlist;
    vector<thread> threads;

    while(true) {

        devlist = get_devices();
        vector<int> newdev = get_new_devices(devlist, prev_devlist);

        if(newdev.size() > 0 && prev_devlist.size() != 0) {
            cout << "New input devices were detected:" << endl;
            for(int i=0; i < newdev.size(); i++) {
                cout << "        /dev/input/event" << newdev[i] << endl;
                threads.push_back(thread(monitor1, newdev[i]));
            }
        }

        this_thread::sleep_for(chrono::milliseconds(newdev_delay));
        prev_devlist.assign(devlist.begin(), devlist.end());
    }

    for(int i=0; i < threads.size(); i++) {
        threads[i].join();
    }

    return 0;
}
