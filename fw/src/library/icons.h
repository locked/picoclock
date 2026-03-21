/*
line_struct icon_light[]={
  {1, 14, 16},
  {2, 14, 16},
  {3, 3, 6},
  {3, 14, 16},
  {3, 24, 27},
  {4, 3, 7},
  {4, 14, 16},
  {4, 22, 27},
  {5, 4, 8},
  {5, 14, 16},
  {5, 21, 26},
  {6, 4, 9},
  {6, 14, 16},
  {6, 20, 24},
  {7, 6, 10},
  {7, 14, 16},
  {7, 19, 24},
  {8, 7, 10},
  {8, 19, 22},
  {11, 13, 17},
  {12, 12, 18},
  {13, 11, 19},
  {14, 1, 8},
  {14, 11, 19},
  {14, 22, 29},
  {15, 1, 8},
  {15, 11, 19},
  {15, 22, 29},
  {16, 11, 19},
  {17, 12, 18},
  {18, 13, 17},
  {20, 7, 9},
  {21, 6, 9},
  {21, 20, 23},
  {22, 5, 9},
  {22, 14, 16},
  {22, 20, 24},
  {23, 4, 8},
  {23, 14, 16},
  {23, 21, 25},
  {24, 3, 7},
  {24, 14, 16},
  {24, 22, 26},
  {25, 2, 6},
  {25, 14, 16},
  {25, 23, 27},
  {26, 2, 5},
  {26, 14, 16},
  {26, 23, 27},
  {27, 14, 16},
  {27, 25, 27},
  {28, 14, 16}};

line_struct icon_rightarrow[]={
  {6, 7, 8},
  {7, 7, 10},
  {8, 7, 12},
  {9, 7, 14},
  {10, 7, 16},
  {11, 7, 18},
  {12, 7, 20},
  {13, 7, 22},
  {14, 7, 24},
  {15, 7, 24},
  {16, 7, 22},
  {17, 7, 20},
  {18, 7, 18},
  {19, 7, 16},
  {20, 7, 14},
  {21, 7, 12},
  {22, 7, 10},
  {23, 7, 8}};


line_struct icon_leftarrow[]={
  {6, 22, 23},
  {7, 20, 23},
  {8, 18, 23},
  {9, 16, 23},
  {10, 14, 23},
  {11, 12, 23},
  {12, 10, 23},
  {13, 8, 23},
  {14, 6, 23},
  {15, 6, 23},
  {16, 8, 23},
  {17, 10, 23},
  {18, 12, 23},
  {19, 14, 23},
  {20, 16, 23},
  {21, 18, 23},
  {22, 20, 23},
  {23, 22, 23}};

line_struct icon_wifi[]={
  {8, 10, 19},
  {9, 8, 10},
  {9, 19, 21},
  {10, 6, 8},
  {10, 21, 23},
  {11, 5, 6},
  {11, 23, 24},
  {12, 4, 5},
  {12, 24, 25},
  {13, 3, 4},
  {13, 11, 18},
  {13, 25, 26},
  {14, 9, 11},
  {14, 18, 20},
  {15, 8, 9},
  {15, 20, 21},
  {16, 7, 8},
  {16, 21, 22},
  {17, 6, 7},
  {17, 22, 23},
  {18, 12, 17},
  {19, 10, 13},
  {19, 16, 19},
  {20, 9, 10},
  {20, 19, 20},
  {22, 12, 17},
  {23, 13, 16}};
*/

line_struct icon_leftarrow[] = {
    {4, 8, 10},
    {5, 6, 8},
    {6, 4, 6},
    {7, 2, 4},
    {8, 2, 4},
    {9, 4, 6},
    {10, 6, 8},
    {11, 8, 10}
};

line_struct icon_rightarrow[] = {
    {4, 5, 7},
    {5, 7, 9},
    {6, 9, 11},
    {7, 11, 13},
    {8, 11, 13},
    {9, 9, 11},
    {10, 7, 9},
    {11, 5, 7}
};

