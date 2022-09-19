//include <lib/roundedcube.scad>

// Helperinos
module rounded_square(d,r) {
    minkowski() {
        translate([r,r]) square([d[0]-2*r, d[1]-2*r]);
        circle(r, $fn=20);
    }
}
module rounded_cube(d, r) {
    linear_extrude(d[2]) { rounded_square([d[0],d[1]], r); }
}

// ---- BASE ------
base_x = 130;
base_y = 120;
// - Wall thickness*2
base_margin = 10;
base_thick = 1;
base_height = 33;
// ----- ESP CUTOUT ------
// Height offset over board
esp_offset = 8;
esp_width = 32;
esp_height = 10;
// ------ PCB SCREW POSTS ----
post_height = 5;
post_screw_dia = 2.8;
post_dia = 7;
posts = [ [7.6,7.6], [101.5, 7.6], [81.3, 68.5], [17.7, 68.5] ];
// ------ PCB -------
pcb_pos_x = 1;
pcb_pos_y = 10;
// ------ TOP SCREW POSTS -----
top_post_height = 8;
top_post_screw_dia = 2.8;
top_post_sq = 7;

// Create the baseplate and perimeter
difference() {
    // Base cube
    rounded_cube([base_x, base_y, base_height], 5);
    // Cut the inner part out
    translate([base_x/2, base_y/2, base_height/2 + base_thick])
    cube([base_x-base_margin, base_y-base_margin, base_height], true);
    // Create a cutout for the ESP
    // - 1.5 is approx. board thickness
    color("green") translate([base_margin/2, base_y + .5, post_height + 1.5 + esp_offset]) rotate([90,0,0]) rounded_cube([esp_width, esp_height, base_margin/2 + 1], 3);
}

// Create the top screw posts
top_posts = [
    [base_margin/2,base_margin/2],
    [base_x - base_margin/2 - top_post_sq, base_margin/2],
    [base_x - base_margin/2 - top_post_sq, base_y - base_margin/2 - top_post_sq],
    [base_margin/2, base_y - base_margin/2 - top_post_sq]
];
for(post = top_posts) {
    difference() {
        // Post
        color("orange") translate([post[0], post[1], base_height - top_post_height])
        cube([top_post_sq,top_post_sq,top_post_height]);
        // Screw hole
        translate([post[0] + top_post_sq/2, post[1] + top_post_sq/2, base_height - top_post_height - 0.5]) cylinder(h=top_post_height + 1, d=top_post_screw_dia, $fn=30);
    }

}

// Create the 4 screw posts
for(post = posts) {
    difference() {
        // Base post
        color("red") 
        translate([base_margin/2 + post[0] + pcb_pos_x, base_margin/2 + post[1] + pcb_pos_y, base_thick])
        cylinder(post_height, d=post_dia, $fn=30);
        // Cut the screwhole out
        translate([base_margin/2 + post[0] + pcb_pos_x, base_margin/2 + post[1] + pcb_pos_y, base_thick])
        cylinder(post_height+1, d=post_screw_dia, $fn=30);
    }

}

// Import the board as a template
// - offset messed up origin
//translate([-58.42, 157.5, 0])
//translate([base_margin/2 + pcb_pos_x, base_margin/2 + pcb_pos_y, post_height])
// - place in base
// - exported from KiCad
//import("uam-thing-board.stl");