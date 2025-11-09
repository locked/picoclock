// PCB size
pcb = [140, 54, 1.6];
pcb_screw_diam = 3.2;
pcb_screw_pos = [4, 4];
pcb_image = "picoclock_scaled.png";

// general thickness
t = 2;

front = [pcb[0] + t * 2, pcb[1] + t * 2, 12];

pcb_support = [pcb_screw_pos[0]*2, front[2] - t - pcb[2], front[2] - pcb[2] - t];

screen_size = [59.3, 29.3, 1.5];
screen_visible_size = [52, 27, 3];

button_x = 50;
button_y = 13;
button_radius = 3;

usbc = [4, 10.7];
usba = [6, 15.5];
sd = [3, 15.7];
usbc_pos = 9.6;  // from center
usba_pos = 8.14; // from center
sd_pos = 0;


screen_holder_screw_rad = 2;
screen_holder_out = [screen_holder_screw_rad*3, pcb[1], 3];
screen_holder_in = [70.4, 39.2, 4];
screen_size_toborder = 7.1;
