// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

//include <board.scad>
//include <box.scad>

include <box.scad>
include <moisture_sensor.scad>

show_vitamins = false;
//show_vitamins = true;
show_box = true;
//show_box = false;
// top = true;
top = false;
space = false;

union() {
  if (show_box) moisure_sensor_cap(top);
  if (show_vitamins) translate(sensor_offset+[1,0.5,1]*grow_offset) moisture_sensor(space=space);
}
