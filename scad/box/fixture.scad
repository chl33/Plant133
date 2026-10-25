// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

// This is a fixture for drilling missing screw holes in latest PCB.
board_thickness = 1.6;
pad_space = 2.54;
num_boards = 5;
board_dims = [23.4*pad_space-0.1, 16.6*pad_space, board_thickness * num_boards];

outside_border = 2;


base_thickness = 1.5;

outside_dims = board_dims + [2*outside_border, 2*outside_border, base_thickness];

if (false) {
  translate([board_dims[0]/2, board_dims[1]/2, board_dims[2]/2 + base_thickness]) color("white")
    import(file="Plant1337.stl", convexity=3);
 }

dscrew = 2.5;
screw_offset = dscrew + 0.2;

translate([-outside_border, -outside_border, 0]) {
  difference() {
    cube(outside_dims);
    translate([outside_border, outside_border, base_thickness+0.001]) {
      cube(board_dims);
      $fn = 20;
      translate([screw_offset, screw_offset, -2]) cylinder(5, dscrew/2, dscrew/2);
      translate([board_dims[0] - screw_offset, board_dims[1] - screw_offset, -2])
	cylinder(5, dscrew/2, dscrew/2);
    }
  }
}
