// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <gui.scad>
show_insert = true;
show_vitamins = false;
include <reservoir_insert.scad>

translate([0, insert_dims[1], insert_dims[2]]) rotate(180, X) Insert();
