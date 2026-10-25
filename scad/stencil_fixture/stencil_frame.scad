// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

epsilon = 0.001;

board_thickness = 1.6;
stencil_dims = [160, 120, 1];
inner_dims = [120, 80];
outside = 3;
//inside_lip = 10;

module stencil_frame() {
  frame_dims = stencil_dims + [2*outside, 2*outside, board_thickness];
  
  difference() {
    // Frame body
    cube(frame_dims);
    // Cutout for board
    translate([(stencil_dims[0]-inner_dims[0])/2, (stencil_dims[1]-inner_dims[1])/2, -epsilon])
      cube([inner_dims[0], inner_dims[1], frame_dims[2] + 1]);

    // Cutout for stencil
    translate([outside, outside,board_thickness]) {
      cube([frame_dims[0]-2*outside, frame_dims[1]-2*outside, frame_dims[2]+1]);
    }
  }
}

stencil_frame();
