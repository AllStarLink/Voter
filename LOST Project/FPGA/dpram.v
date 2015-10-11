/*
 * L O S T -- Logger Of Signal Transitions
 * Version 1.0, 10/11/2015
 *
 * Copyright 2015, Stephen A. Rodgers. All rights reserved.
 * Copyright 2015, Jim Dixon <jim@lambdatel.com>
 *
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

`default_nettype none



/*
* Arbitrary width and addressing depth dual port ram 
*/

module dualportram #(
    // Default parameters. These can be overrriden
    parameter WIDTH = 64,
    parameter DEPTH = 6
) (
    // Port A
    input   wire                clk_a,
    input   wire                we_a,
    input   wire    [DEPTH-1:0]  addr_a,
    input   wire    [WIDTH-1:0]  din_a,
    output  reg     [WIDTH-1:0]  dout_a,
     
    // Port B
    input   wire                clk_b,
    input   wire                we_b,
    input   wire    [DEPTH-1:0]  addr_b,
    input   wire    [WIDTH-1:0]  din_b,
    output  reg     [WIDTH-1:0]  dout_b
	);
 
// Shared memory array
reg [WIDTH-1:0] memarray [(2**DEPTH)-1:0];
 
// Port A
always @(posedge clk_a) begin
	// Always load the output register
    dout_a      <= memarray[addr_a];
    if(we_a) begin // Write enable?
        dout_a      <= din_a;
        memarray[addr_a] <= din_a;
    end
end
 
// Port B
always @(posedge clk_b) begin
	// Always load the output register
    dout_b      <= memarray[addr_b];
    if(we_b) begin // Write enable?
        dout_b      <= din_b;
        memarray[addr_b] <= din_b;
    end
end
 
endmodule
