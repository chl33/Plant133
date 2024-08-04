// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <ProjectBox/project_box.scad>
include <ProjectBox/mounts.scad>
include <ProjectBox/shtc3_window.scad>
include <board.scad>

ones = [1, 1, 1];

wall_thickness = 1;
gap = 0.2;
corner_radius = 2;
mount_offset = pad_space;

space_above_board = 2;
space_below_board = 5;
inner_dims = (board_dims
	      + Z*(space_above_board+space_below_board)
	      + 2*gap*ones);
outer_dims = (inner_dims
	      + 2*ones*wall_thickness
	      + [2, 2, 0] * corner_radius);

// OLED bump offset and dims
oled_o = [20, 0];
oled_d = [outer_dims[0] - oled_o[0], 17, 12];

// bumps is a list of [offset-xy, outer_dims]
humps = [[oled_o, oled_d]];

// cutout for moisture & pump
conns_o = [31.5, 17.1];
conns_d = [30.5, 21];
// cutout for water-level sensor
wlconn_o = [conns_o[0], conns_o[1]+conns_d[1]-epsilon];
wlconn_d = [8.5, 7];
// cutout for oled screen
olsc_o = [oled_o[0] + 7.5, oled_o[1] + 3.5];
olsc_d = [28, 12];
// top_cutouts is a list of [offset-xy,  dims-xu]
top_cutouts = [[conns_o, conns_d], [wlconn_o, wlconn_d], [olsc_o, olsc_d]];

usb_cutout = [[25, wall_thickness+space_below_board+board_thickness-1], [9.5, 3.5]];
ym_cutouts = [usb_cutout];

module in_Plant1337_board_frame(board_height=false) {
  zoffset = wall_thickness + (board_height ? space_below_board + 2*gap + board_thickness : 0);
  in_board_frame(outer_dims, board_dims, zoffset) children();
}

module Plant1337_box(top) {
  wall = wall_thickness;
  shtc3_loc = [board_dims[0] - 10.5, board_dims[1]-5, 0];

  difference() {
    union() {
      project_box(outer_dims,
		  wall_thickness=wall_thickness,
		  gap=gap,
		  snaps_on_sides=true,
		  corner_radius=corner_radius,
		  top=top,
		  humps=humps,
		  top_cutouts=top_cutouts,
		  ym_cutouts=ym_cutouts);
      if (top) {
	in_Plant1337_board_frame(board_height=true)
	  shtc3_window(shtc3_loc, space_above_board+wall, wall, false, z_gap=-1);
      } else {
	// Stuff to add on bottom.
	in_Plant1337_board_frame() {
	  at_corners(board_dims+1.0*X, mount_offset, x_extra=-1.2, y_extra=-0.2)
	    screw_mount(space_below_board, wall, 2.5/2);
	}
      }	
    }
    // Stuff to remove.
    if (top) {
      in_Plant1337_board_frame(board_height=true)
	shtc3_window(shtc3_loc, space_above_board, wall, true, z_gap=-1);
    } else {
      // Screw holes to mount on reservoir lid.
      w = 2;
      len = 25;
      $fn = 40;
      translate([outer_dims[0]/2, outer_dims[1]/2, -0.1]) {
	translate([-len/2, len/2, -1]) cylinder(2*w+2, 1.5, 1.5);
	translate([len/2, -len/2, -1]) cylinder(2*w+2, 1.5, 1.5);
      }
    }
  }
}
