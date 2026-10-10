// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

float_height = 67;
float_diameter = 28.5;
float_top_diam = 11;

module Float() {
  $fn = 80;
  stem_diameter = 8;
  screw_height = 23;
  screw_diameter = 10;
  nut_height = 6;
  nut_diameter = 15;
  color("gray") {
    cylinder(float_height, stem_diameter/2, stem_diameter/2);
    // float-end-stop
    translate([0, 0, 1]) cylinder(1, 15/2, 15/2);
    // float
    translate([0, 0, 2]) cylinder(29, float_diameter/2, float_diameter/2);
    // Screw
    translate([0, 0, float_height-screw_height])
      cylinder(screw_height, screw_diameter/2, screw_diameter/2);
    // Nut
    translate([0, 0, float_height-screw_height- 2]) {
      cylinder(nut_height, nut_diameter/2, nut_diameter/2, $fn=6);
      translate([0, 0, nut_height-1]) cylinder(1, nut_diameter/2, nut_diameter/2);
      // Gasket.
      translate([0, 0, nut_height]) cylinder(2, nut_diameter/2-0.5, nut_diameter/2-0.5);
    }
  }
}
