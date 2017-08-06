#include <iostream>
#include <string>
#include <cstdio>
#include <memory>
#include <array>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

// https://stackoverflow.com/questions/1485116/capturing-keystrokes-in-gnu-linux-in-c
// https://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix
// https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
// https://unix.stackexchange.com/questions/17170/disable-keyboard-mouse-input-on-unix-under-x

using namespace std;

string shell(const char* command) {

    array<char, 128> buffer;
    string output;
    shared_ptr<FILE> pipe(popen(command, "r"), pclose);

    if(!pipe) throw runtime_error("popen() failed!");

    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            output += buffer.data();
    }

    return output;
}

string get_devices() {
    string devlist = shell("xinput --list --id-only");
    replace(devlist.begin(), devlist.end(), '\n', ' ');
    return devlist;
}

bool change(string devlist, string prev_devlist) {
    int prev_ndev = count(prev_devlist.begin(), prev_devlist.end(), ' ') + 1;
    int ndev = count(devlist.begin(), devlist.end(), ' ') + 1;
    if(ndev != prev_ndev) return true;
    else return false;
}

int main(int argc, char **argv) {

    int newdev_delay = 10;

    while(true) {

        string devlist = get_devices();
        string prev_devlist = "";
        bool newdev = false;

        while(!newdev) {
            string devlist = get_devices();
            if(change(devlist, prev_devlist) && prev_devlist != "") {
                cout << "A change was detected" << endl;
            }
            this_thread::sleep_for(chrono::milliseconds(newdev_delay));
            prev_devlist = devlist;
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
