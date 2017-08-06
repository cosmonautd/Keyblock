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
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

// https://stackoverflow.com/questions/1485116/capturing-keystrokes-in-gnu-linux-in-c
// https://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix
// https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
// https://unix.stackexchange.com/questions/17170/disable-keyboard-mouse-input-on-unix-under-x
// https://ysonggit.github.io/coding/2014/12/16/split-a-string-using-c.html

using namespace std;

string shell(const char* command) {

    array<char, 128> buffer;
    string output;
    shared_ptr<FILE> pipe(popen(command, "r"), pclose);

    if(!pipe) throw runtime_error("popen() failed!");

    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr) {
            output += buffer.data();
        }
    }

    return output;
}

vector<string> split(const string &input, char delim) {

    stringstream ss(input);
    string item;
    vector<string> tokens;

    while(getline(ss, item, delim)) {
        tokens.push_back(item);
    }

    return tokens;
}

vector<int> strvec2int(vector<string> input) {

    vector<int> output;

    for(int i=0; i < input.size(); i++) {
        output.push_back(stoi(input[i]));
    }

    return output;
}

vector<int> get_devices() {
    return strvec2int(split(shell("xinput --list --id-only"), '\n'));
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

int main(int argc, char **argv) {

    int newdev_delay = 100;

    while(true) {

        vector<int> devlist = get_devices();
        vector<int> prev_devlist;
        bool newdev = false;

        while(!newdev) {

            devlist = get_devices();
            vector<int> new_devices = get_new_devices(devlist, prev_devlist);

            if(new_devices.size() > 0 && prev_devlist.size() != 0) {
                cout << "New input devices were detected:" << endl;
                for(int i=0; i < new_devices.size(); i++)
                    cout << "        /dev/input/event" << new_devices[i] << endl;
            }
            
            this_thread::sleep_for(chrono::milliseconds(newdev_delay));
            prev_devlist.assign(devlist.begin(), devlist.end());
        }
    }

    // int fd = open("/dev/input/event3", O_RDONLY);
    // struct input_event ev;

    // while (true) {

    //     read(fd, &ev, sizeof(struct input_event));
    //     if(ev.type == 1)
    //         cout << "key " << ev.code << " state " << ev.value << endl;

    // }

    return 0;
}
