// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <MCAD/units.scad>
//include <ProjectBox/rounded_box.scad>

pump_diameter = 25;
pump_diameter2 = 12.5;
pump_height = 47;

pump_cyl_sep =  [-20,  pump_diameter/2 - pump_diameter2/2, 0];
pump_cyl_sep1 = [-20, -pump_diameter/2 + pump_diameter2/2, 0];
pump_wire_bump_diameter = 5.5;
pump_wire_angle = 320;
pump_wire_offset_d = pump_diameter/4;
pump_wire_offset = [pump_wire_offset_d*cos(pump_wire_angle),
		    pump_wire_offset_d*sin(pump_wire_angle),
		    pump_height-epsilon];

module VerticalPump() {
  $fn = 80;

  cyl2_height = 26;
  cyl2_diameter = 8;

  color("white") {
    cylinder(pump_height, pump_diameter/2, pump_diameter/2);
    translate(pump_cyl_sep) {
      cylinder(12, pump_diameter2/2, pump_diameter2/2);
      cylinder(cyl2_height, cyl2_diameter/2, cyl2_diameter/2);
    }
    translate(pump_cyl_sep - Y*pump_diameter2/2) {
      cube([-pump_cyl_sep[0], pump_diameter2, 12]);
    }
    translate(pump_wire_offset) {
      cylinder(1, pump_wire_bump_diameter/2, pump_wire_bump_diameter/2);
    }
  }
  translate(pump_wire_offset) {
    color("black") cylinder(30, 1, 1, $fn=10);
  }
}
