// PCB size
pcb = [140, 54, 1.6];
pcb_screw_diam = 3.6;
pcb_screw_pos = [4, 4];
pcb_bolt_diam = 7;
pcb_bolt_depth = 3.2;
pcb_bolt_length = 40;
pcb_image = "picoclock_scaled.png";
pcb_margin = 0.4;

// general thickness
t = 1.5;

front = [pcb[0] + t * 2, pcb[1] + t * 2, 12];
back = [pcb[0] + t * 2, pcb[1] + t * 2, 50];
pcb_bolt_back_depth = back[2] - pcb_bolt_length + t + pcb_bolt_depth + front[2];

pcb_support = [pcb_screw_pos[0]*2+pcb_margin/2, pcb_screw_pos[0]*2+pcb_margin/2, front[2] - pcb[2] - t];

joint_margin = 0.2;
joint_width = t/2 - joint_margin;

screen_size = [59.3, 29.6, 1.5];
screen_visible_size = [52, 27.4, 3];

button_x = 50;
button_y = 13;
button_radius = 3;

usbc = [4, 10.7, 10.3];
usba = [6, 15.5, 16.3];
sd = [3, 15.7, 19.7];
jack = [4.5, 6.3, 14];
usbc_pos = 9.6;  // from center
usba_pos = 8.14; // from center
sd_pos = 7;
jack_pos = 9.85;

speaker_plate = [132, 10, 19];
speaker = [100, 36, 28];
speaker_support = [11, pcb_support[1], 22];

speaker2 = [51.5, 19.5, 30.3];
speaker2_baffle_diam = 47;
speaker2_screw_diam = 60;
speaker2_screw_from_center = speaker2_screw_diam / 2 * sqrt(2) / 2;
speaker2_support = [52, 52, 2];

screen_holder_screw_rad = 2;
screen_holder_out = [screen_holder_screw_rad*3, pcb[1] - 0.2, 3];
//screen_holder_in = [70.4, 39.2, 4];
screen_holder_in = [68.4, 39.2, 4];
screen_size_toborder = 7.1;


// pin position:
// 32.2 / 24
// center: 144 / 2, 58 / 2
// => 72 +/- 31.2 => 40.8 / 103.2
// => 29 +- 24 => 5 / 53
// +80:
// 120.8 / 183.2
// 85 / 133
