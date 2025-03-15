include <common.scad>;
include <roundedcube.scad>;

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
}