include <common.scad>;

outer_diam = 4.3;   // pcb holes are 5mm
depth = 6 + 1.6 + 1;

btn = [2, 2, 3];
fente = [0.8, 0.5, 2];

module body() {
    cylinder(h=depth, d=outer_diam, $fn=30);
    translate([0, 0, depth]) cylinder(h=1, d=outer_diam+2, $fn=30);
}

difference() {
    body();
    translate([0, 0, btn[2]/2]) cube(btn, center=true);
    translate([-btn[0]/2-fente[0]/2, 0, fente[2]/2]) cube(fente, center=true);
    translate([+btn[0]/2+fente[0]/2, 0, fente[2]/2]) cube(fente, center=true);
}
