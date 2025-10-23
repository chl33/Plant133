// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

// This is a fixture for holding a PCB and stencil in alignment
board_thickness = 1.6;
pad_space = 2.54;
board_dims = [23.4*pad_space-0.1, 16.6*pad_space, board_thickness];
outside_border = 2;
base_thickness = 1;

outside_dims = board_dims + [2*outside_border, 2*outside_border, base_thickness];

if (false) {
  translate([board_dims[0]/2, board_dims[1]/2, board_dims[2]/2 + base_thickness]) color("white")
    import(file="Plant1337.stl", convexity=3);
 }

dscrew = 2.5;
screw_offset = dscrew + 0.2;

translate([-outside_border, -outside_border, 0]) {
  difference() {
    union() {
      cube(outside_dims);
    }
    translate([outside_border-0.1, outside_border-0.1, base_thickness+0.001]) {
      cube(board_dims + [0.2, 0.2, 0]);
    }
    inside_offset = outside_border + screw_offset + dscrew/2 + outside_border;
    translate([inside_offset, inside_offset, -0.01]) {
      cube([outside_dims[0] - 2 * inside_offset, outside_dims[1] - 2 * inside_offset,
	    base_thickness+1]);
    }
  }
  translate([outside_border, outside_border, base_thickness-0.01]) {
    $fn = 20;
    translate([screw_offset, screw_offset, 0])
      cylinder(board_thickness+1, dscrew/2, dscrew/2);
    translate([board_dims[0] - screw_offset, board_dims[1] - screw_offset, 0])
      cylinder(board_thickness+1, dscrew/2, dscrew/2);
  }
}
