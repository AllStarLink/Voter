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
 * This is the top level or 'root' of the design.
 *
 * This level should contain device-specific changes, signal inversions,
 * tri-state and testability logic.
 *
 * For example, PLL's and other device specific things would
 * be instantiated at this level. 
 *
 *
 * Modules further down in the heirarchy should be generic so that
 * different FPGA's can be retargeted with minimal fuss.
 *
 */
 
 `include "config.h"
 
 `ifndef USEPLL // For PLL-less operation
  
module root(clk, rstn, datain_ch0, datain_ch1, datain_ch2, datain_ch3, serialout);
	input clk;
	input rstn;
	input datain_ch0;
	input datain_ch1;
	input datain_ch2;
	input datain_ch3;
	output serialout;
	
	system sys0(
		.clk(clk),
		.rstn(rstn),
		.datain_ch0(datain_ch0),
		.datain_ch1(datain_ch1),
		.datain_ch2(datain_ch2),
		.datain_ch3(datain_ch3),
		.serialout(serialout)
	);

endmodule

`else  // otherwise, generate with PLL

module root(clk, rstn, datain_ch0, datain_ch1, datain_ch2, datain_ch3, serialout, clk20, locked);
	input clk;
	input rstn;
	input datain_ch0;
	input datain_ch1;
	input datain_ch2;
	input datain_ch3;
	output serialout;
	output clk20;
	output locked;
	wire fbouttofbin;
	wire clk100;
	wire rstn_int;
	wire locked_int;
	wire CLKOUT2;
	wire CLKOUT3;
	wire CLKOUT4;
	wire CLKOUT5;
					
	system sys0(
		.clk(clk100),
		.rstn(rstn_int),
		.datain_ch0(datain_ch0),
		.datain_ch1(datain_ch1),
		.datain_ch2(datain_ch2),
		.datain_ch3(datain_ch3),
		.serialout(serialout)
	);
	
	
  PLL_BASE #(
	.BANDWIDTH("OPTIMIZED"), // "HIGH", "LOW" or "OPTIMIZED"
	.CLKFBOUT_MULT(`PLL_MULT), // Multiplication factor for all output clocks
	.CLKFBOUT_PHASE(0.0), // Phase shift (degrees) of all output clocks
	.CLKIN_PERIOD(`PLL_CLKIN_PERIOD), // Clock period (ns) of input clock on CLKIN
	.CLKOUT0_DIVIDE(`PLL_CLKDIV_MAIN), // Division factor for CLKOUT0 (1 to 128)
	.CLKOUT0_DUTY_CYCLE(0.5), // Duty cycle for CLKOUT0 (0.01 to 0.99)
	.CLKOUT0_PHASE(0.0), // Phase shift (degrees) for CLKOUT0 (0.0 to 360.0)
	.CLKOUT1_DIVIDE(`PLL_CLKDIV_AUX), // Division factor for CLKOUT1 (1 to 128)
	.CLKOUT1_DUTY_CYCLE(0.5), // Duty cycle for CLKOUT1 (0.01 to 0.99)
	.CLKOUT1_PHASE(0.0), // Phase shift (degrees) for CLKOUT1 (0.0 to 360.0)
	.CLKOUT2_DIVIDE(1), // Division factor for CLKOUT2 (1 to 128)
	.CLKOUT2_DUTY_CYCLE(0.5), // Duty cycle for CLKOUT2 (0.01 to 0.99)
	.CLKOUT2_PHASE(0.0), // Phase shift (degrees) for CLKOUT2 (0.0 to 360.0)
	.CLKOUT3_DIVIDE(1), // Division factor for CLKOUT3 (1 to 128)
	.CLKOUT3_DUTY_CYCLE(0.5), // Duty cycle for CLKOUT3 (0.01 to 0.99)
	.CLKOUT3_PHASE(0.0), // Phase shift (degrees) for CLKOUT3 (0.0 to 360.0)
	.CLKOUT4_DIVIDE(1), // Division factor for CLKOUT4 (1 to 128)
	.CLKOUT4_DUTY_CYCLE(0.5), // Duty cycle for CLKOUT4 (0.01 to 0.99)
	.CLKOUT4_PHASE(0.0), // Phase shift (degrees) for CLKOUT4 (0.0 to 360.0)
	.CLKOUT5_DIVIDE(1), // Division factor for CLKOUT5 (1 to 128)
	.CLKOUT5_DUTY_CYCLE(0.5), // Duty cycle for CLKOUT5 (0.01 to 0.99)
	.CLKOUT5_PHASE(0.0), // Phase shift (degrees) for CLKOUT5 (0.0 to 360.0)
	.COMPENSATION("SYSTEM_SYNCHRONOUS"), // "SYSTEM_SYNCHRONOUS",
	// "SOURCE_SYNCHRONOUS", "INTERNAL", "EXTERNAL",
	// "DCM2PLL", "PLL2DCM"
	.DIVCLK_DIVIDE(1), // Division factor for all clocks (1 to 52)
	.REF_JITTER(0.100) // Input reference jitter (0.000 to 0.999 UI%)
	) PLL_BASE_inst (
		.CLKFBOUT(fbouttofbin), // General output feedback signal
		.CLKOUT0(clk100), // One of six general clock output signals
		.CLKOUT1(clk20), // One of six general clock output signals
		.CLKOUT2(CLKOUT2), // One of six general clock output signals
		.CLKOUT3(CLKOUT3), // One of six general clock output signals
		.CLKOUT4(CLKOUT4), // One of six general clock output signals
		.CLKOUT5(CLKOUT5), // One of six general clock output signals
		.LOCKED(locked_int), // Active high PLL lock signal
		.CLKFBIN(fbouttofbin), // Clock feedback input
		.CLKIN(clk), // Clock input
		.RST(~rstn) // Asynchronous PLL reset
	);
	
	assign locked = locked_int;
	assign rstn_int = rstn & locked_int;
		

endmodule


`endif

  


	
