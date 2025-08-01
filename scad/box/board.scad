// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <ProjectBox/project_box.scad>

board_thickness = 1.6;
pad_space = 2.54;
board_dims = [23.4*pad_space, 16.6*pad_space, board_thickness];

module Plant1337_to_oled_frame() {
  u = pad_space;
  translate([u*23-0.5, 0.6, 13]) rotate([0, 0, 90]) children();
}

module oled() {
  s = inch * 0.1;
  pin_len = 4;
  color("silver") {
    for (i = [0: 3]) {
      translate([s*i+pad_space, pad_space/2, -pin_len]) cube([0.25, 0.25, pin_len]);
    }
  }
  color("gray") cube([12, 38, 1]);
  translate([0, 5.5, 1]) color("black") cube([12, 28, 1]);
}

module Plant1337_board() {
  u = pad_space;

  // Board imported from KiCad (VRML) -> meshlab
  translate([board_dims[0]/2, board_dims[1]/2, board_dims[2]/2]) color("white")
    import(file="Plant1337.stl", convexity=3);

  Plant1337_to_oled_frame() oled();
}
