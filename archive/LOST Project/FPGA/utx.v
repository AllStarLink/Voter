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
`include "config.h"


`define IDLE 2'b00
`define SENDSTART 2'b01
`define SENDBITS 2'b11
`define SENDSTOP 2'b10

// Generate a one clock cycle wide output every n divisor clocks
// This is a 7 bit synchronous modulo n counter



module baudcounter(clk, rstn, arm, baudce);
	input clk;
	input rstn;
	input arm;
	output reg baudce;

	reg [10:0] baudcntr;
	
	always @(*)
		baudce <= (baudcntr == `BAUDRATE_DIVISOR) ? 1'b1 : 1'b0;
	
	always @(posedge clk or negedge rstn) begin
		if(rstn == 0)
			baudcntr <= 0;
		else begin
			if(arm) begin
				if(baudcntr == `BAUDRATE_DIVISOR)
					baudcntr <= 0;
				else	
					baudcntr <= baudcntr + 1'b1;
			end else
				baudcntr <= 0;
		end
	end
endmodule


// Count the bits we are transmitting
// This is a synchronous decade counter


module bit_counter(clk, rstn, ce, bitnum);
	input clk;
	input rstn;
	input ce;
	output [3:0] bitnum;

	
	reg [3:0] bitnum_int;
	
	assign bitnum = bitnum_int;
	
	always @(posedge clk or negedge rstn) begin
		if(rstn == 0)
			bitnum_int <= 0;
		else begin
			if(ce) begin
				if(bitnum_int == 9)
					bitnum_int <= 0; // Cycle back to 0
				else
					bitnum_int <= bitnum_int + 1'b1;
			end
		end
	end	
endmodule


// parallel data in LSB first shift register

module sr_lsb_first(clk, rstn, load, shift, parallelin, lsbout);
	input clk;
	input rstn;
	input load;
	input shift;
	input [7:0] parallelin;
	output lsbout;

	
	reg [7:0] shiftreg;
	
	assign lsbout = shiftreg[0];
	
	always @(posedge clk or negedge rstn) begin
		if(rstn == 0)
			shiftreg <= 0;
		else begin
			if(load == 1)
				shiftreg <= parallelin;
			else if (shift == 1) begin
				shiftreg[6:0] <= shiftreg[7:1];
				shiftreg[7] <= 0;
			end
		end
	end
endmodule
			
			
// Combinatorial part of the uart tx state machine

module comb_utx_sm(load, shiftregin, baudce, cur_utx_state, bitcounter, nextbit, bitcounterce, busy, done_int, serialout, next_utx_state);
	input load;
	input shiftregin;
	input baudce;
	input [1:0] cur_utx_state;
	input [3:0] bitcounter;
	output reg nextbit;
	output reg bitcounterce;
	output reg busy;
	output reg done_int;
	output reg serialout;
	output reg [1:0] next_utx_state;
	
	always @(*) begin
	
		// Defaults
		busy <= 1;
		done_int <= 0;
		nextbit <= 0;
		serialout <= 1;
		bitcounterce <= baudce;
		
		case(cur_utx_state)
			`IDLE:
				if(load) begin
					next_utx_state <= `SENDSTART;
					serialout <= 0; // Start bit is low
				end else begin
					busy <= 0; // Not busy
					bitcounterce <= 0; //  Don't count bits
					next_utx_state <= `IDLE; // Stay in idle state
				end
		
			`SENDSTART:			
				if(baudce) begin
					// This is entered when the start bit has been sent
					next_utx_state <= `SENDBITS;
					serialout <= shiftregin;
				end else begin
					// Still sending start bit
					serialout <= 0;
					next_utx_state <= `SENDSTART;
				end
		
			`SENDBITS:
				// This is entered when we are sending data bits
			
				begin
					if(bitcounter == 9) begin
						// All data bits sent, send stop bit
						next_utx_state <= `SENDSTOP;
						serialout <= 1; // Stop bit is high
					end else begin
						// Continue sending data bits
						serialout <= shiftregin;
						next_utx_state <= `SENDBITS;
						nextbit <= baudce;		
					end
				end
		
			`SENDSTOP:
				if(baudce) begin
				    // This is entered when the stop bit has been sent
					next_utx_state <= `IDLE;
					done_int <= 1;
					nextbit <= baudce;
				end else begin
					// Still sending stop bit
					next_utx_state <= `SENDSTOP;
				end
	
		
			default:
				begin
					bitcounterce <= 1'bx;
					serialout <= 1'bx;
					busy <= 1'bx;
					next_utx_state <= 2'bxx;
				end
		endcase
	end
	
endmodule


// Sequential part of the uart tx state machine

module seq_utx_sm(clk, rstn, next_utx_state, done_int, done, cur_utx_state);
	input clk;
	input rstn;
	input [1:0] next_utx_state;
	input done_int;
	output reg done;
	output reg [1:0] cur_utx_state;
	
	
	always @(posedge clk or negedge rstn) begin
		if(rstn == 0) begin
			done <= 0;
			cur_utx_state <= `IDLE;
		end else begin
			done <= done_int;
			cur_utx_state <= next_utx_state;
		end
	end
endmodule

	



/*
* UART transmitter
*/

module utx(clk, rstn, load, inbyte, serialout, done);
	input clk;
	input rstn;
	input load;
	input [7:0] inbyte;
	output serialout;
	output done;	
	
	wire baudce;
	wire bitcounterce;
	wire [1:0] cur_utx_state;
	wire [1:0] next_utx_state;
	wire [3:0] bitcounter;
	wire busy_int;
	wire done_int;
	wire shiftregout;
	wire nextbit;
	
	
	// Instantiate baud rate counter
	
	baudcounter bc0(
		.clk(clk),
		.rstn(rstn),
		.arm(busy_int),
		.baudce(baudce)
	);
	
	// Instantiate TX bit counter
	
	bit_counter btc0(
		.clk(clk),
		.rstn(rstn),
		.ce(bitcounterce),
		.bitnum(bitcounter)
		);


	// Instantiate TX shift register
	
	sr_lsb_first sr0(
		.clk(clk),
		.rstn(rstn),
		.load(load),
		.shift(nextbit),
		.parallelin(inbyte),
		.lsbout(shiftregout)
		);
		
	// Instantiate combinatorial part of state machine
	
	comb_utx_sm smc0(
		.load(load),
		.shiftregin(shiftregout),
		.baudce(baudce),
		.cur_utx_state(cur_utx_state),
		.bitcounter(bitcounter),
		.bitcounterce(bitcounterce),
		.busy(busy_int),
		.done_int(done_int),
		.serialout(serialout),
		.nextbit(nextbit),
		.next_utx_state(next_utx_state)
		);
		
	// Instantiate sequential part of state machine
	
	seq_utx_sm sms0(
		.rstn(rstn),
		.clk(clk),
		.done_int(done_int),
		.next_utx_state(next_utx_state),
		.done(done),
		.cur_utx_state(cur_utx_state)
		);
		
endmodule	
	
	

