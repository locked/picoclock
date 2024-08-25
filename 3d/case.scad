include <common.scad>;
include <roundedcube.scad>;

pcb = "picoclock-Edge_Cuts.svg";


module speaker_support() {
    internal_diam = speaker[1];
    height = 3;

    color("Red")
    translate([-speaker[0]/2, ($case[1]+front_panel[1])/2-speaker[0]/2, -$case[2]/2+thickness])

    difference() {
        cube([speaker[0], speaker[0], height]);
        translate([speaker[0]/2, speaker[0]/2, 0])
        cylinder(h=height, d=internal_diam+1);  // 1 for margin

        screw_dist_from_side = (speaker[0] - speaker_screw_dist) / 2;
        x1 = screw_dist_from_side;
        x2 = speaker[0] - screw_dist_from_side;
        y1 = screw_dist_from_side;
        y2 = speaker[0] - screw_dist_from_side;
        translate([x1, y1, 0]) cylinder(h=height, d=3);
        translate([x2, y1, 0]) cylinder(h=height, d=3);
        translate([x1, y2, 0]) cylinder(h=height, d=3);
        translate([x2, y2, 0]) cylinder(h=height, d=3);
    }
}

module speaker_screws() {
    x1 = $case[0]/2-speaker_screw_dist/2;
    x2 = $case[0]/2+speaker_screw_dist/2;
    y1 = ($case[1]+front_panel[1])/2-speaker_screw_dist/2;
    y2 = ($case[1]+front_panel[1])/2+speaker_screw_dist/2;
    translate([x1, y1, 0]) cylinder(h=thickness+1, d=3);
    translate([x2, y1, 0]) cylinder(h=thickness+1, d=3);
    translate([x1, y2, 0]) cylinder(h=thickness+1, d=3);
    translate([x2, y2, 0]) cylinder(h=thickness+1, d=3);
}

module speaker_vents() {
	count = 10;
	for (number = [0:(count-1)]){
        translate([$case[0]/2-speaker[2]/2 + speaker[2]/count/4 + number * (speaker[2]/count), ($case[1]+front_panel[1])/2-speaker[2]/2, 0])
        cube([speaker[2]/count/2, speaker[2], thickness*2]);
    }
}

module feets() {
    t = [$case[0]*foot_dist_factor[0]/2, $case[1]*foot_dist_factor[1], -$case[2]/2-thickness/2];
    d = foot_radius+thickness;
    d2 = foot_radius;
    difference() {
        translate(t) cylinder(h=1, d=d);
        translate(t) cylinder(h=1, d=d2);
    }
    difference() {
        translate([-t[0], t[1], t[2]]) cylinder(h=1, d=d);
        translate([-t[0], t[1], t[2]]) cylinder(h=1, d=d2);
    }
    difference() {
        translate([t[0], $case[1] - $case[1]*foot_dist_factor[1], t[2]]) cylinder(h=1, d=d);
        translate([t[0], $case[1] - $case[1]*foot_dist_factor[1], t[2]]) cylinder(h=1, d=d2);
    }
    difference() {
        translate([-t[0], $case[1] - $case[1]*foot_dist_factor[1], t[2]]) cylinder(h=1, d=d);

        translate([-t[0], $case[1] - $case[1]*foot_dist_factor[1], t[2]]) cylinder(h=1, d=d2);
    }
}

module vents() {
	count = 20;
	for (number = [0:(count-1)]){
        translate([$case[0]/2-vent + number * ((vent * 2)/count), $case[1] - thickness - 1, $case[2]/2 - vent/4])
        cube([vent/count/2, vent, vent/2]);
    }
}

module case($case) {
    translate([-$case[0]/2, 0, -$case[2]/2])

    difference() {
        color("Grey")
        roundedcube([$case[0], $case[1], $case[2]], false, $case[2]/4, "y");
        translate([thickness, -thickness, thickness])
        color("Green")
        roundedcube([$case[0]-thickness*2, $case[1], $case[2]-thickness*2], false, $case[2]/4, "y");

        // Speaker screw
        speaker_screws();

        // Speaker vents
        speaker_vents();

        // USB
        translate([0, front_panel[1], $case[2]/2-usb[0]-usb_from_middle]) cube([thickness*2, usb[1], usb[0]]);

        // Thermal vents
        vents();

        // SD card
    }

