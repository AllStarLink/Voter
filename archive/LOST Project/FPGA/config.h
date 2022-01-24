

// define this if you wish to an internal PLL to generate system clock. Be aware that
// the part requires a minimum input clock freq. of 19 MHz for proper PLL operation.
// NOTE: be sure to ALSO define the proper clock pin in the 'root.ucf' file !!
//`define USEPLL

// Fclk must be 100.000000 MHz

`ifdef USEPLL

// Given PLL example is for 50 MHz Fin (input clock), 100 Mhz system clock Fclk (out),
// and 600 Mhz Fvco

// PLL_CLKIN_PERIOD is Fin clock period in uSecs. 
// e.g. for 50 Mhz Fin, would be 20.0
`define PLL_CLKIN_PERIOD 20.0

// PLL_MULT is the value Fvco / Fin (for this part, 400.0 MHz <= Fvco <= 1080.0 MHz)
// e.g. for 50 MHz Fin and Fvco = 600.0 MHz, PLL_MULT is 12 (50 * 12 = 600)
`define PLL_MULT 12

// PLL_CLKDIV_MAIN is the value Fvco / Fclk
// e.g. for Fvco @ 600.0 MHz, PLL_CLKDIV_MAIN is 6 (600.0 / 6 = 100.0)
`define PLL_CLKDIV_MAIN 12//6

// PLL_CLKDIV_AUX is the value Fvco / Faux
// e.g. for Faux @ 20.0 MHz, and Fvco @ 600.0 MHz, PLL_CLKDIV_AUX is 30 (600.0 / 30 = 20.0)
`define PLL_CLKDIV_AUX 30

`endif

// FIFO words per channel. Parameter is the power of 2 for a given depth.
// Mess with this to try on different FPGA's.
// The Lattice ICEstick can support a maximum depth of 8 or 256 bytes (w/ 4 channels)

`ifdef USEPLL // PLL takes up much routable room, so we want have deep FIFO!! :-(

`define FIFO_DEPTH 9 
//`define FIFO_DEPTH 8 // MAX depth for Lattice design with 4 channels.

`else

`define FIFO_DEPTH 11

`endif


`define BAUDRATE_DIVISOR 868 // Set this to whatever baudrate you need according to the following:
// divisor == Fclk/desired_baud_rate
