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
* Arbitrary width synchronous up/down counter
*/

module counter (rstn, clk, up, down, count);
    // Default parameter. This can be overrriden
	parameter WIDTH = 8;
	
	input rstn;
	input clk;
	input up;
	input down;
	output [WIDTH-1:0] count;
	
	reg[WIDTH-1:0] count_int = 0;
	
	assign count = count_int;
	
	always @ (posedge clk or negedge rstn) begin
		if(rstn == 0) begin
			count_int <= 0;
		end else begin		
			if(up == 1) begin
				count_int  <= count_int + 1'b1;
			end else if(down == 1) begin
				count_int <= count_int - 1'b1;
			end
		end
	end
endmodule


