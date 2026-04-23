
res_width = 100;
res_length = 300;
//res_length = 350;
wall_thickness = 1;
cap_height = 10;
gap = 0.5;
epsilon = 0.01;
pin_rad = 1.5;
pin_height = 3;
res_wall_thickness = 2.5;

outside_rad = res_width / 2 + wall_thickness + gap;
outside_len = res_length + 2 * (wall_thickness + gap);
outside_height = cap_height + wall_thickness;

module cap() {
  $fn = 180;
  difference() {
    union() {
      translate([outside_rad, outside_rad, 0]) {
	cylinder(outside_height, outside_rad, outside_rad);
      }
      translate([outside_rad, 0, 0]) {
	cube([outside_len - outside_rad*2, outside_rad * 2, outside_height]);
      }
      translate([outside_len-outside_rad, outside_rad, 0]) {
	cylinder(outside_height, outside_rad, outside_rad);
      }
    }
    union() {
      // Carve-out the inside of the cover.
      inside_rad = res_width / 2;
      inside_len = res_length - 2 * gap;
      offset = [gap + wall_thickness, gap + wall_thickness, -epsilon];
      translate(offset + [inside_rad, inside_rad, 0]) {
	cylinder(cap_height, inside_rad, inside_rad);
      }
      translate(offset+ [inside_rad, 0, 0]) {
	cube([inside_len - inside_rad*2, inside_rad * 2, cap_height]);
      }
      translate(offset + [inside_len-inside_rad, inside_rad, 0]) {
	cylinder(cap_height, inside_rad, inside_rad);
      }
      // Slot on side for ebox hook and wires.
      slot_y = 10;
      slot_x = 60;
      translate([outside_len/2-epsilon, -epsilon, -epsilon]) {
	cube([slot_x, slot_y, outside_height+1]);
      }
      // Slot for tubing
      tubing_offset_x = outside_len/2-epsilon;
      tubing_slot_y = 35;
      tubing_width = 10;
      translate([tubing_offset_x, offset[1], cap_height-1]) {
	cube([tubing_width, tubing_slot_y, 10]);
      }
    }
  }
  // Add 'pins' to help hold cover in place.
  pin_x_offset = wall_thickness + 2*gap + pin_rad + res_wall_thickness;
  translate([pin_x_offset, outside_rad, 0]) {
    translate([0, 0, cap_height-pin_height]) {
      cylinder(pin_height, pin_rad, pin_rad);
      sphere(pin_rad);
    }
  }
  translate([outside_len - pin_x_offset, outside_rad, 0]) {
    translate([0, 0, cap_height-pin_height]) {
      cylinder(pin_height, pin_rad, pin_rad);
      sphere(pin_rad);
    }
  }
}

module cap_left() {
  difference() {
    cap();
    translate([outside_len/2, -1, -50]) {
      cube([outside_len/2 + 1, 2 * (outside_rad + 1), 100]);
    }
  }
}

module cap_right() {
  difference() {
    cap();
    translate([-1, -1, -50]) {
      cube([outside_len/2 + 1, 2 * (outside_rad + 1), 100]);
    }
  }
}

module cap_side(left) {
  if (left) {
    cap_left();
  } else {
    cap_right();
  }
}

//cap();
cap_side(false);

