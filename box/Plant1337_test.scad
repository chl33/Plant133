include <board.scad>
include <box.scad>
include <gui.scad>

union() {
  if (show_box) {
    difference() {
      Plant1337_box(top);
      // translate([0, 44, -1]) cube([100, 100, 100]);
    }
  }
  if (show_vitamins) {
    translate([wall_thickness+corner_radius,
	       wall_thickness+corner_radius,
	       wall_thickness+space_below_board-1])
      Plant1337_board();
  }
}
