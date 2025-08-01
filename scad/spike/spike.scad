// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <MCAD/units.scad>

spike_length = 50;
spike_width = 20;
spike_tube_diam = 6;
spike_tube_len = 5;
spike_wall_thickness = 1.5;

hole_diameter = 2;

tube_inner_diameter = 6;
tube_connection_length = 10;

module spike() {
  r = spike_width / 2;
  h = r * sqrt(3) / 2;
  l = spike_length;
  w = spike_wall_thickness;

  hr = hole_diameter / 2;
  tir = tube_inner_diameter / 2;
  tcl = tube_connection_length;

  module segment() {
    // Top
    difference() {
      translate([-r/2, 0, h-w]) cube([r, l, w]);
      // Watering holes in top face.
      for (dy = [4:8:l]) translate([0, dy, h-w-1]) cylinder(2+w, hr, hr, $fn=20);
    }
    // Front face (connector side)
    translate([-r/2, 0, 0]) cube([r, w, h]);

    // The triangular sections that together form the point.
    points = [[-r/2, l, h], [r/2, l, h], [r/2, l, h-w], [-r/2, l, h-w],
	      [0, l+h, 0], [0, l+h-w, 0]];
    // For now, not using inner front point 5
    faces = [[0, 1, 2, 3], // front face
	     [0, 4, 1],    // top face
	     [3, 5, 4, 0],    // left face
	     [2, 1, 4, 5],    // right face
	     [3, 2, 5]];   // bottom face

    difference() {
      polyhedron(points, faces);
      // One watering hole in the point section.
      translate([0, l+h/3, 0]) cylinder(h, hr, hr, $fn=20);
    }
  }

  difference() {
    union() {
      // The prismatic shell.
      for (angle = [0:60:359]) rotate(angle, Y) segment();
      // The outside of the tube connector
      translate([0, -tcl+epsilon, 0]) rotate(-90, X) cylinder(tcl+w, tir, tir, $fn=40);
    }
    // Hole for water through tube connector.
    translate([0, -tcl-1, 0]) rotate(-90, X) cylinder(tcl+w+2, tir-1, tir-1, $fn=40);
    // Hole in tip
    translate([0, l+h-w-1, 0]) rotate(-90, X) cylinder(w+2, hr, hr, $fn=20);
  }
}

// A rippled surface to cut spike so that 2 parts can be put together in a way
//  that makes gluing/epoxying back together easy.
module cut_surface() {
  fz = 50;
  fy = 100;
  fx = 100;

  a = 4;
  b = a * sqrt(3)/2;
  c = a/2;

  points = [[0, 0, 0], [fx/2, 0, 0], [fx/2, 2*b+a, 0], [0, 2*b+a, 0], // bottom
	    [0, b, c], [fx/2, b, c], [fx/2, b+a, c], [0, b+a, c]];      // top
  faces = [[0, 1, 2, 3],  // bottom
	   [7, 6, 5, 4],  // top
	   [3, 7, 4, 0],  // left
	   [5, 6, 2, 1],  // right
	   [0, 4, 5, 1],  // front
	   [2, 6, 7, 3],  // back
	   ];

  dz = 3;
  h = dz * sqrt(2) / 2;
  y_step = 2*(a+b);
  // Most of the cut shape is simple.
  translate([-fx/2, 0, -fz-c/2+epsilon]) cube([fx, fy, fz]);
  difference() {
    union() {
      // Ripples at the top of the left side of the cut surface.
      translate([-fx/2, 0, -c/2])
	for (yi = [0:y_step:fy]) translate([0, yi, 0]) polyhedron(points, faces);
      // Ripples at the top of the right side of the cut surface.
      translate([0, -a-b, -c/2])
	for (yi = [0:y_step:fy]) translate([0, yi, 0]) polyhedron(points, faces);
    }
    // Clamp the rippled surface.
    translate([-fx/2-epsilon, -0.05-a-b+epsilon, -fz]) cube([fx+2*epsilon, a+b+2*epsilon, fz+2*c]);
  }
  // A simpler cut shape for the tube connection.
  translate([-fx/2, -tube_connection_length-1, -fz])
    cube([fx, tube_connection_length+1+epsilon, fz]);
}


difference() {
  spike();
  cut_surface();
}
