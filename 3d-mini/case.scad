include <common.scad>;
include <screen_holder.scad>;


module pcb() {
    rotate([90]) translate([t, t]) difference() {
        cube(pcb);
        for (x = [0:1]) {
            for (y = [0:1]) {
                translate([pcb_screw_pos[0] + x * (pcb[0]-pcb_screw_pos[0]*2), pcb_screw_pos[0] + y * (pcb[1]-pcb_screw_pos[1]*2)]) cylinder(pcb[2], pcb_screw_diam/2, pcb_screw_diam/2, $fn=15);
            }
        }

        // Pins holes
        for (x = [0:1]) {
            for (y = [0:1]) {
                cyl_x = (x == 0 ? 1 : -1) * screen_holder_in[0]/2 + (x == 0 ? -1 : 1) * screen_holder_out[0]/2;
                cyl_y = (y == 0 ? 1 : -1) * screen_holder_out[1]/2 + (y == 0 ? -1 : 1) * screen_holder_screw_rad + (y == 0 ? -1 : 1) * 1;
                echo(cyl_x, cyl_y);
                translate([
                    cyl_x - t,
                    cyl_y - t,
                    -15])
                translate([front[0]/2, front[1]/2, front[2]+2.7])
                cylinder(h=3, d=2.5, $fn=20);
            }
        }
    }

    rotate([90]) io();
}

module front_screw() {
    // PCB support screw
    for (x = [0:1]) {
        for (y = [0:1]) {
            translate([
                (x == 0 ? pcb[0] - pcb_screw_pos[0] : pcb_screw_pos[0]) + t,
                (y == 0 ? pcb[1] - pcb_screw_pos[1] : pcb_screw_pos[1]) + t,
                0
            ]) cylinder(front[2], pcb_screw_diam/2, pcb_screw_diam/2, $fn=15);

            translate([
                (x == 0 ? pcb[0] - pcb_screw_pos[0] : pcb_screw_pos[0]) + t,
                (y == 0 ? pcb[1] - pcb_screw_pos[1] : pcb_screw_pos[1]) + t,
                front[2] - pcb_bolt_depth]) cylinder(h=pcb_bolt_depth, r=pcb_bolt_diam/2, $fn=15);
        }
    }
}
module front() {
    joint_margin = 0.2;
    front_inner = [pcb[0] + pcb_margin, pcb[1] + pcb_margin, front[2] - t];
    joint_width = t/2 - joint_margin;

    rotate([90]) difference() {
        union(){
            cube(front);
            // front/back joint
            /*
            for (i = [0:1]) {
                translate([
                    0,
                    i * (front[1] - 1),
                    -1]) cube([front[0],1,1]);
            }
            for (i = [0:1]) {
                translate([
                    i * (front[0] - 1),
                    0,
                    -1]) cube([1,front[1],1]);
            }
            */

            // back joint
            for (i = [0:1]) {
                // horizontal
                translate([
                    t/2 + joint_margin,
                    i * front[1] + (i == 0 ? 1 : -2) * joint_width + (i == 0 ? 1 : -1) * joint_margin,
                    -1+joint_margin]) cube([
                        front[0]-t-joint_margin*2,
                        joint_width,
                        1-joint_margin
                    ]);
            }
            for (i = [0:1]) {
                // vertical
                translate([
                    i * front[0] + (i == 0 ? 1 : -2) * joint_width + (i == 0 ? 1 : -1) * joint_margin,
                    t/2 + joint_margin,
                    -1+joint_margin]) cube([
                        joint_width,
                        front[1]-t-joint_margin*2,
                        1-joint_margin
                    ]);
            }
        }
        translate([t-pcb_margin/2, t-pcb_margin/2]) cube(front_inner);
        front_screw();
        screen();
        buttons();
        io();
    }

    // screen holder pins
    rotate([90]) translate([
        front[0]/2,
        front[1]/2,
        front[2]-t
    ]) screen_holder_pins();
    // screen stop pin
    rotate([90]) translate([
        front[0]/2 - screen_holder_out[1]/2 - 8.3,
        16,
        front[2]-t-5])
    cube([3,3,5]);

