#include <unistd.h>
#include <cmath>

#include <iostream>

#include <vector.h>
namespace serialize {
    std::string serialize(const math::vector& v) {
        return "{\"x\":"+std::to_string(v.x)+",\"y\":"+std::to_string(v.y)+",\"z\":"+std::to_string(v.z)+"}";
    }
};

#include <missioncontrol.h>


int main() {
    mission_control control("/tmp/server.sock");

    readable<int> a("ax", 1);
    readable<int> b("bx", a.data);

    readable<math::vector> angular_acceleration("angular_acceleration", math::vector(0, 0, 0));
    readable<math::vector> angular_velocity("angular_velocity", math::vector(0, 0, 0));
    readable<math::vector> angle("angle", math::vector(1, 0.2, 0));

    control.bind_readable(a);
    control.bind_readable(b);
    control.bind_readable(angular_velocity);
    control.bind_readable(angle);

    double kx = 0.0;
    control.add_writable<double>("filter", kx, [](double& new_value) -> double {
        if(new_value < 0.0) new_value = 0.0;
        else if(new_value > 1.0) new_value = 1.0;
        printf("Changing kx to %f\n", new_value);
        return new_value;
    });
    // control.bind_readable(list);
    a = 2;
    a = 10;
    readable<std::vector<double>> list("l", 1);

    list.data[0] = 0;
    // list.data.push_back(1);
    control.bind_readable(list);

    control.log("Finished init!");

    std::cout << "testing: " << control.build_msg() << std::endl;

    control.advertise();
    usleep(1000000);
    return -1;

    int i = 0;
    int j = 0;
    control.tick();
    while(true) {
        angular_acceleration = *angle * -1;
        angular_velocity = *angular_velocity + *angular_acceleration * 0.01;
        angle = *angle + *angular_velocity * 0.01;
        i ++;
        if(i %24 == 23) {
            j++;
            control.log("Logging "+std::to_string(j));
        }
        usleep(1000000/24);
        control.tick();
    }
    // double a[6];
    // double filter[6];
    // // control.bind_continous_output("ax", a);
    // control.bind_readable("ax", a[0]);
    // control.bind_readable("ay", a[1]);
    // control.bind_readable("az", a[2]);
    // control.bind_readable("vr", a[3]);
    // control.bind_readable("vy", a[4]);
    // control.bind_readable("vp", a[5]);
    // control.add_writable("filter-freq", filter[0], []() {

    // });

    // double angles[3] = {0, 0, 0};
    // control.bind_readable("roll", angles[0]);
    // control.bind_readable("pitch", angles[1]);
    // control.bind_readable("yaw", angles[2]);
    // double t = 0.0;
    // double eng = 0.0;
    // control.bind_readable("time", t);
    // control.bind_readable("engpwr", eng);
    // double throttle = 0.0;
    // control.add_writable("throttle", throttle, []() {

    // });
    // while(true) {

    //     a[0] = ((double) rand()) / RAND_MAX - 0.5;
    //     a[1] = ((double) rand()) / RAND_MAX - 0.5;
    //     a[2] = ((double) rand()) / RAND_MAX - 0.5;
    //     a[3] += 0.1 * (((double) rand()) / RAND_MAX - 0.5);
    //     a[4] += 0.1 * (((double) rand()) / RAND_MAX - 0.5);
    //     a[5] += 0.1 * (((double) rand()) / RAND_MAX - 0.5);

    //     angles[0] += 0.01 * a[3];
    //     angles[1] += 0.01 * a[4];
    //     angles[2] += 0.01 * a[5];

    //     t += 0.1;
    //     eng = sin(t) * 0.1 + 0.3;

    //     usleep(1000000/24);
    //     control.tick();
    // }
}