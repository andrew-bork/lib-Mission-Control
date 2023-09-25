#include <missioncontrol.h>
#include <unistd.h>
#include <cmath>

int main() {
    mission_control control("/tmp/server.sock");
    double a[6];
    double filter[6];
    // control.bind_continous_output("ax", a);
    control.bind_readable("ax", a[0]);
    control.bind_readable("ay", a[1]);
    control.bind_readable("az", a[2]);
    control.bind_readable("vr", a[3]);
    control.bind_readable("vy", a[4]);
    control.bind_readable("vp", a[5]);
    control.add_writable("filter-freq", filter[0], []() {

    });

    double angles[3] = {0, 0, 0};
    control.bind_readable("roll", angles[0]);
    control.bind_readable("pitch", angles[1]);
    control.bind_readable("yaw", angles[2]);
    double t = 0.0;
    double eng = 0.0;
    control.bind_readable("time", t);
    control.bind_readable("engpwr", eng);
    double throttle = 0.0;
    control.add_writable("throttle", throttle, []() {

    });
    while(true) {

        a[0] = ((double) rand()) / RAND_MAX - 0.5;
        a[1] = ((double) rand()) / RAND_MAX - 0.5;
        a[2] = ((double) rand()) / RAND_MAX - 0.5;
        a[3] += 0.1 * (((double) rand()) / RAND_MAX - 0.5);
        a[4] += 0.1 * (((double) rand()) / RAND_MAX - 0.5);
        a[5] += 0.1 * (((double) rand()) / RAND_MAX - 0.5);

        angles[0] += 0.01 * a[3];
        angles[1] += 0.01 * a[4];
        angles[2] += 0.01 * a[5];

        t += 0.1;
        eng = sin(t) * 0.1 + 0.3;

        usleep(1000000/24);
        control.tick();
    }
}