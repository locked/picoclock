include <common.scad>;
include <roundedcube.scad>;

panel_thickness = 3;
support_depth = pcb_interspace - 2;

button_x = 47.85;
button_y = 16;
button_radius = 3;

module outline() {
    translate([-case[0]/2 + thickness, 0, -case[2]/2 + thickness])

    // pcb
    difference() {
        roundedcube([front_panel[0], front_panel[2]/2, front_panel[2]], false, front_panel[2]/4, "y");

        translate([0, panel_thickness, 0]) cube([front_panel[0], front_panel[2]/2, front_panel[2]]);
    }

    screws_support();
}
module screen() {
    translate([-case[0]/2 + thickness, 0, -case[2]/2 + thickness])

    translate([front_panel[0]/2 - screen_visible_size[0]/2, 0, front_panel[1]/2 + screen_visible_size[1]/2 - thickness])
    cube([screen_visible_size[0], screen_visible_size[2]+front_panel[1], screen_visible_size[1]]);
}
module screws_support() {
    // PCB screw holes
    for (i=[0:3]) {
        x = (i < 2 ? 1 : -1) * pcb_screw[0]/2;
        z = (i == 0 || i == 2 ? 1 : -1) * pcb_screw[1]/2;
        translate([x, panel_thickness, z]) rotate([-90]) cylinder(h=support_depth, d=pcb_screw[2] * 2);
    }
}
module screws() {
    // PCB screw holes
    for (i=[0:3]) {
        x = (i < 2 ? 1 : -1) * pcb_screw[0]/2;
        z = (i == 0 || i == 2 ? 1 : -1) * pcb_screw[1]/2;
        translate([x, 0, z]) rotate([-90]) cylinder(h=inner_cyl_h, d=pcb_screw[2]);
    }
}
module buttons() {
    for (i=[0:6]) {
        x = (i < 3 ? 1 : -1) * button_x;
        z = (i % 3) * button_y - button_y;
        translate([x, 0, z]) rotate([-90]) cylinder(h=panel_thickness, d=button_radius*2);
    }
}

// Front Panel
color([0.2, 0.2, 0.9, 0.5]) {
    difference() {
        outline();
        screen();
        screws();
        buttons();
    }
}