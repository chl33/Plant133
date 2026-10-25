// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <vertical_pump.scad>
include <float.scad>

include <ProjectBox/rounded_box.scad>

// TODO
// - [ ] Pump holder
//   - [ ] Pump snaps into place, maybe held by tension?
// - [ ] Snaps
// - [ ] Only legs on the left side?

// Make this the distance from the bottom of the float to the top of the buttom-nut.
insert_leg_height = 60;
insert_leg_radius = 4;

insert_top_thickness = 2;

insert_dy = 50; // TBD
insert_end_dx = 30; // TBD
insert_float_dx = float_diameter + 4 * insert_leg_radius; // TBD
insert_pump_dx = 40; // TBD


module insert_leg() {
    cylinder(insert_leg_height, insert_leg_radius, insert_leg_radius);
}

module insert_legs(dx, on_right=false) {
  translate([dx-insert_leg_radius, insert_leg_radius, 0]) insert_leg();
  translate([dx-insert_leg_radius, insert_dy-insert_leg_radius, 0]) insert_leg();
  translate([insert_leg_radius, insert_leg_radius, 0]) insert_leg();
  translate([insert_leg_radius, insert_dy-insert_leg_radius, 0]) insert_leg();
}

module insert_top(dx) {
  top_dims = [dx, insert_dy, insert_top_thickness];
  translate([0, 0, insert_leg_height]) {
    rounded_box(top_dims, insert_leg_radius);
  }
}

module insert_end() {
  insert_top(insert_end_dx);
  insert_legs(insert_end_dx, on_right=true);
}
module insert_float_holder() {
  difference() {
    float_y_offset = float_diameter / 2 + 2 * insert_leg_radius;
    pump_y_offset = 15;  // TBD
    insert_top(insert_float_dx);
    translate([insert_float_dx / 2, float_y_offset, insert_leg_height - epsilon]) {
      cylinder(1 + insert_top_thickness, float_top_diam/2, float_top_diam/2);
    }
    translate([(insert_float_dx - float_top_diam) / 2, -epsilon, insert_leg_height - epsilon]) {
      cube([float_top_diam, float_y_offset, 1 + insert_top_thickness]);
    }
  }
  insert_legs(insert_float_dx);
}
module insert_pump_holder() {
  insert_top(insert_pump_dx);
  insert_legs(insert_pump_dx);
}

translate([insert_float_dx + 1, 0, 0]) insert_end();
insert_float_holder();

