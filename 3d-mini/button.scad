include <common.scad>;

hole_diam = 5;
outer_diam = 6;   // pcb holes are 5mm
inner_diam = 5;   // pcb holes are 5mm
depth = 6 + 1.6 + 1;
depth_blocker = 5.5 - 2.3;
depth_visible = 3;
sphere_visible = 0.4;

//btn_size = 2.4;	// Ok, tight
btn_size = 2.5;	// Ok, loose
btn = [btn_size, btn_size, 3];
fente = [1.4, 0.5, 3];

module body() {
    // part on btn
    cylinder(h=depth_blocker, d=inner_diam, $fn=30);
    // blocker
    translate([0, 0, depth_blocker-1]) cylinder(h=1, d=outer_diam+2, $fn=50);
    // visible part
    minkowski()
    {
        translate([0, 0, depth_blocker]) cylinder(h=depth_visible-sphere_visible, d=hole_diam-0.4-sphere_visible*2, $fn=40);
        sphere(sphere_visible);
    }
}

difference() {
    body();

    shiftx = 0.1;

    translate([0-shiftx, 0, btn[2]/2]) cube(btn, center=true);
    translate([-btn[0]/2-fente[0]/2-shiftx, 0, fente[2]/2]) cube(fente, center=true);
    translate([+btn[0]/2+fente[0]/2-shiftx, 0, fente[2]/2]) cube(fente, center=true);
}
