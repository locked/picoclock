include <common.scad>;

pcb = "picoclock-Edge_Cuts.svg";

//translate([-case[0]/2-2, -case[1]/2-43, 0])
//linear_extrude(height=2, scale=1)
//  offset(r=0, $fn=5)
//    import(file = pcb, dpi = 300);

screen_holder_screw_rad = 2;
screen_holder_out = [screen_holder_screw_rad*3, 63, 3];
screen_holder_in = [70.4, 39.2, 4];
screen_size = [59.3, 29.3, 1.5];
screen_size_toborder = 7.1;
screen_visible_size = [50, 25, 3];

module support() {
    cube(screen_holder_in, center=true);

    // 2 holders
    translate([-screen_holder_in[0]/2+screen_holder_out[0]/2, 0])
    cube([screen_holder_out[0], screen_holder_out[1], screen_holder_in[2]], center=true);

    translate([+screen_holder_in[0]/2-screen_holder_out[0]/2, 0])
    cube([screen_holder_out[0], screen_holder_out[1], screen_holder_in[2]], center=true);

    // 4 screw supports
    translate([+screen_holder_in[0]/2-screen_holder_out[0]/2, -screen_holder_out[1]/2+screen_holder_out[0]/2, -screen_holder_out[2]])
    cube([screen_holder_out[0], screen_holder_out[0], screen_holder_out[2]], center=true);
    translate([+screen_holder_in[0]/2-screen_holder_out[0]/2, screen_holder_out[1]/2-screen_holder_out[0]/2, -screen_holder_out[2]])
    cube([screen_holder_out[0], screen_holder_out[0], screen_holder_out[2]], center=true);

    translate([-screen_holder_in[0]/2+screen_holder_out[0]/2, -screen_holder_out[1]/2+screen_holder_out[0]/2, -screen_holder_out[2]])
    cube([screen_holder_out[0], screen_holder_out[0], screen_holder_out[2]], center=true);
    translate([-screen_holder_in[0]/2+screen_holder_out[0]/2, screen_holder_out[1]/2-screen_holder_out[0]/2, -screen_holder_out[2]])
    cube([screen_holder_out[0], screen_holder_out[0], screen_holder_out[2]], center=true);
}

difference() {
    support();

    translate([-screen_size_toborder/2, 0, 0]) cube([screen_size[0]+screen_size_toborder, screen_size[1], screen_size[2]], center=true);

    translate([-screen_size[0]/2-screen_size_toborder/2, 0, -2]) cube([screen_size_toborder, screen_size[1], 4], center=true);

    translate([0, 0, 1]) cube(screen_visible_size, center=true);

    // Screw holes
    translate([screen_holder_in[0]/2-screen_holder_out[0]/2, -screen_holder_out[1]/2+screen_holder_screw_rad+1, -screen_holder_out[2]])
    cylinder(h=10, d=4, $fn=20, center=true);
    translate([screen_holder_in[0]/2-screen_holder_out[0]/2, screen_holder_out[1]/2-screen_holder_screw_rad-1, -screen_holder_out[2]])
    cylinder(h=10, d=4, $fn=20, center=true);

    translate([-screen_holder_in[0]/2+screen_holder_out[0]/2, -screen_holder_out[1]/2+screen_holder_screw_rad+1, -screen_holder_out[2]])
    cylinder(h=10, d=4, $fn=20, center=true);
    translate([-screen_holder_in[0]/2+screen_holder_out[0]/2, screen_holder_out[1]/2-screen_holder_screw_rad-1, -screen_holder_out[2]])
    cylinder(h=10, d=4, $fn=20, center=true);
}