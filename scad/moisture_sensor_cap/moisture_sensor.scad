// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <MCAD/units.scad>
include <ProjectBox/headers.scad>

moisture_sensor_dims = [22.5, 92, 1.5];

module moisture_sensor(space=false) {
  dims = moisture_sensor_dims + Z * (space ? 0.5 : 0);

  offset_val = space ? 0.5 : 0;

  points = [[0, 0],      // fl
	    [0, 18],     // l cut-out fl
	    [1.75, 19],   // l cut-out fr
	    [1.75, 22],   // l cut-out hr
	    [0, 23],     // l cut-out hl
	    [0, 28],     // l mid
	    [0, 85],     // hl
	    [11, 92],    // point
	    [22.5, 85],    // hr
	    [22.5, 28],    // r mid
	    [22.5, 23],    // r cut-out hr
	    [22.5-1.75, 22],  // r cut-out hl
	    [22.5-1.75, 19],  // r cut-out fl
	    [22.5, 18],    // r cut-out rr
	    [22.5, 0]];    // fr

  translate([1, 1, -1]*offset_val) {
    color("green") {
      linear_extrude(dims[2]+2*offset_val) {
	offset(offset_val)
	  polygon(points = points, paths = [[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14]]);
      }
      linear_extrude(2*dims[2]+2*offset_val) {
	offset(offset_val)
	  polygon(points = points, paths = [[0, 1, 2, 3, 4, 5, 9, 10, 11, 12, 13, 14]]);
      }

      component_z = 1.5;
      translate([1, 3, dims[2]-epsilon]) cube([20, 15, component_z]);
      translate([2, 18-epsilon, dims[2]-epsilon]) cube([18, 5, component_z]);
    }
    conn_dims = connector_nx1_dims(3);
    if (space) {
      // Reserve simple space for connector when using difference to make space for sensor.
      gap = 0.3;
      space_dims = gap*[2,1,2] + conn_dims;
      color("white") {
	translate([(dims[0]-space_dims[0])/2, -6, dims[2]-0.6-1.4])
	  cube([space_dims[0], space_dims[2]+4.5, space_dims[1]+4]);
      }
    } else {
      translate(-header_offset + [(dims[0])/2, dims[2]-1, dims[2]+conn_dims[1]/2]) {
	rotate(180, Y) rotate(90, X) connector_nx1(3, pin_extend=false);
      }
    }
    // Location of pins on the back
    color("white") translate([6, 4, -2+epsilon]) cube([10, 2, 2]);
  }
}
