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
 
 
 /*
 * SERIAL DATA FORMAT
 *
 * When an event occurs, a 21 byte ASCII serial data packet will be emitted.
 * The format of this packet is as follows:
 *
 *
 * :ssdddddddddddddddd\r\n
 *
 * The output is ASCII.
 *
 * ss 				- Status bits in ASCII hex
 * dddddddddddddddd - Data bits in ASCII hex (count + data bit) 
 * most significant bits first
 *
 * 
 * Status:
 * -------
 *
 * BIT
 * 7	Error (OVERRUN) Flag
 * 6-4	Reserved
 * 3-0  Channel ID
 *
 * 1. The overrun error flag will be sent if the FIFO was overrun on a
 *	  specific channel. The count and data bytes will be undefined.
 *
 * 2. The reserved bits are set to 0
 *
 * 3. The channel ID indicates the channel which posted the event.
 *
 *
 * Count and data bytes
 *
 * 
 *
 * Bits 63:1	Number of 10MHz clocks since reset
 * Bit 0		New state of the data bit
 *
 * Upon reset, an initial event will be posted on all channels.
 */
 

`default_nettype none

`define ATTN_CHECK 3'b000
`define SEND_HEADER 3'b001
`define SEND_STATUS_HIGH 3'b010
`define SEND_STATUS_LOW 3'b011
`define SEND_WORD_HIGH 3'b100
`define SEND_WORD_LOW 3'b101
`define SEND_TRAILER 3'b110
`define SEND_TRAILER2 3'b111







// System state machine


// Combinatorial part

module sys_sm_comb(sys_sm_cur, channel_cur, channel_scan, byteaddr, channel_data, hexchar, attention_ch0, 
attention_ch1, attention_ch2, attention_ch3, overrun_cur, overrun_ch0, overrun_ch1, overrun_ch2, overrun_ch3,
utxdone, clearoverrun_ch0, clearoverrun_ch1, clearoverrun_ch2, clearoverrun_ch3, txstart, nextbyte,
overrun_next, unload_ch0, unload_ch1, unload_ch2, unload_ch3, next_scan_channel, hexnybble,sys_sm_next, channel_next, txdata);

	input [2:0] sys_sm_cur;
	input [1:0] channel_cur;
	input [1:0] channel_scan;
	input [2:0] byteaddr;
	input [7:0] channel_data;
	input [7:0] hexchar;
	input attention_ch0;
	input attention_ch1;
	input attention_ch2;
	input attention_ch3;
	input overrun_cur;
	input overrun_ch0;
	input overrun_ch1;
	input overrun_ch2;
	input overrun_ch3;
	input utxdone;
	output reg clearoverrun_ch0;
	output reg clearoverrun_ch1;
	output reg clearoverrun_ch2;
	output reg clearoverrun_ch3;
	output reg txstart;
	output reg nextbyte;
	output reg overrun_next;
	output reg unload_ch0;
	output reg unload_ch1;
	output reg unload_ch2;
	output reg unload_ch3;
	output reg next_scan_channel;
	output reg [3:0] hexnybble;
	output reg [2:0] sys_sm_next;
	output reg [1:0] channel_next;
	output reg [7:0] txdata;


	always @ (*) begin
		// Defaults
		
		clearoverrun_ch0 <= 0;
		clearoverrun_ch1 <= 0;
		clearoverrun_ch2 <= 0;
		clearoverrun_ch3 <= 0;
		unload_ch0 <= 0;
		unload_ch1 <= 0;
		unload_ch2 <= 0;
		unload_ch3 <= 0;
		channel_next <= channel_cur;
		overrun_next <= overrun_cur;
		txstart <= 0;
		nextbyte <= 0;
		next_scan_channel <= 0;
		txdata <= 8'h3A;
		hexnybble <= 0;
		
		
		case(sys_sm_cur)
			`ATTN_CHECK:
				// Check channel attention bits
				if((channel_scan == 0) && (attention_ch0)) begin
					// Save overrun state for later
					overrun_next <= overrun_ch0;
					// Clear channel overrun
					clearoverrun_ch0 <= 1;
					// Next state
					sys_sm_next <= `SEND_HEADER;
					// Set the channel we are going to work with
					channel_next <= 0; // Channel 0
					// Send the header byte
					txstart <= 1;
					// Next scan channel
					next_scan_channel <= 1;
					
				end else if((channel_scan == 1) && (attention_ch1)) begin	
					// Save overrun state for later
					overrun_next <= overrun_ch1;
					// Clear channel overrun
					clearoverrun_ch1 <= 1;
					// Next state
					sys_sm_next <= `SEND_HEADER;
					// Set the channel we are going to work with
					channel_next <= 1; // Channel 1
					// Send the header byte
					txstart <= 1;
					// Next scan channel
					next_scan_channel <= 1;
				end else if((channel_scan == 2) && (attention_ch2)) begin	
					// Save overrun state for later
					overrun_next <= overrun_ch2;
					// Clear channel overrun
					clearoverrun_ch2 <= 1;
					// Next state
					sys_sm_next <= `SEND_HEADER;
					// Set the channel we are going to work with
					channel_next <= 2; // Channel 2
					// Send the header byte
					txstart <= 1;
					// Next scan channel
					next_scan_channel <= 1;
				end else if((channel_scan == 3) &&(attention_ch3)) begin	
					// Save overrun state for later
					overrun_next <= overrun_ch3;
					// Clear channel overrun
					clearoverrun_ch3 <= 1;
					// Next state
					sys_sm_next <= `SEND_HEADER;
					// Set the channel we are going to work with
					channel_next <= 3; // Channel 3
					// Send the header byte
					txstart <= 1;	
					// Next scan channel
					next_scan_channel <= 1;						
				end else begin
					// Nothing to send
					// Next scan channel
					next_scan_channel <= 1;
					sys_sm_next <= `ATTN_CHECK;
				end
			
			 
			`SEND_HEADER:
				if(~utxdone)
					// Still busy
					sys_sm_next <= `SEND_HEADER;
				else begin
					// Send the status byte
					//txdata <= {overrun_cur, 3'b000, channel_cur};
					// Convert high bits of status
					hexnybble <= {overrun_cur, 3'b000};
					txdata <= hexchar;
					txstart <= 1;
					sys_sm_next <= `SEND_STATUS_HIGH;	
				end
			
		
				
			`SEND_STATUS_HIGH:
				if(~utxdone)
					// Still busy
					sys_sm_next <= `SEND_STATUS_HIGH;
				else begin
					// Send the low nybble of the status byte
					hexnybble <= channel_cur;
					txdata <= hexchar;
					txstart <= 1; 
					sys_sm_next <= `SEND_STATUS_LOW;
				end
				
				
			`SEND_STATUS_LOW:
				if(~utxdone)
					// Still busy
					sys_sm_next <= `SEND_STATUS_LOW;
				else begin
					// Send the highest nybble in the long long word
					hexnybble <= channel_data[7:4];
					txdata <= hexchar;
					txstart <= 1; 
					sys_sm_next <= `SEND_WORD_HIGH;
				end
			
			`SEND_WORD_HIGH:
				if(~utxdone)
					// Still busy
					sys_sm_next <= `SEND_WORD_HIGH;
				else begin	
					// Low nybble
					hexnybble <= channel_data[3:0];
					txdata <= hexchar;
					nextbyte <= 1;
					txstart <= 1;
					sys_sm_next <= `SEND_WORD_LOW;
				end	
			
				
				
			`SEND_WORD_LOW:
				if(~utxdone)
					// Still busy
					sys_sm_next <= `SEND_WORD_LOW;
				else begin	
					if(byteaddr == 0) begin
						txdata <= 8'h0D; // CR
						txstart <= 1;
						if(channel_cur == 4'b0000)
							unload_ch0 <= 1;
						else if (channel_cur == 4'b0001)
							unload_ch1 <= 1;
						else if (channel_cur == 4'b0010)
							unload_ch2 <= 1;
						else if (channel_cur == 4'b0011)
							unload_ch3 <= 1;
						sys_sm_next <= `SEND_TRAILER;
					end else begin
						// Next high nybble
						hexnybble <= channel_data[7:4];
						txdata <= hexchar;
						txstart <= 1;
						sys_sm_next <= `SEND_WORD_HIGH;
					end	
				end
			
			`SEND_TRAILER:
				if(~utxdone)
					// Still busy
					sys_sm_next  <= `SEND_TRAILER;
				else begin
					txdata <= 8'h0A;
					txstart <= 1;
					sys_sm_next <= `SEND_TRAILER2;
				end
			
			`SEND_TRAILER2:
				if(~utxdone)
					// Still busy
					sys_sm_next  <= `SEND_TRAILER2;
				else
					sys_sm_next <= `ATTN_CHECK;
			
			default:
				begin
					sys_sm_next <= 3'bxxx;
					channel_next <= 2'bxx;
					txdata <= 8'bxxxxxxxx;
					clearoverrun_ch0 <= 1'bx;
					clearoverrun_ch1 <= 1'bx;
					clearoverrun_ch2 <= 1'bx;
					clearoverrun_ch3 <= 1'bx;
					unload_ch0 <= 1'bx;
					unload_ch1 <= 1'bx;
					unload_ch2 <= 1'bx;
					unload_ch3 <= 1'bx;
					txstart <= 1'bx;
					nextbyte <= 1'bx;
					overrun_next <= 1'bx;
				end
			
		endcase
	end
endmodule

// Sequential part

module sys_sm_seq(clk, rstn, overrun_next, sys_sm_next, channel_next, overrun_cur, sys_sm_cur, channel_cur);
	input clk;
	input rstn;
	input overrun_next;
	input [2:0] sys_sm_next;
	input [1:0] channel_next;
	output reg overrun_cur;
	output reg [2:0] sys_sm_cur;
	output reg [1:0] channel_cur;
	
	always @(posedge clk or negedge rstn) begin
		if(rstn == 0) begin
			channel_cur <= 0;
			overrun_cur <= 0;
			sys_sm_cur <= `ATTN_CHECK;
		end else begin
			overrun_cur <= overrun_next;
			channel_cur <= channel_next;
			sys_sm_cur <= sys_sm_next;
		end
	end