    // pcb additional support
    translate([-15, front_panel[1], -$case[2]/2 + thickness]) cube([30, 5, 3]);
    translate([-15, front_panel[1], $case[2]/2 - thickness - 3]) cube([30, 5, 3]);

    difference() {
        color("Green") {
            translate([-($case[0]/2-inner_cyl_shift), front_panel[1], -($case[2]/2-inner_cyl_shift)])
            rotate([-90]) cylinder(h=$case[1]-front_panel[1]-thickness, r=inner_cyl_radius);
            translate([+($case[0]/2-inner_cyl_shift), front_panel[1], -($case[2]/2-inner_cyl_shift)])
            rotate([-90]) cylinder(h=$case[1]-front_panel[1]-thickness, r=inner_cyl_radius);
            translate([-($case[0]/2-inner_cyl_shift), front_panel[1], +($case[2]/2-inner_cyl_shift)])
            rotate([-90]) cylinder(h=$case[1]-front_panel[1]-thickness, r=inner_cyl_radius);
            translate([+($case[0]/2-inner_cyl_shift), front_panel[1], +($case[2]/2-inner_cyl_shift)])
            rotate([-90]) cylinder(h=$case[1]-front_panel[1]-thickness, r=inner_cyl_radius);
        }

        color("Red") {
            // PCB screw holes insert
            translate([-pcb_screw[0]/2, front_panel[1], -pcb_screw[1]/2])
            rotate([-90]) cylinder(h=pcb_screw_insert[0], d=pcb_screw_insert[1], $fn=6);
            translate([pcb_screw[0]/2, front_panel[1], -pcb_screw[1]/2])
            rotate([-90]) cylinder(h=pcb_screw_insert[0], d=pcb_screw_insert[1], $fn=6);
            translate([-pcb_screw[0]/2, front_panel[1], pcb_screw[1]/2])
            rotate([-90]) cylinder(h=pcb_screw_insert[0], d=pcb_screw_insert[1], $fn=6);
            translate([pcb_screw[0]/2, front_panel[1], pcb_screw[1]/2])
            rotate([-90]) cylinder(h=pcb_screw_insert[0], d=pcb_screw_insert[1], $fn=6);

            // PCB screw holes
            translate([-pcb_screw[0]/2, front_panel[1], -pcb_screw[1]/2])
            rotate([-90]) cylinder(h=20, d=pcb_screw[2]);
            translate([pcb_screw[0]/2, front_panel[1], -pcb_screw[1]/2])
            rotate([-90]) cylinder(h=20, d=pcb_screw[2]);
            translate([-pcb_screw[0]/2, front_panel[1], pcb_screw[1]/2])
            rotate([-90]) cylinder(h=20, d=pcb_screw[2]);
            translate([pcb_screw[0]/2, front_panel[1], pcb_screw[1]/2])
            rotate([-90]) cylinder(h=20, d=pcb_screw[2]);
        }
    }

    // Feet
    feets();

    // Speaker
    speaker_support();
    //translate([0, (case[1]+front_panel[1])/2, -front_panel[2]/2-2])
    //cylinder(h=speaker[2], r1=speaker[0]/2, r2=speaker[1]/2);

    /*
    // Front Panel
    color([0.2, 0.2, 0.9, 0.5]) {
        translate([-front_panel[0]/2, 0, -front_panel[2]/2])
        difference() {
            // pcb
            roundedcube([front_panel[0], front_panel[2]/2, front_panel[2]], false, front_panel[2]/4, "y");
            translate([0, front_panel[1], 0]) cube([front_panel[0], front_panel[2]/2 - front_panel[1], front_panel[2]]);
            
            // screen
            translate([front_panel[0]/2 - screen[0]/2, 0, front_panel[2]/2 - screen[2]/2])
            cube([screen[0], screen[1]+front_panel[1], screen[2]]);
        }
    }*/
}

case(case);

/*
rotate([-90])
translate([-case[0]/2-2,-case[1]/2-43,0])
linear_extrude(height=2, scale=1)
  offset(r=0, $fn=5)
    import(file = pcb, dpi = 300);
*/