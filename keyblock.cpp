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

#include <gsl/gsl_statistics.h>

using namespace std;

#define PRESS   true
#define RELEASE false

/*  Returns a string containing the stdout of a shell command.
*/
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

/*  Returns the contents of a directory matching a pattern
    Example: ls("/*") returns files and directories in /
*/
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

/*  Returns true if str starts with a given prefix, false otherwise.
*/
bool startswith(string prefix, string str) {

    size_t lenpre = strlen(prefix.c_str()),
           lenstr = strlen(str.c_str());
    return lenstr < lenpre ? false : strncmp(prefix.c_str(), str.c_str(), lenpre) == 0;
}

/*  Returns input device ids read from /dev/input/
*/
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

/*  Returns input device ids from devlist that are not found in prev_devlist.
*/

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

/*  Returns true if file exists, false otherwise.
*/
inline bool exists (const string& filename) {
    struct stat buffer;
    return (stat (filename.c_str(), &buffer) == 0);
}

/*  Converts a linux kernel keycode to X11 keycode.
*/
int lk2x11(int lk_keycode) {
    return lk_keycode + 8;
}

/*  Performs minimum allowed keystroke latency on keystroke timestamps.
    Returns true if keystroke latency average is below a threshold.
*/
bool min_allowed_keystroke_latency(vector<double> keystrokes) {
    double threshold = 7000000;
    double delays[keystrokes.size() - 1];
    if(keystrokes.size() > 1) {
        for(int i=0; i < keystrokes.size() - 1; i++) {
            delays[i] = keystrokes[i] - keystrokes[i+1];
        }
    }
    double mean = gsl_stats_mean(delays, 1, keystrokes.size()-1);
    if(mean < threshold) return true;
    return false;
}

/*  Performs an attack similarity analysis on keystroke timestamps.
    Returns true if mean or standard deviation of measured keystroke
    latencies is close to empirical mean or std of keystroke injection latencies.
*/
bool attack_similarity_analysis(vector<double> keystrokes) {
    double delays[keystrokes.size() - 1];
    if(keystrokes.size() > 1) {
        for(int i=0; i < keystrokes.size() - 1; i++) {
            delays[i] = keystrokes[i] - keystrokes[i+1];
        }
    }
    double mean = gsl_stats_mean(delays, 1, keystrokes.size()-1);
    double std = gsl_stats_sd_m(delays, 1, keystrokes.size()-1, mean);
    if(mean - 2000000 < 500000 || std < 100000 ) return true;
    return false;
}

/*  Performs a human similarity analysis on keystroke timestamps.
    Returns true if latency mean or std is far below measured human latencies.
    The fixed values are computed from empirical data.
*/
bool human_similarity_analysis(vector<double> keystrokes) {
    double delays[keystrokes.size() - 1];
    if(keystrokes.size() > 1) {
        for(int i=0; i < keystrokes.size() - 1; i++) {
            delays[i] = keystrokes[i] - keystrokes[i+1];
        }
    }
    double mean = gsl_stats_mean(delays, 1, keystrokes.size()-1);
    double std = gsl_stats_sd_m(delays, 1, keystrokes.size()-1, mean);
    if(mean < 31.59*1000000 - 2*9.59*1000000 || std < 1000000) return true;
    else return false;
}

/*  Monitor thread.
    Captures all events from device with given id, performs analysis,
    redirects events to X if no unusual activity is detected, otherwise,
    block device until disconnect. Thread ends if device is removed.
*/
void monitor(int id) {
    
    string device = "/dev/input/event" + to_string(id);
    char name[256] = "Unknown";
    int result = 0;

    int fd = open(device.c_str(), O_RDONLY);
    result = ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    ioctl(fd, EVIOCGRAB, 1);

    cout << "Monitoring " << device << " " << name << endl;

    struct input_event ev;

    Display *display;
    display = XOpenDisplay(NULL);
    KeyCode keycode = 0;

    vector<double> keystrokes;
    bool disable = false;

    while (true) {

        read(fd, &ev, sizeof(struct input_event));

        if(ev.type == 1) {

            if(ev.value == 1) {
                double t = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
                keystrokes.insert(keystrokes.begin(), t);
            }

            if(keystrokes.size() > 1) {
                
                disable = disable ? disable : human_similarity_analysis(keystrokes);
                
                if(disable) {

                    for(int i=8; i < 256; i++) {
                        XTestFakeKeyEvent(display, i, RELEASE, 0);
                        XFlush(display);
                    }
                    
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
        }

        while(keystrokes.size() > 10) keystrokes.pop_back();

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
                threads.push_back(thread(monitor, newdev[i]));
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
