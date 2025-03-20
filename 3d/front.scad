include <common.scad>;

pcb = "picoclock-Edge_Cuts.svg";

//translate([-case[0]/2-2, -case[1]/2-43, 0])
//linear_extrude(height=2, scale=1)
//  offset(r=0, $fn=5)
//    import(file = pcb, dpi = 300);

screen_holder_screw_rad = 2;
screen_holder_out = [screen_holder_screw_rad*3, 63, 3];
screen_holder_in = [70.4, 39.2, 4];
screen_size_toborder = 7.1;

module support() {
    cube(screen_holder_in, center=true);

    translate([0, 0]) cube([screen_holder_in[0], screen_holder_out[1], screen_holder_in[2]], center=true);
    // 2 holders
    //translate([-screen_holder_in[0]/2+screen_holder_out[0]/2, 0]) cube([screen_holder_out[0], screen_holder_out[1], screen_holder_in[2]], center=true);
    //translate([+screen_holder_in[0]/2-screen_holder_out[0]/2, 0]) cube([screen_holder_out[0], screen_holder_out[1], screen_holder_in[2]], center=true);

    // 4 screw supports
    $x = screen_holder_in[0]/2-screen_holder_out[0]/2;
    $y = -screen_holder_out[1]/2+screen_holder_out[0]/2;
    $z = -screen_holder_out[2];
    translate([$x, $y, $z]) cube([screen_holder_out[0], screen_holder_out[0], screen_holder_out[2]], center=true);
    $y2 = screen_holder_out[1]/2-screen_holder_out[0]/2;
    translate([$x, $y2, $z]) cube([screen_holder_out[0], screen_holder_out[0], screen_holder_out[2]], center=true);

    translate([-screen_holder_in[0]/2+screen_holder_out[0]/2, -screen_holder_out[1]/2+screen_holder_out[0]/2, -screen_holder_out[2]])
    cube([screen_holder_out[0], screen_holder_out[0], screen_holder_out[2]], center=true);
    translate([-screen_holder_in[0]/2+screen_holder_out[0]/2, screen_holder_out[1]/2-screen_holder_out[0]/2, -screen_holder_out[2]])
    cube([screen_holder_out[0], screen_holder_out[0], screen_holder_out[2]], center=true);

    // Support for printing
    for (x = [0:3]) {
        $posy = x < 2 ? -screen_holder_out[1]/2 + screen_holder_out[0]: screen_holder_out[1]/2;
        $mirrorz = x % 2 == 0 ? 1 : 0;
        mirror([$mirrorz,0,0])
            translate([-screen_holder_in[0]/2+screen_holder_out[0], $posy, -1.5])
                rotate([90, 90])
                    linear_extrude(screen_holder_out[0])
                        polygon(points=[[0,0], [screen_holder_out[0]/2, 0], [0, screen_holder_out[0]]], paths=[[0,1,2]]);
    }

    // Add border to block light
    translate([-screen_holder_in[0]/2 + screen_holder_out[0] + 1, -screen_holder_out[1]/2 + 2, screen_holder_in[2]/2]) rotate([90]) linear_extrude(2)
        polygon(points=[
            [0,0],
            [screen_holder_in[0] - screen_holder_out[0]*2 - 2, 0],
            [screen_holder_in[0] - screen_holder_out[0]*2 - 4, 2],
            [2, 2]
        ], paths=[
            [0,1,2,3]
        ]);
    translate([-screen_holder_in[0]/2 + screen_holder_out[0] + 1, screen_holder_out[1]/2, screen_holder_in[2]/2]) rotate([90]) linear_extrude(2)
        polygon(points=[
            [0,0],
            [screen_holder_in[0] - screen_holder_out[0]*2 - 2, 0],
            [screen_holder_in[0] - screen_holder_out[0]*2 - 4, 2],
            [2, 2]
        ], paths=[
            [0,1,2,3]
        ]);

    //translate([0, -screen_holder_out[1]/2 + 1, screen_holder_in[2]/2 + 1]) cube([screen_holder_in[0] - screen_holder_out[0]*2 - 2, 2, 2], center=true);
    //translate([0, screen_holder_out[1]/2 - 1, screen_holder_in[2]/2 + 1]) cube([screen_holder_in[0] - screen_holder_out[0]*2 - 2, 2, 2], center=true);
}

difference() {
    support();

    translate([-screen_size_toborder/2, 0, 0]) cube([screen_size[0]+screen_size_toborder, screen_size[1], screen_size[2]], center=true);

    translate([-screen_size[0]/2-screen_size_toborder/2, 0, -2]) cube([screen_size_toborder, screen_size[1], 4], center=true);

    // Visible screen size
    translate([0, 0, 1]) cube(screen_visible_size, center=true);

    // Remove material on the left to ease printing
    translate([-screen_holder_in[0]/2+5, 0, 1]) cube([10, screen_visible_size[1], screen_visible_size[2]], center=true);

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