    // add PCB support (cannot do it earlier
    // since it overlap with main body and
    // difference does not handle that well)
    rotate([90]) difference() {
        for (x = [0:1]) {
            for (y = [0:1]) {
                translate([
                    t + x * (front[0] - pcb_support[0] - t*2 + pcb_margin / 2),
                    t + y * (front[1] - pcb_support[1] - t*2 + pcb_margin / 2),
                    pcb[2]]) cube(pcb_support);
            }
        }
        front_screw();
    }
}

module io(forback=false) {
    // usb-c
    translate([
        0,
        front[1]/2 - usbc_pos - usbc[1] / 2,
        -usbc[0],
    ]) cube([usbc[2], usbc[1], usbc[0] + (forback ? 1 : 0)]);

    // usb-a
    translate([
        0,
        front[1]/2 + usba_pos - usba[1] / 2,
        -usba[0],
    ]) cube([usba[2], usba[1], usba[0] + (forback ? 1 : 0)]);

    // SD card
    translate([
        front[0] - sd[2],
        front[1]/2 + sd_pos - sd[1]/2,
        -sd[0],
    ]) cube([sd[2], sd[1], sd[0] + (forback ? 1 : 0)]);

    // Jack
    translate([
        front[0] - jack[2],
        front[1]/2 - jack_pos - jack[1]/2,
        -jack[0],
    ]) cube([jack[2], jack[1], jack[0] + (forback ? 1 : 0)]);
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

module screen_holder_pins() {
    // Pins toward PCB
    pin_height = 1.6;
    for (x = [0:1])  {
        for (y = [0:1]) {
            cyl_x = (x == 0 ? 1 : -1) * screen_holder_in[0]/2 + (x == 0 ? -1 : 1) * screen_holder_out[0]/2;
            cyl_y = (y == 0 ? 1 : -1) * screen_holder_out[1]/2 + (y == 0 ? -1 : 1) * screen_holder_screw_rad + (y == 0 ? -1 : 1) * 1;
            //echo(cyl_x, cyl_y);
            translate([
                cyl_x,
                cyl_y,
                -pin_height])
            cylinder(h=pin_height, d1=1.8, d2=2, $fn=20);
        }
    }
}



module back_screw() {
    // PCB support screw
    for (x = [0:1]) {
        for (y = [0:1]) {
            translate([
                (x == 0 ? pcb[0] - pcb_screw_pos[0] : pcb_screw_pos[0]) + t,
                (y == 0 ? pcb[1] - pcb_screw_pos[1] : pcb_screw_pos[1]) + t,
                -back[2]-t])
            cylinder(h=back[2]+t+1, r=pcb_screw_diam/2, $fn=15);

            // bolt
            translate([
                (x == 0 ? pcb[0] - pcb_screw_pos[0] : pcb_screw_pos[0]) + t,
                (y == 0 ? pcb[1] - pcb_screw_pos[1] : pcb_screw_pos[1]) + t,
                -back[2] - pcb_bolt_depth])
            cylinder(h=pcb_bolt_back_depth, r=pcb_bolt_diam/2, $fn=6);
        }
    }
}
module speaker2_holes() {
    for (x = [0:1]) {
        for (y = [0:1]) {
            translate([
                (x == 0 ? -1 : 1) * speaker2_screw_from_center,
                (y == 0 ? -1 : 1) * speaker2_screw_from_center,
                -t-speaker2_support[2]
            ]) cylinder(h=10, r=1.6, $fn=10);
        }
    }
}
module speaker2_support() {
    difference() {
        translate([-speaker2[0]/2, -speaker2[0]/2, 0]) cube([speaker2[0], speaker2[0], speaker2_support[2]]);
        speaker2_holes();
        cylinder(h=speaker2_support[2], r=speaker2_baffle_diam/2 + 1, $fn=30);
    }
}
module speaker2() {
    difference() {
        union() {
            cylinder(speaker2[2], speaker2[0]/2, speaker2[1]/2);
            translate([-speaker2[0]/2, -speaker2[0]/2]) cube([speaker2[0], speaker2[0], 1]);
        }
        speaker2_holes();
    }
}
module speaker() {
    cube(speaker);
    translate([-(speaker_plate[0]-speaker[0])/2, 0, -(speaker_plate[2]-speaker[2])])
    cube(speaker_plate);
}
module speaker_grid() {
    startx = (back[0]-speaker[0])/2 - t/2;
    endx = startx + speaker[0];
    starty = (back[1]-speaker2[0])/2 + speaker2[0] * 0.14;
    endy = starty + speaker2[0] * 0.7;
    step = 5;
    for (x = [startx:step:endx]) {
        for (y = [starty:step:endy]) {
            translate([x, y, -back[2]])
            cube([2,2,t]);
        }
    }
}
module speaker_supports_holes() {
    rotate([90]) translate([
        pcb_support[0]+t+3+1,
        -back[2]+speaker[2]+t-10-4,
        -speaker_support[1]-t
    ]) cylinder(h=speaker_support[1]+t, r=3);
    rotate([90]) translate([
        back[0]-pcb_support[0]-t-5+1,
        -back[2]+speaker[2]+t-10-4,
        -speaker_support[1]-t
    ]) cylinder(h=speaker_support[1]+t, r=3);

