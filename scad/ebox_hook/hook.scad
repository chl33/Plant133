// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <../box/board.scad>
include <../box/box.scad>

epsilon = 0.01;

screw_head_height = 2;
wall_thickness = 1;
reservoir_thickness = 3;
gap = 0.2;

module plantl33_hook() {
  module recessed_hole() {
    w = 2;
    d = screw_head_height + wall_thickness;
    translate([0, 0, -epsilon]) {
      cylinder(d+1, 1.5, 1.5);
      cylinder(screw_head_height, 4, 4);
    }
  }

  union() {
    // Ebox mount plate.
    difference() {
      $fn = 40;
      cube([inner_dims[0]+2, inner_dims[1], wall_thickness+screw_head_height]);
      // Screw holes fox ebox;
      translate([-0, -5, 0]) {
	translate([15, 15, 0]) recessed_hole();
	translate([40, 40, 0]) recessed_hole();
      }
    }
    sep = wall_thickness+reservoir_thickness+gap*2;
    // Inside reservoir.
    translate([0, 0, -sep]) cube([5, inner_dims[1], wall_thickness]);
    // Top of the hook, spaning the reservoir wall.
    translate([-2*wall_thickness+epsilon, 0, -sep])
      cube([wall_thickness*2, inner_dims[1], wall_thickness+sep+screw_head_height]);
  }
}
