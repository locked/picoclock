case_size_factor = 1.8;
case = [100*case_size_factor, 61*case_size_factor, 38*case_size_factor];

thickness = 2;

inner_cyl_radius = 6;
inner_cyl_shift = 10; //$case[2]/5;

screen = [60, 3, 30];

speaker = [85, 60, 45];
speaker_screw_dist = 52;

vent = 40;

pcb_screw = [157, 48, 2.3]; // width, height, diam
usb = [9, 3.5];
usb_from_middle = 3.5;

pin_header_height = 8;
pin_socket_height = 3;
pcb_interspace = pin_header_height + pin_socket_height;
pcb_thickness = 1.6;

front_panel = [case[0] - thickness - 1, pcb_thickness * 2 + pcb_interspace , case[2] - thickness - 1];