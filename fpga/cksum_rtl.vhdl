--
-- Copyright (C) 2009-2012 Chris McClelland
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
--
library ieee;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

--chanAddr_out   => chanAddr,

-- Host >> FPGA pipe:
--h2fData_in    => h2fData,			-- data lines used when the host writes to a channel
--h2fValid_in   => h2fValid,			-- '1' means "on the next clock rising edge, please accept the data on h2fData"
--h2fReady_out    => h2fReady,			-- channel logic can drive this low to say "I'm not ready for more data yet"

-- Host << FPGA pipe:
--f2hData_out     => f2hData,			-- data lines used when the host reads from a channel
--f2hValid_out    => f2hValid,			-- channel logic can drive this low to say "I don't have data ready for you"
--f2hReady_in   => f2hReady			-- '1' means "on the next clock rising edge, put your next byte of data on f2hData"

			---- DVR interface -> Connects to comm_fpga module
			--chanAddr_in  => chanAddr,
			--h2fData_in   => h2fData,
			--h2fValid_in  => h2fValid,
			--h2fReady_out => h2fReady,
			--f2hData_out  => f2hData,
			--f2hValid_out => f2hValid,
			--f2hReady_in  => f2hReady,
			

architecture rtl of swled is
	-- Flags for display on the 7-seg decimal points
	signal flags                   : std_logic_vector(3 downto 0);
	signal buf1, buf2, buf3, buf4, buf5, buf6, buf7, buf8: std_logic_vector(7 downto 0)  := "00000000";
	signal reset_buf: std_logic:= '0';
	signal load1, load2, load3, load4, load5, load6, load7, load8: std_logic_vector(7 downto 0)  := (others => '0');
	signal light_it_up, burn_it_down: std_logic_vector(7 downto 0)  := (others => '0'); -- light is dirn in which sig is displayed. burn_it_down is opposite of that dirn
	signal light_pointer: unsigned(31 downto 0) := to_unsigned(0, 32);
	signal light_pointer_helpr: unsigned(31 downto 0):= to_unsigned(0, 32);
	signal li_po_hel_mod3: unsigned(31 downto 0);
	signal decbuff640: std_logic_vector(63 downto 0) := (others => '0');
	signal dynamicbuff320: std_logic_vector(31 downto 0) := (others => '0');

	signal num_cycles: unsigned(31 downto 0) := to_unsigned(0, 32);
	signal data_update_cycle: unsigned(31 downto 0) := to_unsigned(0, 32);
	constant clk_complete: unsigned(31 downto 0) := to_unsigned(48000000, 32);

	-- Registers implementing the channels
	signal checksum, checksum_next : std_logic_vector(15 downto 0) := (others => '0');
	signal reg0, reg0_next         : std_logic_vector(7 downto 0)  := (others => '0');
	signal inp_state: unsigned(31 downto 0):= to_unsigned(0, 32);
	signal overall_state: unsigned(31 downto 0):= to_unsigned(0, 32);
	constant chani:unsigned(31 downto 0):= to_unsigned(0, 32);
	constant chanip1:unsigned(31 downto 0):= to_unsigned(1, 32);
	constant coordinates: std_logic_vector(31 downto 0) := "00100010001000100010001000100010"; -- MS 4 bits are X coordinates and LS 4 bits are Y coordinates
			-- Repeated to make 32 bits
	--signal final_coordinates: std_logic_vector(31 downto 0) := (others => '0');
	signal enable_enc, enable_dec, enable_send: std_logic := '0';
	signal reset_enc, reset_dec, reset_send: std_logic := '1';
	signal done_enc, done_dec, done_send: std_logic := '0';
	--constant key: std_logic_vector(31 downto 0) := "10101010101010101010101010101010";
	constant key: std_logic_vector(31 downto 0) := "11100000000000000000001100000000";

	--constant key: std_logic_vector(31 downto 0) := "10101011101101100101101000101101";
	signal coordinates_enc: std_logic_vector(31 downto 0) := (others => '0');
	signal dummy_data: std_logic_vector(31 downto 0):= "10101011101010011001010011000100";

	signal result_dec: std_logic_vector(31 downto 0) := (others => '0');
	signal input_dec: std_logic_vector(31 downto 0):= (others => '0');

	signal board_note: std_logic_vector(7 downto 0):= (others => '0'); -- Host will read from this

	signal final_disp: std_logic_vector(7 downto 0) := (others => '0');
	signal dec_donnn, inp_giv:STD_LOGIC:= '0';
	signal enc_inp, dec_inp: std_logic_vector(31 downto 0):= (others => '0');
	signal enc_out: std_logic_vector(31 downto 0);
	signal send_inp:std_logic_vector(31 downto 0) := (others => '0');
	signal time_cntr: unsigned(33 downto 0) := (others => '0');
	signal swaggy_vec: STD_LOGIC_VECTOR(33 downto 0) := "0000001110010011100001110000000000";
	--signal swaggy_vec: STD_LOGIC_VECTOR(33 downto 0) := "1011011100011011000000000000000000";
	--constant time_lim:unsigned(33 downto 0) := to_unsigned(46875, 16)*to_unsigned(262144, 18);
	constant time_lim:unsigned(33 downto 0) := unsigned(swaggy_vec);
	signal others_ctrla, others_ctrlb: STD_LOGIC:= '0';
	constant ack1: std_logic_vector(31 downto 0):= "10110111101101111011011110110111" ;
	constant ack2: std_logic_vector(31 downto 0):= "11010010110100101101001011010010" ;

	signal enc_ack1, enc_ack2: std_logic_vector(31 downto 0);
	signal reset_acke, done_ack1e, done_ack2e, enable_ack2e, enable_ack1e: std_logic;
	constant zeros34: std_logic_vector(33 downto 0):= "0000000000000000000000000000000000";
	constant one34: STD_LOGIC_VECTOR(33 downto 0) := "0000000000000000000000000000000001";
	signal f2hValid_out_clone: std_logic;
	signal deb_reg: std_logic_vector(7 downto 0);
	signal deb_sig:std_logic;
	signal traffic_sw1, traffic_sw2: std_logic:= '0';
	signal swin_i, opp_swini:std_logic:= '0';
	--signal i: unsigned(7 downto 0):= to_unsigned(0, 32);
	signal i2: std_logic_vector(6 downto 0):= "0000010" ;
	signal i2plus1: std_logic_vector(6 downto 0):= "0000011" ;
	--signal corr_chan: STD_LOGIC:= '0';
	--signal opp_swini: std_logic:= '0';
	signal zeus, dark_lord: std_logic:= '0';
	signal num_cycles_reset: std_logic:= '0';



	--
	signal final_project_wait_cycles: unsigned(31 downto 0) := to_unsigned(0, 32);
	signal up_push: std_logic;
	signal down_push: std_logic;
	signal left_push: std_logic;
	signal right_push: std_logic;


	--uart_rx	: in std_logic;
	--uart_tx	: out std_logic;
	--Signals pulled on Atlys LEDs for debugging
	--wr_en  : out std_logic; --20102016
	--rd_en  : out std_logic; --20102016
	--full   : out std_logic; --20102016
	--empty  : out std_logic); --20102016	

	signal rst : STD_LOGIC;
	--
	signal deb_btn_raw_l, deb_btn_raw_r, deb_btn_raw_rst, deb_btn_raw_u, deb_btn_raw_d: std_logic;
	signal deb_btn_fil_l, deb_btn_fil_r, deb_btn_fil_rst, deb_btn_fil_u, deb_btn_fil_d: std_logic;
	-- signal
	-- -----------------------
	-- signals for uart
	signal basic_uart_reset: std_logic;                       -- reset
    signal rx_data: std_logic_vector(7 downto 0); -- received byte
    signal rx_enable:  std_logic;                  -- validates received byte (1 system clock spike)
    signal tx_data: std_logic_vector(7 downto 0);  -- byte to send
    signal tx_enable: std_logic;                   -- validates byte to send if tx_ready is '1'
    signal tx_ready: std_logic;                   -- if '1', we can send a new byte, otherwise we won't take it
    
    -- Physical interface
    --signal rx: std_logic;
    --signal tx: std_logic;


    signal uart_data_buffer: std_logic_vector(7 downto 0); -- received byte
    signal uart_fliper, uart_previous : std_logic:= '0';
	--signal 

	component encrypter is
	    Port ( clock : in  STD_LOGIC;
	           K : in  STD_LOGIC_VECTOR (31 downto 0);
	           P : in  STD_LOGIC_VECTOR (31 downto 0);
	           C : out  STD_LOGIC_VECTOR (31 downto 0);
	           reset : in  STD_LOGIC;
			   done: out std_logic:='0';
	           enable : in  STD_LOGIC);
	end component;

	component decrypter is
	    Port ( clock : in  STD_LOGIC;
	           K : in  STD_LOGIC_VECTOR (31 downto 0);
	           C : in  STD_LOGIC_VECTOR (31 downto 0);
	           P : out  STD_LOGIC_VECTOR (31 downto 0);
	           reset : in  STD_LOGIC;
			   done: out std_logic:='0';
	           enable : in  STD_LOGIC);
	end component;


	component sender is
		Port (	clock : in  STD_LOGIC;
				read_sig: in STD_LOGIC;
				inp_reg_once: in STD_LOGIC_VECTOR(31 downto 0); -- It will send from left to right
				reset : in  STD_LOGIC;
				out_reg: out STD_LOGIC_VECTOR(7 downto 0);
				out_sig: out STD_LOGIC:='0';
				done: out std_logic:='0';
				--enable : in  STD_LOGIC);

				enable : in  STD_LOGIC;
	           	--corr_chan: in STD_LOGIC:= '0';
	           	chanAddr_in: in std_logic_vector(6 downto 0) ;
	           	i2: in STD_LOGIC_VECTOR(6 downto 0);
				deb_reg: out STD_LOGIC_VECTOR(7 downto 0));
	end component;

	--component send_data is
	--    Port ( 	clock : in  STD_LOGIC;
	--           	f2hReady_in_value : in  STD_LOGIC;
	--           	data_external : in  STD_LOGIC_VECTOR (31 downto 0);
	--           	reset : in  STD_LOGIC;
	--			output_to_send: out std_logic_vector(7 downto 0):= (others => '0'); -- Host will read from this
	--			f2hValid_out_value : out STD_LOGIC:='0';
	--			done: out std_logic:='0';
	--           	enable : in STD_LOGIC := '0';
	--           	corr_chan: in STD_LOGIC:= '0';
	--           	deb_reg: out STD_LOGIC_VECTOR(7 downto 0));
	--end component;


	component debouncer
        port(clk: in STD_LOGIC;
            button: in STD_LOGIC;
            button_deb: out STD_LOGIC);
    end component;



    component basic_uart is
      generic (
        DIVISOR: natural  -- DIVISOR = 100,000,000 / (16 x BAUD_RATE)
        -- 2400 -> 2604
        -- 9600 -> 651
        -- 115200 -> 54
        -- 1562500 -> 4
        -- 2083333 -> 3
      );
      port (
        clk: in std_logic;                         -- clock
        reset: in std_logic;                       -- reset
        
        -- Client interface
        rx_data: out std_logic_vector(7 downto 0); -- received byte
        rx_enable: out std_logic;                  -- validates received byte (1 system clock spike)
        tx_data: in std_logic_vector(7 downto 0);  -- byte to send
        tx_enable: in std_logic;                   -- validates byte to send if tx_ready is '1'
        tx_ready: out std_logic;                   -- if '1', we can send a new byte, otherwise we won't take it
        
        -- Physical interface
        rx: in std_logic;
        tx: out std_logic
      );
    end component;



	--component uart is
	--port (clk 	 : in std_logic;
	--		rst 	 : in std_logic;
	--		rx	 	 : in std_logic;
	--		tx	 	 : out std_logic;
	--		--Signals pulled on Atlys LEDs for debugging
	--		Bwr_en  : out std_logic; --20102016--
	--		Brd_en  : out std_logic; --20102016--
	--		Bfull   : out std_logic; --20102016--
	--		Bempty  : out std_logic); --20102016--
	--end component uart;	



