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

module mux64to8 (inword, sel, outbyte);
	input [63:0] inword;
	input [2:0] sel;
	output reg [7:0] outbyte;
	
	always @(*) begin
		case(sel)
			3'h0:
				outbyte <= inword[7:0];
				
			3'h1:
				outbyte <= inword[15:8];
		
			3'h2:
				outbyte <= inword[23:16];
		
			3'h3:
				outbyte <= inword[31:24];
		
			3'h4:
				outbyte <= inword[39:32];
		
			3'h5:
				outbyte <= inword[47:40];
		
			3'h6:
				outbyte <= inword[55:48];
		
			3'h7:
				outbyte <= inword[63:56];
		
			default:
				outbyte <= 8'bxxxxxxxx;
		endcase
	end
endmodule

			
		
