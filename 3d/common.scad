case_size_factor = 1.8;
case = [100*case_size_factor, 61*case_size_factor, 38*case_size_factor];

thickness = 2;

inner_cyl_radius = 6.5;
inner_cyl_shift = 10; //$case[2]/5;
inner_cyl_x = case[0]/2-inner_cyl_shift-0.5;
inner_cyl_z = case[2]/2-inner_cyl_shift+0.1;
inner_cyl_h = 18;

screen_size = [59.3, 29.3, 1.5];
screen_visible_size = [52, 27, 3];

speaker = [68, 60, 45]; // diag 85
speaker_screw_dist = 52;

foot_radius = 10;
foot_dist_factor = [0.7, 0.85];

vent = 50;

pcb_screw = [157, 48, 3.5]; // width, height, diam
pcb_screw_insert = [3, 7.9]; // height, diam
usb = [11, 3.5];
usb_from_middle = 2;
sdcard = [15, 3];

pin_header_height = 8;
pin_socket_height = 3;
pcb_interspace = pin_header_height + pin_socket_height;
pcb_thickness = 1.6;

front_panel = [case[0] - thickness * 2, pcb_thickness * 2 + pcb_interspace, case[2] - thickness * 2];

screen_holder_screw_rad = 2;
screen_holder_out = [screen_holder_screw_rad*3, 63, 3];
screen_holder_in = [70.4, 39.2, 4];
screen_size_toborder = 7.1;