endmodule

// Channel multiplexer

module channel_mux(ch0, ch1, ch2, ch3, channel, dataout);
	input [7:0] ch0;
	input [7:0] ch1;
	input [7:0] ch2;
	input [7:0] ch3;
	input [1:0] channel;
	output reg [7:0] dataout;
	
	always @(*) begin
		case(channel)
			0:
				dataout <= ch0;
				
			1:
				dataout <= ch1;
				
			2:
				dataout <= ch2;
			
			3:
				dataout <= ch3;
				
			default:
				dataout <= 0;
		endcase
	end
endmodule

// Convert hex nybble to ascii digit
	
module nybbletohexdigit(nybble, digit);
	input [3:0] nybble;
	output reg [7:0] digit;

	always @(*) begin
		case(nybble)
			0:
				digit <= 8'h30;
			1:
				digit <= 8'h31;
			2:
				digit <= 8'h32;
			3:	
				digit <= 8'h33;
			4:
				digit <= 8'h34;
			5:
				digit <= 8'h35;
			6:
				digit <= 8'h36;
			7:
				digit <= 8'h37;
			8:
				digit <= 8'h38;
			9:
				digit <= 8'h39;
			10:
				digit <= 8'h41;
			11:
				digit <= 8'h42;
			12:
				digit <= 8'h43;
			13:
				digit <= 8'h44;
			14:
				digit <= 8'h45;
			15:
				digit <= 8'h46;
			default:
				digit <= 8'bxxxxxxxx;
		endcase
	end
