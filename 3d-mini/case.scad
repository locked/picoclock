// PCB size
pcb = [140, 54, 1.6];
pcb_screw_diam = 3.2;
pcb_screw_pos = [4, 4];
pcb_image = "picoclock_scaled.png";

// general thickness
t = 2;

front = [pcb[0] + t * 2, pcb[1] + t * 2, 12];

pcb_support = [pcb_screw_pos[0]*2, front[2] - t - pcb[2], pcb_screw_pos[1]*2];

screen_size = [59.3, 29.3, 1.5];
screen_visible_size = [52, 27, 3];

button_x = 50;
button_y = 13;
button_radius = 3;

usbc = [4, 10.7];
usba = [6, 15.5];
sd = [3, 15.7];
usbc_pos = 9.6;  // from center
usba_pos = 8.14; // from center
sd_pos = 0;

module pcb() {
    rotate([90]) translate([t, t]) difference() {
        cube(pcb);
        for (x = [0:1]) {
            for (y = [0:1]) {
                translate([pcb_screw_pos[0] + x * (pcb[0]-pcb_screw_pos[0]*2), pcb_screw_pos[0] + y * (pcb[1]-pcb_screw_pos[1]*2)]) cylinder(pcb[2], pcb_screw_diam/2, pcb_screw_diam/2, $fn=15);
            }
        }
    }
}

module front_screw() {
    // PCB support screw
    for (x = [0:1]) {
        for (y = [0:1]) {
            translate([
                t + pcb_screw_pos[0] + x * (pcb[0] - pcb_screw_pos[0] - t*2),
                t + pcb_screw_pos[1] + y * (pcb[1] - pcb_screw_pos[1] - t*2)],
                pcb[2]) cylinder(front[2], pcb_screw_diam/2, pcb_screw_diam/2, $fn=15);

            translate([
                t + pcb_screw_pos[0] + x * (pcb[0] - pcb_screw_pos[0] - t*2),
                t + pcb_screw_pos[1] + y * (pcb[1] - pcb_screw_pos[1] - t*2),
                front[2] - 3]) cylinder(3, 3, 3, $fn=6);
        }
    }
}
module front() {
    front_inner = [pcb[0], pcb[1], front[2] - t];
    rotate([90]) difference() {
        cube(front);
        translate([t, t]) cube(front_inner);
        front_screw();
        screen();
        buttons();
        io();
    }
    // add PCB support (cannot do it earlier
    // since it overlap with main body and
    // difference does not handle that well)
    rotate([90]) difference() {
        for (x = [0:1]) {
            for (y = [0:1]) {
                translate([
                    t + x * (front[0] - pcb_support[0] - t*2),
                    t + y * (front[1] - pcb_support[2] - t*2),
                    pcb[2]]) cube(pcb_support);
            }
        }
        front_screw();
    }
}

module io() {
    // usb-c
    translate([
        0,
        front[1]/2 - usbc_pos - usbc[1] / 2,
        pcb[2],
    ]) cube([
        t,
        usbc[1],
        usbc[0]
    ]);

    // usb-a
    translate([
        0,
        front[1]/2 + usba_pos - usba[1] / 2,
        pcb[2],
    ]) cube([
        t,
        usba[1],
        usba[0]
    ]);

    // SD card
    translate([
        front[0] - t,
        front[1]/2 - sd[1]/2,
        pcb[2],
    ]) cube([
        t,
        sd[1],
        sd[0]
    ]);
}

module screen() {
    translate([
        front[0]/2 - screen_visible_size[0]/2,
        front[1]/2 - screen_visible_size[1]/2,
        front[2]-t,
    ]) cube([
        screen_visible_size[0],
        screen_visible_size[1],
        screen_visible_size[2]
    ]);
}

module buttons() {
    for (i=[0:6]) {
        x = (i < 3 ? 1 : -1) * button_x;
        y = (i % 3) * button_y - button_y;
        translate([front[0]/2 + x, front[1]/2 + y, front[2] - t]) cylinder(h=t, d=button_radius*2);
    }
}

module back() {
}

//pcb();

front();

back();

//rotate([90]) translate([t, t]) color("red") surface(file=pcb_image, convexity = 1);