line_struct icon_light[] = {
    // Top vertical ray
    {1, 7, 8},
    // Top-left diagonal, top vertical, top-right diagonal
    {2, 2, 3},  {2, 7, 8},  {2, 12, 13},
    {3, 3, 4},              {3, 11, 12},
    // Top of the center sun circle
    {4, 6, 9},
    {5, 5, 10},
    {6, 4, 11},
    // Left horizontal ray, center of sun circle, right horizontal ray
    {7, 1, 2},  {7, 4, 11}, {7, 13, 14},
    {8, 1, 2},  {8, 4, 11}, {8, 13, 14},
    // Bottom of the center sun circle
    {9, 4, 11},
    {10, 5, 10},
    {11, 6, 9},
    // Bottom-left diagonal, bottom-right diagonal
    {12, 3, 4},             {12, 11, 12},
    // Bottom-left diagonal, bottom vertical, bottom-right diagonal
    {13, 2, 3}, {13, 7, 8}, {13, 12, 13},
    // Bottom vertical ray
    {14, 7, 8}
};

line_struct icon_bluetooth[] = {
    {1, 7, 7},
    {2, 7, 8},
    {3, 7, 7}, {3, 9, 9},
    {4, 4, 4}, {4, 7, 7}, {4, 10, 10},
    {5, 5, 5}, {5, 7, 7}, {5, 9, 9},
    {6, 6, 8},
    {7, 7, 7},
    {8, 6, 8},
    {9, 5, 5}, {9, 7, 7}, {9, 9, 9},
    {10, 4, 4}, {10, 7, 7}, {10, 10, 10},
    {11, 7, 7}, {11, 9, 9},
    {12, 7, 8},
    {13, 7, 7}
};

line_struct icon_wifi[] = {
    // Outer arch
    {2, 3, 12},
    {3, 1, 2},  {3, 13, 14},
    {4, 0, 0},  {4, 15, 15},
    // Middle arch
    {6, 4, 11},
    {7, 2, 3},  {7, 12, 13},
    {8, 1, 1},  {8, 14, 14},
    // Inner arch
    {10, 6, 9},
    {11, 5, 5}, {11, 10, 10},
    // Center dot
    {13, 7, 8},
    {14, 7, 8}
};

line_struct icon_escape[] = {
    // Top edge of the door
    {2, 8, 14},
    {3, 8, 14},
    // Upper door frame
    {4, 8, 9},  {4, 13, 14},
    // Arrow top diagonal and door frame
    {5, 4, 5},  {5, 8, 9},  {5, 13, 14},
    {6, 2, 4},  {6, 8, 9},  {6, 13, 14},
    // Arrow shaft/tip and right door frame (left frame is covered by the arrow)
    {7, 1, 10}, {7, 13, 14},
    {8, 1, 10}, {8, 13, 14},
    // Arrow bottom diagonal and door frame
    {9, 2, 4},  {9, 8, 9},  {9, 13, 14},
    {10, 4, 5}, {10, 8, 9}, {10, 13, 14},
    // Lower door frame
    {11, 8, 9}, {11, 13, 14},
    // Bottom edge of the door
    {12, 8, 14},
    {13, 8, 14}
};

line_struct icon_weather[] = {
    // Top sun ray
    {0, 11, 11},
    // Diagonal sun rays and top vertical ray
    {1, 9, 9},  {1, 11, 11}, {1, 13, 13},
    // Top of sun core
    {2, 10, 12},
    // Middle of sun core and horizontal rays
    {3, 9, 13},
    // Bottom of sun core
    {4, 10, 12},
    // Diagonal sun rays and bottom vertical ray
    {5, 9, 9},  {5, 11, 11}, {5, 13, 13},
    // Top of the left cloud bump, plus the bottom-most sun ray
    {6, 4, 6},  {6, 11, 11},
    // Expanding left cloud bump, and top of the right cloud bump
    {7, 3, 7},  {7, 10, 12},
    // Main body of the cloud (valley between bumps is filled in here)
    {8, 2, 13},
    {9, 1, 14},
    {10, 1, 14},
    // Bottom edge of the cloud
    {11, 2, 13}
};