endmodule
				

/*
* This is the module just below the root which is the top level generic verilog code
* no device specific code is in this module, that is all done in the root module.
*/

module system(clk, rstn, datain_ch0, datain_ch1, datain_ch2, datain_ch3, serialout);
	input clk;				// Counter and fifo clock
	input rstn;				// Global reset, low true
	input datain_ch0;		// Channel 0 Data bit input
	input datain_ch1;		// Channel 1 Data bit input	
	input datain_ch2;		// Channel 2 Data bit input
	input datain_ch3;		// Channel 3 Data bit input		
	output serialout;   	// Async serial data out


	
	wire unload_ch0;
	wire clearoverrun_ch0;
	wire attention_ch0;
	wire overrun_ch0;
	wire unload_ch1;
	wire clearoverrun_ch1;
	wire attention_ch1;
	wire overrun_ch1;
	wire unload_ch2;
	wire clearoverrun_ch2;
	wire attention_ch2;
	wire overrun_ch2;
	wire unload_ch3;
	wire clearoverrun_ch3;
	wire attention_ch3;
	wire overrun_ch3;
	wire txdone;
	wire nextbyte;
	wire txstart;
	wire overrun_cur;
	wire overrun_next;
	wire next_scan_channel;

	wire [2:0] sys_sm_cur;
	wire [2:0] sys_sm_next;
	wire [2:0] byteaddr;
	wire [1:0] channel_cur;
	wire [1:0] channel_scan;
	wire [1:0] channel_next;
	wire [7:0] txdata;
	wire [7:0] dataout_ch0;
	wire [7:0] dataout_ch1;
	wire [7:0] dataout_ch2;
	wire [7:0] dataout_ch3;
	
	wire [7:0] dataout;
	wire [7:0] hexchar;
	wire [3:0] hexnybble;
	wire [62:0] count;
	
	
	// Instantiate free running counter
	counter ctr63(
		.clk(clk),
		.rstn(rstn),
		.up(1'b1),
		.down(1'b0),
		.count(count)
		);
	defparam ctr63.WIDTH = 63;
	
	// Scan counter
	counter ctr2(
		.clk(clk),
		.rstn(rstn),
		.up(next_scan_channel),
		.down(1'b0),
		.count(channel_scan)
		);
	defparam ctr2.WIDTH = 2;
	
	// Instantiate channel 0	
	channel ch0(
		.clk(clk),
		.rstn(rstn),
		.datain(datain_ch0),
		.unload(unload_ch0),
		.counterin(count),
		.byteaddr(~byteaddr), // Complemented so high nybbles get sent first
		.clearoverrun(clearoverrun_ch0),
		.overrun(overrun_ch0),
		.attention(attention_ch0),
		.dataout(dataout_ch0)
		);
		
		
	// Instantiate channel 1	
	channel ch1(
		.clk(clk),
		.rstn(rstn),
		.datain(datain_ch1),
		.unload(unload_ch1),
		.counterin(count),
		.byteaddr(~byteaddr), // Complemented so high nybbles get sent first
		.clearoverrun(clearoverrun_ch1),
		.overrun(overrun_ch1),
		.attention(attention_ch1),
		.dataout(dataout_ch1)
		);
		
		// Instantiate channel 2	
	channel ch2(
		.clk(clk),
		.rstn(rstn),
		.datain(datain_ch2),
		.unload(unload_ch2),
		.counterin(count),
		.byteaddr(~byteaddr), // Complemented so high nybbles get sent first
		.clearoverrun(clearoverrun_ch2),
		.overrun(overrun_ch2),
		.attention(attention_ch2),
		.dataout(dataout_ch2)
		);
		
		
		// Instantiate channel 3	
	channel ch3(
		.clk(clk),
		.rstn(rstn),
		.datain(datain_ch3),
		.unload(unload_ch3),
		.counterin(count),
		.byteaddr(~byteaddr), // Complemented so high nybbles get sent first
		.clearoverrun(clearoverrun_ch3),
		.overrun(overrun_ch3),
		.attention(attention_ch3),
		.dataout(dataout_ch3)
		);
		
	// Instantiate channel multiplexer
	channel_mux cmux0(
		.ch0(dataout_ch0),
		.ch1(dataout_ch1),
		.ch2(dataout_ch2),
		.ch3(dataout_ch3),
		.channel(channel_cur),
		.dataout(dataout)
		);
		
	// Instantiate byte counter
	counter bc0(
		.clk(clk),
		.rstn(rstn),
		.up(nextbyte),
		.down(1'b0),
		.count(byteaddr)
		);
	defparam bc0.WIDTH = 3;
	
	// Instantiate nybble to ascii hex converter
	nybbletohexdigit ntohd0(
		.nybble(hexnybble),
		.digit(hexchar)
		);
	
	// Instantiate combinatorial part of state machine
	sys_sm_comb ssc0(
		.sys_sm_cur(sys_sm_cur),
		.channel_cur(channel_cur),
		.channel_scan(channel_scan),
		.byteaddr(byteaddr),
		.channel_data(dataout),
		.attention_ch0(attention_ch0),
		.attention_ch1(attention_ch1),
		.attention_ch2(attention_ch2),
		.attention_ch3(attention_ch3),
		.overrun_cur(overrun_cur),
		.overrun_ch0(overrun_ch0),
		.overrun_ch1(overrun_ch1),
		.overrun_ch2(overrun_ch2),
		.overrun_ch3(overrun_ch3),
		.utxdone(txdone),
		.hexchar(hexchar),
	
		.clearoverrun_ch0(clearoverrun_ch0),
		.clearoverrun_ch1(clearoverrun_ch1),
		.clearoverrun_ch2(clearoverrun_ch2),
		.clearoverrun_ch3(clearoverrun_ch3),
		.txstart(txstart),
		.nextbyte(nextbyte),
		.overrun_next(overrun_next),
		.unload_ch0(unload_ch0),
		.unload_ch1(unload_ch1),
		.unload_ch2(unload_ch2),
		.unload_ch3(unload_ch3),
		.next_scan_channel(next_scan_channel),
		.sys_sm_next(sys_sm_next),
		.channel_next(channel_next),
		.txdata(txdata),
		.hexnybble(hexnybble)
		);


	
	// Instantiate sequential part of state machine
	sys_sm_seq sss0(
		.clk(clk),
		.rstn(rstn),
		.overrun_next(overrun_next),
		.sys_sm_next(sys_sm_next),
		.channel_next(channel_next),
		.overrun_cur(overrun_cur),
		.sys_sm_cur(sys_sm_cur),
		.channel_cur(channel_cur)
		);
	
	
		
	// Instantiate a UART Transmitter	
	utx utx0(
		.clk(clk),
		.rstn(rstn),
		.load(txstart),
		.inbyte(txdata),
		.serialout(serialout),
		.done(txdone)
		);
		

endmodule


	
	
