// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <moisture_sensor.scad>

sensor_num = "4";
wall_thickness = 3;

grow_offset = 0.8;

grown_moisture_sensor_dims = moisture_sensor_dims + [2, 1, 1] * grow_offset;
moisture_box_dims = [grown_moisture_sensor_dims[0] + 2 * wall_thickness,
		     30,
		     2*wall_thickness+grown_moisture_sensor_dims[2]+2];

sensor_offset = [(moisture_box_dims[0] - grown_moisture_sensor_dims[0])/2+grow_offset/2,
		 wall_thickness+2,
		 wall_thickness];

module tube_holder() {
  $fn = 40;
  difference() {
    cylinder(10, 6, 6);
    translate([0, 0, 5]) rotate(45, Z) rotate(90, X) translate([0, 0, -7]) cylinder(14, 4, 4);
  }
}

module moisure_sensor_cap(top) {
  wall = wall_thickness;
  dims = moisture_box_dims;
  difference() {
    union() {
      cube(dims);
      translate([11, 20, -10+epsilon]) tube_holder();
    }
    
    translate(sensor_offset) moisture_sensor(true);
    cut_height = 2.51;
    if (top) {
      translate([-1, -1, -20]) cube([3+dims[0], 3+dims[1], cut_height+20]);
      text_size = 15;
      text_depth = 0.8;
      translate([20.5, 25, dims[2]-text_depth+epsilon])
	linear_extrude(2*text_depth) {
	rotate(180, Z) text(sensor_num, size=text_size);
      }
    } else {
      translate([-1, -1, cut_height + epsilon]) cube([3+dims[0], 3+dims[1], 30]);
    }
    // Screw holes
    translate([0, 5.5, 0]) {
      translate([2.25, 19.8, -1]) cylinder(dims[2]+2, 3/2, 3/2, $fn=20);
      translate([dims[0]-2.25, 19.8, -1]) cylinder(dims[2]+2, 3/2, 3/2, $fn=20);
    }
    // Screw holes
    translate([0, -16.5, 0]) {
      translate([2.25, 19.5, -1]) cylinder(dims[2]+2, 3/2, 3/2, $fn=20);
      translate([dims[0]-2.25, 19.5, -1]) cylinder(dims[2]+2, 3/2, 3/2, $fn=20);
    }
  }
}