    rotate([90]) translate([
        pcb_support[0]+t+3+1,
        -back[2]+speaker[2]+t-10-4,
        -speaker_support[1]-t+6
    ]) cylinder(h=5, r=6);
    rotate([90]) translate([
        back[0]-pcb_support[0]-t-5+1,
        -back[2]+speaker[2]+t-10-4,
        -speaker_support[1]-t+6
    ]) cylinder(h=5, r=6);
}
module speaker_supports() {
    rotate([90]) difference() {
        union() {
            translate([
                pcb_support[0]+t,
                t,
                -back[2]-(speaker_plate[2]-speaker[2])-7.5
            ]) cube(speaker_support);
            translate([
                back[0]-pcb_support[0]-t-speaker_support[0],
                t,
                -back[2]-(speaker_plate[2]-speaker[2])-7.5
            ]) cube(speaker_support);
        }
        speaker_supports_holes();
    }
}
module back() {
    back_inner = [pcb[0], pcb[1], back[2] - t];
    joint_margin = 0.2;
    rotate([90]) difference() {
        union(){
            translate([0, 0, -back[2]]) cube(back);
            // ADD FRONT JOINT
        }
        translate([t, t, -back[2] + t]) cube(back_inner);
        io(true);
        back_screw();
        speaker_grid();
        speaker_supports_holes();
        translate([back[0]/2, back[1]/2, -back[2] + t + speaker2_support[2]]) speaker2_holes();
    }

    // add PCB support (cannot do it earlier
    // since it overlap with main body and
    // difference does not handle that well)
    rotate([90]) difference() {
        for (x = [0:1]) {
            for (y = [0:1]) {
                translate([
                    t + x * (front[0] - pcb_support[0] - t*2),
                    t + y * (front[1] - pcb_support[1] - t*2),
                    -back[2]])
                cube([pcb_support[0], pcb_support[1], back[2]+0.8]);
            }
        }
        back_screw();
        speaker_supports_holes();
    }

    //rotate([90]) translate([back[0]/2, back[1]/2, -back[2] + t + speaker2_support[2]]) speaker2();
    rotate([90]) translate([back[0]/2, back[1]/2, -back[2] + t]) speaker2_support();

    //rotate([90]) translate([(back[0]-speaker[0])/2, pcb_support[0] + t, -back[2]]) speaker();
    speaker_supports();
}

//pcb();

front();

//rotate([90]) translate([front[0]/2, front[1]/2, 6]) screen_holder();

//back();

//rotate([90]) translate([t, t]) color("red") surface(file=pcb_image, convexity = 1);
