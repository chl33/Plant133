// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <reservoir_insert.scad>

translate([0, top_dims[1], top_dims[2]]) rotate(180, X) InsertTop();