begin                                                                     --BEGIN_SNIPPET(registers)
	--deb_btn_raw_l, deb_btn_raw_r, deb_btn_raw_rst, deb_btn_raw_u, deb_btn_raw_d
	deb_btn_raw_l <= but_left;
	deb_btn_raw_r <= but_rite;
	deb_btn_raw_u <= but_up;
	deb_btn_raw_d <= but_down;
	deb_btn_raw_rst <= but_reset;
	--rst <= not(reset_btn);
	--i_uart : uart port map (clk => clk_in, rst => rst, rx => uart_rx, tx => uart_tx, Bwr_en => wr_en, Brd_en => rd_en, Bfull => full, Bempty => empty);  --20102016
	Our_Enc: encrypter port map (clk_in, key, enc_inp, enc_out, reset_enc, done_enc, enable_enc);
	Our_AEnc1: encrypter port map(clk_in, key, ack1, enc_ack1, reset_acke, done_ack1e, enable_ack1e);
	Our_AEnc2: encrypter port map(clk_in, key, ack2, enc_ack2, reset_acke, done_ack2e, enable_ack2e);
	Our_Dec: decrypter port map (clk_in, key, input_dec,   result_dec,      reset_dec, done_dec, enable_dec);
	Our_Sendr: sender port map (clk_in, f2hReady_in, send_inp, reset_send, board_note, f2hValid_out_clone, done_send, enable_send, chanAddr_in, i2, deb_reg);
	left_debouncer: debouncer port map(clk_in, deb_btn_raw_l, deb_btn_fil_l);
	rite_debouncer: debouncer port map(clk_in, deb_btn_raw_r, deb_btn_fil_r);
	rest_debouncer: debouncer port map(clk_in, deb_btn_raw_rst, deb_btn_fil_rst);
	uupp_debouncer: debouncer port map(clk_in, deb_btn_raw_u, deb_btn_fil_u);
	down_debouncer: debouncer port map(clk_in, deb_btn_raw_d, deb_btn_fil_d);
	our_basic_uart: basic_uart
	generic map (DIVISOR => 1250) -- 2400
	port map (
		clk => clk_in, reset => basic_uart_reset,
		rx_data => rx_data, rx_enable => rx_enable,
		tx_data => tx_data, tx_enable => tx_enable, tx_ready => tx_ready,
		rx => rx,
		tx => tx
	);

	--Our_Sendr: send_data port map (clk_in, f2hReady_in, send_inp, reset_send, board_note, f2hValid_out_clone, done_send, enable_send, deb_reg);
	-- Infer registers
	f2hValid_out <= f2hValid_out_clone;
	--process(i2, clk_in)
	--begin
	--	--2i <= to_unsigned(2, 8) * i;
	--	--2iplus1 <= to_unsigned(2, 8) * 
	--	if (rising_edge(clk_in)) then
	--	if (unsigned(chanAddr_in) = unsigned(i2plus1)) then
	--		corr_chan <= '1';
	--	else
	--		corr_chan <= '0';
	--	end if;
	--	end if;
	--	--corr_chan <= (unsigned(chanAddr_in) = unsigned(i2plus1));
	--end process;

	-- UART data load in buffer
	process	(rx_enable, clk_in)
	begin
		if (rising_edge(clk_in) and rx_enable = '1') then
			uart_data_buffer <= rx_data;
			uart_fliper <= not uart_fliper;
			if (reset_buf = '1') then
				buf1 <= "00000000";
				buf2 <= "00000000";
				buf3 <= "00000000";
				buf4 <= "00000000";
				buf5 <= "00000000";
				buf6 <= "00000000";
				buf7 <= "00000000";
				buf8 <= "00000000";
			else
				if (rx_data(5 downto 3)="000") then  
					buf1<= rx_data;
				elsif  (rx_data(5 downto 3)="001") then  
					buf2<= rx_data;
				elsif  (rx_data(5 downto 3)="010") then  
					buf3<= rx_data;
				elsif  (rx_data(5 downto 3)="011") then  
					buf4<= rx_data;
				elsif  (rx_data(5 downto 3)="100") then  
					buf5<= rx_data;
				elsif  (rx_data(5 downto 3)="101") then  
					buf6<= rx_data;
				elsif  (rx_data(5 downto 3)="110") then  
					buf7<= rx_data;
				elsif  (rx_data(5 downto 3)="111") then  
					buf8<= rx_data;
				end if ;
			end if;
		end if;
	end process;

	--


	process(clk_in)
	begin
		if ( rising_edge(clk_in) ) then
			if ( reset_in = '1' ) then
				reg0 <= (others => '0');
				checksum <= (others => '0');
				decbuff640 <= (others => '0');
				dynamicbuff320 <= (others => '0');
				enable_enc <= '0';
				inp_state <= to_unsigned(0, 32);
				time_cntr <= (others => '0');
				--traffic_sw2 <= '0';
				traffic_sw1 <= '0';
			else
				reg0 <= reg0_next;
				checksum <= checksum_next;
				if (h2fValid_in = '1' and chanAddr_in = i2plus1 and (others_ctrlb = others_ctrla)) then
					dynamicbuff320 <= dynamicbuff320(23 downto 0) & h2fData_in;
					if (others_ctrlb = others_ctrla) then
						inp_state <= (inp_state + 1) mod 20;
					--else 
					end if;
					inp_giv <= '1';
				else
					dynamicbuff320 <= dynamicbuff320;
					if ( not (others_ctrlb = others_ctrla)) then
						inp_state <= to_unsigned(0, 32);
						others_ctrlb <= not others_ctrlb;
					end if;
				end if;

				-- reset state
				if (deb_btn_fil_rst = '1') then
					overall_state <= to_unsigned(9000, 32);

				elsif(overall_state = 0) then 
					if (light_pointer_helpr <3) then
						zeus <= '1';
						overall_state <= to_unsigned(0, 32);
					else
						zeus <= '0';
						--overall_state <= to_unsigned(169, 32);
						dark_lord <= '1';
						overall_state <= to_unsigned(1, 32);
						num_cycles_reset <= '1';
					end if;
					
				elsif (overall_state = 1) then
					num_cycles_reset <= '0';
					overall_state <= to_unsigned(8, 32);
				elsif (overall_state = 8) then
					zeus <= '0';
					--overall_state <= to_unsigned(169, 32);
					dark_lord <= '1';
					enable_enc <= '0'; 

					enable_ack1e <= '0';
					enable_ack2e <= '0';

					--Our_AEnc1: encrypter port map(clk_in, key, ack1, enc_ack1, reset_acke, done_ack1e, enable_ack1e);

					--signal enc_ack1, enc_ack2: std_logic_vector(31 downto 0);
					--signal reset_acke, done_ack1e, done_ack2e, enable_ack2e, enable_ack1e;
					enc_inp <= coordinates;
					reset_enc<='1';
					reset_acke <= '1';
					overall_state <= to_unsigned(9, 32);
				elsif (overall_state = 9) then
					--deb_sig <= '0';
					if (done_enc = '0' or done_ack1e = '0' or  done_ack2e = '0')then
						enable_enc <= '1';
						enable_ack1e <= '1';
						enable_ack2e <= '1';
						reset_acke <= '0';
						reset_enc <= '0';
						overall_state <= to_unsigned(9, 32);
					else
						enable_enc <= '0';
						coordinates_enc <= enc_out;
						enable_send <= '0';
						enable_ack1e <= '0';
						enable_ack2e <= '0';
						reset_send <= '1';

						send_inp <= enc_out;
						--send_inp <= dummy_data;
						overall_state <= to_unsigned(10, 32);
					end if;
				elsif (overall_state = 10) then
					--deb_sig <= '0';
					if(done_send = '0') then
						enable_send <= '1';
						reset_send <= '0';
						overall_state <= to_unsigned(10, 32);
					else
						--deb_sig <= '1';
						enable_send <= '0';
						reset_send <= '1';
						time_cntr <= unsigned(zeros34);
						overall_state <= to_unsigned(20, 32);
						--time_cntr <= unsigned(zeros34);
					end if;
				elsif (overall_state = 20) then
					deb_sig <=  '0';
					if (not (inp_state = 4)) then
						if (time_cntr > time_lim) then
							overall_state <= to_unsigned(8, 32);
							time_cntr <= (others => '0');
							others_ctrla <= not others_ctrla;
						else
							overall_state <= to_unsigned(20, 32);
							time_cntr <= time_cntr + unsigned(one34);
						end if;
					else

						if (dynamicbuff320 = coordinates_enc) then
							enable_send <= '0';
							reset_send <= '1';
							send_inp <= enc_ack1;
							overall_state <= to_unsigned(30, 32);
						else
							--inp_state <= to_unsigned(0, 32);
							overall_state <=to_unsigned (0, 32);
							others_ctrla <= not others_ctrla;
							time_cntr <= (others => '0');
						end if;
					end if;
				elsif (overall_state = 30) then
					if (done_send = '1') then
						enable_send <= '0';
						--reset_send <= '1';
						overall_state <= to_unsigned(40, 32);
						time_cntr <= unsigned(zeros34);
					else
						enable_send <= '1';
						reset_send <= '0';
						overall_state <= to_unsigned(30, 32);
					end if;
				elsif (overall_state = 40) then
					if (not (inp_state = 8)) then
						--time_cntr <= time_cntr + to_unsigned(1,34);
						if (time_cntr > time_lim) then
							overall_state <= to_unsigned(8, 32);
							time_cntr <= (others => '0');
							others_ctrla <= not others_ctrla;
						else
							time_cntr <= time_cntr + unsigned(one34);
							overall_state <= to_unsigned(40, 32);
						end if;
					else
						if (dynamicbuff320 = enc_ack2) then
							enable_send <= '0';
							send_inp <= enc_ack1;
							overall_state <= to_unsigned(50, 32);
						else
							--inp_state <= to_unsigned(0, 32);
							overall_state <=to_unsigned (0, 32);
							others_ctrla <= not others_ctrla;
							time_cntr <= unsigned(zeros34);
						end if;
					end if;
				elsif (overall_state = 50) then
					if (not (inp_state = 12)) then
						--time_cntr <= time_cntr + to_unsigned(1, 34);
						if (time_cntr > time_lim) then
							overall_state <= to_unsigned(8, 32);
							time_cntr <= (others => '0');
							others_ctrla <= not others_ctrla;
						else
							time_cntr <= time_cntr + unsigned(one34);
							overall_state <= to_unsigned(50, 32);
						end if;
					else
						--time_cntr <= unsigned(zeros34);
						--enable_dec <= '0';
						--reset_dec <= '1';
						enable_dec <= '0';
						reset_dec <= '1';

	--Our_Dec: decrypter port map (clk_in, key, input_dec,   result_dec,      reset_dec, done_dec, enable_dec);

						input_dec <= dynamicbuff320; ---------------------------------------------------
						overall_state <= to_unsigned(60, 32);
						--time_cntr;
						time_cntr <= unsigned(zeros34);

					end if;
				elsif (overall_state = 60) then
					if (done_dec = '0') then
						enable_dec <='1';
						reset_dec <= '0';
						overall_state <= to_unsigned(60, 32);
					else
						decbuff640(63 downto 32) <= result_dec;
						--enable_dec <= '0';
						--reset_dec <= '1';
						enable_enc <= '0';
						enable_dec <= '0';
						reset_dec <= '1';
						reset_enc <= '1';
						enable_send <= '0';
						reset_send <= '1';
						send_inp <= enc_ack1;
						overall_state <= to_unsigned(70, 32);
					end if;
				elsif (overall_state = 70) then
					if (done_send = '0') then
						enable_send <= '1';
						reset_send <= '0';
						overall_state <= to_unsigned(70, 32);
					else
						enable_send <= '0';
						overall_state <= to_unsigned(80, 32);
						time_cntr <= unsigned(zeros34);
					end if;
				elsif (overall_state = 80) then
					if (not (inp_state = 16)) then
						--time_cntr <= time_cntr + to_unsigned(1, 34);
						if (time_cntr > time_lim) then
							overall_state <= to_unsigned(8, 32);
							time_cntr <= (others => '0');
							others_ctrla <= not others_ctrla;
						else
							time_cntr <= time_cntr + unsigned(zeros34); -- r0 1;
							overall_state <= to_unsigned(80, 32);
						end if;
					else
						--time_cntr <= unsigned(zeros3dec_inp4);
						time_cntr <= unsigned(zeros34);
						--enable_dec <= '0';
						--reset_dec <= '1';
						enable_enc <= '0';
						reset_enc <= '1';
						enc_inp <= dynamicbuff320;---------------------------------------------------
						overall_state <= to_unsigned(90, 32);
					end if;
				elsif (overall_state = 90) then
					if (done_enc = '0') then
						--enable_dec <='1';
						--reset_dec <= '0';
						enable_enc <= '1';
						reset_enc <= '0';
						--reset_dec<= '1';
						overall_state <= to_unsigned(90, 32);
					else
						decbuff640(31 downto 0) <= enc_out;
						--enable_dec <= '0';
						enable_enc <= '0';
						enable_send <= '0';
						reset_send <= '1';
						send_inp <= enc_ack1;
						overall_state <= to_unsigned(100, 32);
					end if;
				elsif (overall_state = 100) then
					deb_sig <= '1';
					if (done_send = '0') then
						enable_send <= '1';
						reset_send<= '0';
						overall_state <= to_unsigned(100, 32);
					else
						enable_send <= '0';
						overall_state <= to_unsigned(110, 32);
						time_cntr <= unsigned(zeros34);
					end if;
				elsif (overall_state = 110) then
					if ((not (inp_state = 0)) and (inp_giv = '1')) then
						--time_cntr <= time_cntr + to_unsigned(1, 34);
						if (time_cntr > time_lim) then
							overall_state <= to_unsigned(8, 32);
							time_cntr <= (others => '0');
							others_ctrla <= not others_ctrla;
						else
							time_cntr <= time_cntr + to_unsigned(1, 34); --    r0 1;r0 1;r0 1;r0;w1 22222222;w1 d2d2d2d2;w1 22222222;r0 1;r0 1;r0 1;r0;w1 22222244;r0 1;r0 1;r0 1;r0
							overall_state <= to_unsigned(110, 32); -- r0 1;r0 1;r0 1;r0;w1 22222222;r0 1;r0 1;r0 1;r0;w1 d2d2d2d2;w1 c289d1d8;r0 1;r0 1;r0 1;r0;w1 e1e9f0b8
						end if;
					else
						--enable_dec <= '0';
						--reset_dec <= '1';
						reset_enc <= '1';
						enable_enc <= '0';
						enable_send <= '0';
						if (dynamicbuff320 = enc_ack2) then
							overall_state <= to_unsigned(119, 32);
							--overall_state <= to_unsigned(120, 32);
							dark_lord <= '0';
							zeus <= '0';
							num_cycles_reset <= '1';
							time_cntr <= (others => '0');
						else
							overall_state <= to_unsigned(8, 32);
							others_ctrla <= not others_ctrla;
							time_cntr <= (others => '0');
						end if;
					end if;
				elsif (overall_state = 119) then
					num_cycles_reset <= '0';
					if (light_pointer_helpr = 1) then
						overall_state <= to_unsigned(120, 32);
					else
						overall_state <= to_unsigned(119, 32);
					end if;
				elsif (overall_state = 120) then
					num_cycles_reset <= '0';
					--if (traffic_sw1 = traffic_sw2) then
					--	overall_state <= to_unsigned(120, 32);
					--	traffic_sw1 <= traffic_sw1;
					--else
					--	overall_state <= to_unsigned(130, 32);
					--	--overall_state <= to_unsigned(120, 32);
					--	traffic_sw1 <= not traffic_sw1;
					if (light_pointer_helpr = 0) then
						overall_state <= to_unsigned(130, 32);
					else
						overall_state <= to_unsigned(120, 32);
					end if;
				elsif (overall_state = 130) then
					dark_lord <= '1';
					if (deb_btn_fil_u = '1') then
						overall_state <= to_unsigned(140,32);
					else
						overall_state <= to_unsigned(150, 32);
					end if ;
				elsif (overall_state = 140) then -- send 8bit data
					if (deb_btn_fil_d = '1') then
						-- encryption and send data
						enable_enc <= '0';
						enc_inp <= sw_in & "0000000000000000" & "11111111";
						reset_enc <= '1';
						overall_state <= to_unsigned(141, 32);
					else
						overall_state <= to_unsigned(140,32);
					end if ;
				elsif (overall_state = 141) then
					if (done_enc = '0')then
						enable_enc <= '1';
						reset_enc <= '0';
						overall_state <= to_unsigned(141, 32);
					else
						enable_enc <= '0';
						enable_send <= '0';
						reset_send <= '1';
						send_inp <= enc_out;
						--send_inp <= dummy_data;
						overall_state <= to_unsigned(142, 32);
					end if;
				elsif (overall_state = 142) then
					if(done_send = '0') then
						enable_send <= '1';
						reset_send <= '0';
						overall_state <= to_unsigned(142, 32);
					else
						--deb_sig <= '1';
						enable_send <= '0';
						--reset_send <= '1';
						overall_state <= to_unsigned(160, 32);
						--time_cntr <= unsigned(zeros34);
					end if;

				elsif (overall_state = 150) then -- send dummy data
					--if (down_push = '1') then
						-- encryption and send data
						enable_enc <= '0';
						enc_inp <= "11011000100100111010011100100101"; -- Agreed string
						reset_enc <= '1';
						overall_state <= to_unsigned(151, 32);
					--else
					--	overall_state <= to_unsigned(150,32);
					--end if ;
				elsif (overall_state = 151) then
					if (done_enc = '0')then
						enable_enc <= '1';
						reset_enc <= '0';
						overall_state <= to_unsigned(151, 32);
					else
						enable_enc <= '0';
						enable_send <= '0';
						reset_send <= '1';
						send_inp <= enc_out;
						--send_inp <= dummy_data;
						overall_state <= to_unsigned(152, 32);
					end if;
				elsif (overall_state = 152) then
					if(done_send = '0') then
						enable_send <= '1';
						reset_send <= '0';
						overall_state <= to_unsigned(152, 32);
					else
						--deb_sig <= '1';
						enable_send <= '0';
						--reset_send <= '1';
						overall_state <= to_unsigned(160, 32);
						--time_cntr <= unsigned(zeros34);
					end if;
				elsif (overall_state = 160) then 
					if (deb_btn_fil_l = '1') then
						overall_state <= to_unsigned(170, 32);
						--tx_enable <= '1'; warning 1 is wrong
						--overall_state <= to_unsigned(0, 32);
					else
						overall_state <= to_unsigned(180, 32);
						--overall_state <= 0;
						--overall_state <= to_unsigned(0, 32);
						
					end if;
				--elsif (overall_state = 169) then
				--	if (deb_btn_fil_l = '1') then
				--		overall_state <= to_unsigned(170, 32);
				--	else
				--		overall_state <= to_unsigned(169, 32);
				--	end if;
				elsif (overall_state = 170) then -- send data to UART
					if (deb_btn_fil_r = '1') then
						overall_state <= to_unsigned(171, 32);
						tx_data <= sw_in;
					else
						overall_state <= to_unsigned(170, 32);
					end if;
				--our_basic_uart: basic_uart port map(clk_in, basic_uart_reset, rx_data, rx_enable, tx_data, tx_enable, tx_ready, rx, tx);

				elsif (overall_state = 171) then
					--if (tx_ready = '1') then
					--	tx_enable <= '1';
					--	overall_state <= to_unsigned(172, 32);
					--	--overall_state <= to_unsigned(180, 32);
					--	num_cycles_reset <= '1';
					--elsif (tx_enable = '0') then
					--	tx_enable <= '1';
					--	overall_state <= to_unsigned(171, 32);
					--end if;
					if (tx_ready = '1') then
						tx_enable <= '1';
						tx_data <= sw_in;
						overall_state <= to_unsigned(173, 32);
					else
						overall_state <= to_unsigned(171, 32);
					end if;
				--elsif (overall_state = 172) then
				--	if (tx_ready = '0') then
				--		overall_state <= to_unsigned(173, 32);
				--	else
				--		overall_state <= to_unsigned(172, 32);
				--	end if;
				elsif (overall_state = 173) then
					if (tx_ready = '1') then
						tx_enable <= '0';
						overall_state <= to_unsigned(175, 32);
					else
						tx_enable <= '1';
						overall_state <= to_unsigned(173, 32);
					end if;
				elsif (overall_state = 175) then
					tx_enable <= '0';
					overall_state <= to_unsigned(180, 32);
					
				elsif (overall_state = 180) then
					-- proc received data if any
					-- warning pseudo code
					if (uart_fliper = not uart_previous) then
						overall_state <= to_unsigned(190, 32);
						num_cycles_reset <= '1';
						uart_previous <= uart_fliper;
					else
						--overall_state <= to_unsigned(8, 32);
						num_cycles_reset <= '1';
						overall_state <= to_unsigned(190, 32);
					end if;
					num_cycles_reset <= '0';
				elsif (overall_state = 190) then
					--overall_state <= to_unsigned(0, 32);
					--if (light_pointer_helpr < 23) then
					num_cycles_reset <= '0';
					if (light_pointer_helpr < 5) then
						overall_state <= to_unsigned(190, 32);
						zeus <= '0';
					else
						zeus <= '1';
						overall_state <= to_unsigned(0, 32);
					end if;

					-- wait for 5 seconds then goto 0;
				elsif (overall_state = 9000) then -- send reset bits then goto state 0
					enable_enc <= '0';
					enc_inp <= "10101110101010101010101001101100"; -- Agreed string
					reset_enc <= '1';
					overall_state <= to_unsigned(9001, 32);
				elsif (overall_state = 9001) then
					if (done_enc = '0')then
						enable_enc <= '1';
						reset_enc <= '0';
						overall_state <= to_unsigned(9001, 32);
					else
						enable_enc <= '0';
						enable_send <= '0';
						reset_send <= '1';
						send_inp <= enc_out;
						--send_inp <= dummy_data;
						overall_state <= to_unsigned(9002, 32);
						reset_buf <= '1';

						num_cycles_reset <= '1';
					end if;
				elsif (overall_state = 9002) then
					if(done_send = '0') then
						enable_send <= '1';
						reset_send <= '0';
						overall_state <= to_unsigned(9002, 32);
					else
						--deb_sig <= '1';
						enable_send <= '0';
						reset_buf <= '0';
						--reset_send <= '1';
						overall_state <= to_unsigned(0, 32);
						num_cycles_reset <= '0';
						uart_previous <= uart_fliper;
						--time_cntr <= unsigned(zeros34);
					end if;
				end if ;
			end if;

			if (num_cycles > clk_complete) then
				--light_it_up <= light_it_up_next;
				--light_pointer_helpr <= (light_pointer_helpr + 1) mod 32;
				light_pointer_helpr <= (light_pointer_helpr + 1) mod 24;

				--data_update_cycle <= (data_update_cycle + 1) mod 32; -- previously 16
				data_update_cycle <= (data_update_cycle + 1) mod 24; -- previously 16
				num_cycles <= to_unsigned(0, 32);
			else 
				if (num_cycles_reset = '1') then
					num_cycles <= to_unsigned(0,32);
					data_update_cycle <= to_unsigned(0, 32);
					light_pointer_helpr <= to_unsigned(0, 32);
				else
					num_cycles <= num_cycles + 1;
				end if;
			end if;
		end if;
	end process;

	process(light_pointer_helpr, clk_in)
	begin
		if ( rising_edge(clk_in) ) then
			--if (light_pointer_helpr < 24) then
				light_pointer <= light_pointer_helpr / to_unsigned(3, 32);
				li_po_hel_mod3 <= light_pointer_helpr mod to_unsigned(3, 32);
			--else
			--	light_pointer <= to_unsigned(8, 32);
			--	li_po_hel_mod3 <= to_unsigned(0, 32);
			--end if;
			--if (light_pointer_helpr = 23) then
			--	traffic_sw2 <= not traffic_sw2;
			--	--li_po_hel_mod3 <= to_unsigned(0, 32);
			--end if;
		end if;
	end process;

	process (clk_in, decbuff640, data_update_cycle)
	begin
		if (rising_edge(clk_in)) then
				-- update buffs based on leftmost 32 bits
			if (data_update_cycle = 0) then
				--if (dec_donnn = '1') then
				--	led_out <= "11111111";
				--else
				--	led_out <= "00000111";
				--end if;

				if (not(buf1(7) = decbuff640(63))) then 
					load1<= decbuff640(63 downto 56);
				else 
					if (not(buf1(6) = decbuff640(62))) then
						if (buf1(6)='0') then  
							load1<= buf1;
						else 
							load1<=  decbuff640(63 downto 56);
						end if;
					else 
						if 	(unsigned(buf1(2 downto 0)) > unsigned(decbuff640(58 downto 56 ))) then
							load1<= buf1;
						else 
							load1 <= decbuff640(63 downto 56);
						end if;
					end if ;
				end if;


				if (not(buf2(7) = decbuff640(55))) then 
					load2<= decbuff640(55 downto 48);
				else 
					if (not(buf2(6) = decbuff640(54))) then
						if (buf2(6)='0') then  
							load2<= buf2;
						else 
							load2<=  decbuff640(55 downto 48);
						end if;
					else 
						if 	(unsigned(buf2(2 downto 0))> unsigned(decbuff640(50 downto 48))) then
							load2<= buf2;
						else 
							load2 <= decbuff640(55 downto 48);
						end if;
					end if ;
				end if;

				if (not(buf3(7) = decbuff640(47))) then 
					load3<= decbuff640(47 downto 40);
				else 
					if (not(buf3(6) = decbuff640(46))) then
						if (buf3(6)='0') then  
							load3<= buf3;
						else 
							load3<=  decbuff640(47 downto 40);
						end if;
					else 
						if 	(unsigned(buf3(2 downto 0)) > unsigned(decbuff640(42 downto 40))) then
							load3<= buf3;
						else 
							load3 <= decbuff640(47 downto 40);
						end if;
					end if ;
				end if;




				if (not(buf4(7) = decbuff640(39))) then 
					load4<= decbuff640(39 downto 32);
				else 
					if (not(buf4(6) = decbuff640(38))) then
						if (buf4(6)='0') then  
							load4<= buf4;
						else 
							load4<=  decbuff640(39 downto 32);
						end if;
					else 
						if 	(unsigned(buf4(2 downto 0)) > unsigned(decbuff640(34 downto 32))) then
							load4<= buf4;
						else 
							load4 <= decbuff640(39 downto 32);
						end if;
					end if ;
				end if;


				if (not(buf5(7) = decbuff640(31))) then 
					load5<= decbuff640(31 downto 24);
				else 
					if (not(buf5(6) = decbuff640(30))) then
						if (buf5(6)='0') then  
							load5<= buf5;
						else 
							load5<=  decbuff640(31 downto 24);
						end if;
					else 
						if 	(unsigned(buf5(2 downto 0)) > unsigned(decbuff640(26 downto 24))) then
							load5<= buf5;
						else 
							load5 <= decbuff640(31 downto 24);
						end if;
					end if ;
				end if;


				if (not(buf6(7) = decbuff640(23))) then 
					load6<= decbuff640(23 downto 16);
				else 
					if (not(buf6(6) = decbuff640(22))) then
						if (buf6(6)='0') then  
							load6<= buf6;
						else 
							load6<=  decbuff640(23 downto 16);
						end if;
					else 
						if 	(unsigned(buf6(2 downto 0)) > unsigned(decbuff640(18 downto 16))) then
							load6<= buf6;
						else 
							load6 <= decbuff640(23 downto 16);
						end if;
					end if ;
				end if;

				if (not(buf7(7) = decbuff640(15))) then 
					load7<= decbuff640(15 downto 8);
				else 
					if (not(buf7(6) = decbuff640(14))) then
						if (buf7(6)='0') then  
							load7<= buf7;
						else 
							load7<=  decbuff640(15 downto 8);
						end if;
					else 
						if 	(unsigned(buf7(2 downto 0)) > unsigned(decbuff640(10 downto 8))) then
							load7<= buf7;
						else 
							load7 <= decbuff640(15 downto 8);
						end if;
					end if ;
				end if;


				if (not(buf8(7) = decbuff640(7))) then 
					load8<= decbuff640(7 downto 0);
				else 
					if (not(buf8(6) = decbuff640(6))) then
						if (buf8(6)='0') then  
							load8<= buf8;
						else 
							load8<=  decbuff640(7 downto 0);
						end if;
					else 
						if 	(unsigned(buf8(2 downto 0)) > unsigned(decbuff640(2 downto 0))) then
							load8<= buf8;
						else 
							load8 <= decbuff640(7 downto 0);
						end if ;
					end if;
				end if;
				--load1 <= decbuff640(63 downto 56);
				--load2 <= decbuff640(55 downto 48);
				--load3 <= decbuff640(47 downto 40);
				--load4 <= decbuff640(39 downto 32);
				--load5 <= decbuff640(31 downto 24);
				--load6 <= decbuff640(23 downto 16);
				--load7 <= decbuff640(15 downto 08);
				--load8 <= decbuff640(07 downto 00);


				-- if ()

				-- update buffs based on rightmost 32 bits
			end if;
		end if;
	end process;

	process(clk_in, load1, load2, load3, load4, load5, load6, load7, load8, light_pointer)
	begin
	if ( rising_edge(clk_in) ) then
		if (light_pointer = 0) then
			burn_it_down <= load5;
			light_it_up <= load1;
		elsif (light_pointer = 1) then
			burn_it_down <= load6;
			light_it_up <= load2;
		elsif (light_pointer = 2) then
			burn_it_down <= load7;
			light_it_up <= load3;
		elsif (light_pointer = 3) then
			burn_it_down <= load8;
			light_it_up <= load4;
		elsif (light_pointer = 4) then
			burn_it_down <= load1;
			light_it_up <= load5;
		elsif (light_pointer = 5) then
			burn_it_down <= load2;
			light_it_up <= load6;
		elsif (light_pointer = 6) then
			burn_it_down <= load3;
			light_it_up <= load7;
		elsif (light_pointer = 7) then
			burn_it_down <= load4;
			light_it_up <= load8;
		else
			light_it_up <= "00000000"; -- waiting time
			burn_it_down <= "00000000";
		end if;
	end if;
	end process;

	process(clk_in, light_pointer, sw_in)
	begin
		if ( rising_edge(clk_in) ) then
			if (light_pointer = 0) then 
				swin_i <= sw_in(0);
				opp_swini <= sw_in(4);
			elsif (light_pointer = 1) then 
				swin_i <= sw_in(1);
				opp_swini <= sw_in(5);
			elsif (light_pointer = 2) then 
				swin_i <= sw_in(2);
				opp_swini <= sw_in(6);
			elsif (light_pointer = 3) then
				swin_i <= sw_in(3);
				opp_swini <= sw_in(7);
			elsif (light_pointer = 4) then 
				swin_i <= sw_in(4);
				opp_swini <= sw_in(0);
			elsif (light_pointer = 5) then 
				swin_i <= sw_in(5);
				opp_swini <= sw_in(1);
			elsif (light_pointer = 6) then 
				swin_i <= sw_in(6);
				opp_swini <= sw_in(2);
			elsif (light_pointer = 7) then 
				swin_i <= sw_in(7);
				opp_swini <= sw_in(3);
			else
				swin_i <= sw_in(0); -- 8
				opp_swini <= sw_in(0);
			end if;
		end if;
	end process;

	--final_disp(6 downto 0) <= deb_reg(6 downto 0);
	--final_disp(7) <= traffic_sw1;
	--final_disp(0) <= traffic_sw2;
	--final_disp(6 downto 1) <= std_logic_vector(light_pointer_helpr(5 downto 0));
	--final_disp(7) <= f2hValid_out_clone;
	--final_disp(6 downto 0) <= std_logic_vector(overall_state(6 downto 0));
	--final_disp <= std_logic_vector(overall_state(7 downto 0));
	--final_disp(7 downto 4) <= std_logic_vector(light_pointer(3 downto 0));
	--final_disp(3 downto 0) <= STD_LOGIC_VECTOR(data_update_cycle(3 downto 0));
	--final_disp <= light_it_up;
	--final_disp <= uart_data_buffer;
	--final_disp <= decbuff640(7 downto 0);

	process (light_it_up, light_pointer_helpr, data_update_cycle)
	begin
		-- final_disp
		if (zeus = '1') then
			final_disp <= "11111111";
		elsif (dark_lord = '1') then
			final_disp <= "00000000";
		else
			final_disp(7 downto 5) <= light_it_up(5 downto 3);
			final_disp(4 downto 3) <= "00";
			--if (light_pointer = 0) then
			--	final_disp(4) <= '1';
			--else 
			--	final_disp(4) <= '0';
			--end if;
			--if (data_update_cycle = 0) then
			--	final_disp(3) <= '1';
			--else 
			--	final_disp(3) <= '0';
			--end if;
			--final_disp(3) <= f2hValid_out_clone;
			--final_disp(4) <= f2hReady_in;
			if (light_it_up(7) = '0' or light_it_up(6) = '0') then
				final_disp(2 downto 0) <= "001"; --red
			elsif (light_it_up(7) = '1' and light_it_up(6) = '1') then
				if (swin_i = '0') then
					final_disp(2 downto 0) <= "001"; -- red
				elsif (opp_swini = '0') then
					if (light_it_up(2 downto 0) = "001") then
					--if (burn_it_down(2 downto 0) = "001") then
						final_disp(2 downto 0) <= "010"; -- amber
					else
						final_disp(2 downto 0) <= "100"; -- green
					end if;
				elsif (unsigned(light_it_up(5 downto 3)) < to_unsigned(4, 3)) then
					final_disp(2 downto 0) <= "001"; -- red
				else
					if (li_po_hel_mod3 = 0) then 
						final_disp(2 downto 0) <= "100"; -- green
					elsif (li_po_hel_mod3 = 1) then
						final_disp(2 downto 0) <= "010"; -- amber
					else
						final_disp(2 downto 0) <= "001"; -- red
					end if;					
				end if;
				--if (light_it_up(2 downto 0) = "001" or light_it_up(2 downto 0) = "000") then
				--	final_disp(2 downto 0) <= "010";
				--else
				--	final_disp(2 downto 0) <= "100";
				--end if;
			end if;
		end if;
	end process;




	--process (light_it_up, light_pointer, data_update_cycle)
	--begin
	--	-- final_disp
	--	final_disp(7 downto 5) <= light_it_up(5 downto 3);
	--	final_disp(4 downto 3) <= "00";
	--	--if (light_pointer = 0) then
	--	--	final_disp(4) <= '1';
	--	--else 
	--	--	final_disp(4) <= '0';
	--	--end if;
	--	--if (data_update_cycle = 0) then
	--	--	final_disp(3) <= '1';
	--	--else 
	--	--	final_disp(3) <= '0';
	--	--end if;
	--	--final_disp(3) <= f2hValid_out_clone;
	--	--final_disp(4) <= f2hReady_in;
	--	if (light_it_up(7) = '0' or light_it_up(6) = '0') then
	--		final_disp(2 downto 0) <= "001";
	--	elsif (light_it_up(7) = '1' and light_it_up(6) = '1') then
	--		if (light_it_up(2 downto 0) = "001" or light_it_up(2 downto 0) = "000") then
	--			final_disp(2 downto 0) <= "010";
	--		else
	--			final_disp(2 downto 0) <= "100";
	--		end if;
	--	end if;
	--end process;

 
	-- Drive register inputs for each channel when the host is writing
	reg0_next <=
		h2fData_in when chanAddr_in = i2plus1 and h2fValid_in = '1'
		else reg0;
	--checksum_next <=
	--	std_logic_vector(unsigned(checksum) + unsigned(h2fData_in))
	--		when chanAddr_in = "0000000" and h2fValid_in = '1'
	--	else h2fData_in & checksum(7 downto 0)
	--		when chanAddr_in = "0000001" and h2fValid_in = '1'
	--	else checksum(15 downto 8) & h2fData_in
	--		when chanAddr_in = "0000010" and h2fValid_in = '1'
	--	else checksum;

	-- Select values to return for each channel when the host is reading
	with chanAddr_in select f2hData_out <=
		-- sw_in                  when "0000000",
		board_note				when i2,
		--checksum(15 downto 8) when "0000001",
		--checksum(7 downto 0)  when "0000010",
		--std_logic_vector(overall_state(7 downto 0)) when "0000010",
		--std_logic_vector(inp_state(7 downto 0)) when "0000011",
		--decbuff640(63 downto 56) when "0000100",
		--decbuff640(55 downto 48) when "0000101",
		--decbuff640(47 downto 40) when "0000110",
		--decbuff640(39 downto 32) when "0000111",
		--decbuff640(31 downto 24) when "0001000",
		--decbuff640(23 downto 16) when "0001001",
		--decbuff640(15 downto 08) when "0001010",
		--decbuff640(07 downto 00) when "0001011",
		--load1 					 when "0001111",
		--load2 					 when "0010000", -- 16
		--load3 					 when "0010001",
		--load4 					 when "0010010",
		--load5 					 when "0010011",
		--load6 					 when "0010100",
		--load7 					 when "0010101",
		--load8 					 when "0010110",
		--sw_in					 when "0010111",
		--std_logic_vector(time_lim(33 downto 26))		when "0011000", -- 24
		--std_logic_vector(time_lim(31 downto 24))		when "0011001",
		--std_logic_vector(time_lim(23 downto 16))		when "0011010",
		--std_logic_vector(time_lim(15 downto 08))		when "0011011",
		--std_logic_vector(time_lim(07 downto 00))		when "0011100",



		x"00" when others;

	-- Assert that there's always data for reading, and always room for writing
	--f2hValid_out <= '1';
	h2fReady_out <= '1';                                                     --END_SNIPPET(registers)

	-- LEDs and 7-seg display
	--led_out <= decbuff640(7 downto 0);
	--led_out <= load2;
	led_out <= final_disp;
	flags <= "00" & f2hReady_in & reset_in;
	seven_seg : entity work.seven_seg
		port map(
			clk_in     => clk_in,
			data_in    => checksum,
			dots_in    => flags,
			segs_out   => sseg_out,
			anodes_out => anode_out
		);
end architecture;
-- c289d158e1e9f0b8