line_struct icon_alarm[] = {
    {0, 7, 8},
    {1, 3, 4},  {1, 7, 8},  {1, 11, 12},
    {2, 2, 5},              {2, 10, 13},
    {3, 3, 4},  {3, 6, 9},  {3, 11, 12},
    {4, 4, 5},              {4, 10, 11},
    {5, 3, 3},  {5, 7, 7},  {5, 12, 12},
    {6, 2, 2},  {6, 7, 7},  {6, 13, 13},
    {7, 2, 2},  {7, 7, 9},  {7, 13, 13}, // Clock hands (pointing to 12 and 3)
    {8, 2, 2},              {8, 13, 13},
    {9, 2, 2},              {9, 13, 13},
    {10, 3, 3},             {10, 12, 12},
    {11, 4, 5},             {11, 10, 11},
    {12, 6, 9},
    {13, 4, 5},             {13, 10, 11}, // Left and right legs
    {14, 3, 4},             {14, 11, 12}
};

line_struct icon_disabled_alarm[] = {
    {0, 7, 8},                          {0, 14, 15},
    {1, 3, 4},  {1, 7, 8},  {1, 11, 11}, {1, 13, 14},
    {2, 2, 5},              {2, 10, 10}, {2, 12, 13},
    {3, 3, 4},  {3, 6, 9},              {3, 11, 12},
    {4, 4, 5},                          {4, 10, 11},
    {5, 3, 3},  {5, 7, 7},  {5, 9, 10},  {5, 12, 12},
    {6, 2, 2},              {6, 8, 9},   {6, 13, 13},
    {7, 2, 2},              {7, 7, 8},   {7, 13, 13},
    {8, 2, 2},              {8, 6, 7},   {8, 13, 13},
    {9, 2, 2},              {9, 5, 6},   {9, 13, 13},
                {10, 4, 5},              {10, 12, 12}, // Slash cuts left border here
                {11, 3, 4},              {11, 10, 11},
    {12, 2, 3}, {12, 6, 9},
    {13, 1, 2}, {13, 4, 5},              {13, 10, 11},
    {14, 0, 1}, {14, 3, 4},              {14, 11, 12},
    {15, 0, 0}
};

line_struct icon_music[] = {
    // Top slanted beam
    {1, 11, 14},
    {2, 9, 14},
    {3, 5, 12},
    {4, 5, 10},
    // Vertical stems
    {5, 5, 6},  {5, 11, 12},
    {6, 5, 6},  {6, 11, 12},
    {7, 5, 6},  {7, 11, 12},
    {8, 5, 6},  {8, 11, 12},
    // Left stem continues, right notehead starts
    {9, 5, 6},  {9, 9, 12},
    // Left stem continues, right notehead body
    {10, 5, 6}, {10, 8, 13},
    {11, 5, 6}, {11, 8, 13},
    // Left notehead starts, right notehead finishes
    {12, 3, 6}, {12, 9, 12},
    // Left notehead body
    {13, 2, 7},
    {14, 2, 7},
    // Left notehead finishes
    {15, 3, 6}
};


icon_struct icons[10] = {
  {icon_light, sizeof(icon_light)},
  {icon_rightarrow, sizeof(icon_rightarrow)},
  {icon_leftarrow, sizeof(icon_leftarrow)},
  {icon_wifi, sizeof(icon_wifi)},
  {icon_bluetooth, sizeof(icon_bluetooth)},
  {icon_escape, sizeof(icon_escape)},
  {icon_weather, sizeof(icon_weather)},
  {icon_alarm, sizeof(icon_alarm)},
  {icon_disabled_alarm, sizeof(icon_disabled_alarm)},
  {icon_music, sizeof(icon_music)},
};
