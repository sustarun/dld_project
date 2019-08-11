----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    05:12:16 01/23/2018 
-- Design Name: 
-- Module Name:    encrypter - Behavioral 
-- Project Name: 
-- Target Devices: 
-- Tool versions: 
-- Description: 
--
-- Dependencies: 
--
-- Revision: 
-- Revision 0.01 - File Created
-- Additional Comments: 
--
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
use IEEE.NUMERIC_STD.ALL;

use IEEE.STD_LOGIC_UNSIGNED.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity sender is
   Port ( clock : in  STD_LOGIC;
   	read_sig: in STD_LOGIC;
   	inp_reg_once: in STD_LOGIC_VECTOR(31 downto 0); -- It will send from left to right
	reset : in  STD_LOGIC;
   	out_reg: out STD_LOGIC_VECTOR(7 downto 0);
   	out_sig: out STD_LOGIC;
	done: out std_logic;
	enable : in  STD_LOGIC;
	--corr_chan: in STD_LOGIC:= '0';
	chanAddr_in: in std_logic_vector(6 downto 0) ;
	i2: in STD_LOGIC_VECTOR(6 downto 0);
	deb_reg: out STD_LOGIC_VECTOR(7 downto 0));
end sender;

architecture Behavioral of sender is
signal state_enc:STD_LOGIC:='0';
signal inp_internal:STD_LOGIC_VECTOR (31 downto 0);
signal inp_reg: STD_LOGIC_VECTOR(31 downto 0);
signal state: unsigned(7 downto 0);
signal done_sig: std_logic:= '0';
--signal i2: unsigned(6 downto 0) := to_unsigned(4, 7);
--deb_reg(7) <= state_enc;
--signal Bit32_register : STD_LOGIC_VECTOR (31 downto 0);
begin
	--process(Bit32_register)
	--	begin
	--		last_digit <= Bit32_register(0);
	--		all_ors <= Bit32_register(31) or Bit32_register(30) or Bit32_register(29) or Bit32_register(28) or Bit32_register(27) or Bit32_register(26) or Bit32_register(25) or Bit32_register(24) or Bit32_register(23) or Bit32_register(22) or Bit32_register(21) or Bit32_register(20) or Bit32_register(19) or Bit32_register(18) or Bit32_register(17) or Bit32_register(16) or Bit32_register(15) or Bit32_register(14) or Bit32_register(13) or Bit32_register(12) or Bit32_register(11) or Bit32_register(10) or Bit32_register(9) or Bit32_register(8) or Bit32_register(7) or Bit32_register(6) or Bit32_register(5) or Bit32_register(4) or Bit32_register(3) or Bit32_register(2) or Bit32_register(1) or Bit32_register(0);
	--	end process;

	--process(T)
	--	begin
	--		Concat_T <= T & T & T & T & T & T & T & T;
	--	end process;

	--C <= C_internal;
	out_reg <= inp_reg(31 downto 24);
	deb_reg(7) <= done_sig;
	deb_reg(6 downto 0) <= std_logic_vector(state(6 downto 0));
	done <= done_sig;
	process(clock, reset, enable)
	begin
		if (reset = '1') then
			--out_reg <= "00000000";
			out_sig <= '0';
			state <= to_unsigned(0, 8);
			--C_internal <= "00000000000000000000000000000000";
			--Bit32_register <= "00000000000000000000000000000000";
			--inp_reg <= "00000000000000000000000000000000";
			done_sig <='0';
			state_enc <= '0';
			--i2 <= to_unsigned(4, 7);
		elsif (clock'event and clock = '1') then
			if (enable = '1') then
				if(state_enc = '0') then
					inp_reg <= inp_reg_once;
					--Bit32_register <= K;
					out_sig <= '1';
					state <= to_unsigned(1, 8);
					state_enc <= '1';
					--deb_reg <= (others => '0');
				elsif(state_enc = '1') then
					if (done_sig = '1') then
						state <= to_unsigned(1, 8);
						out_sig <= '0';
					elsif (read_sig = '1' and (chanAddr_in = i2)) then
						out_sig <= '0';
						--deb_sig <= '1';
						inp_reg <= inp_reg(23 downto 0) & "00000000";
						if (state < 5) then
							state <= state + to_unsigned(1, 8);
							--deb_reg <= "00000001";
						elsif (state = 5) then
							--state <= to_unsigned(6, 8);
							state <= to_unsigned(0, 8);
							done_sig <= '1';
							--out_sig <= '0';
							out_sig <= '1';
							--out_sig <= '0';
							--deb_reg <= "00000010";
						--elsif (state = 6) then
						--	state <= to_unsigned(0, 8);
						--	done_sig <= '0';
						end if;
					else
						--deb_sig <= '0';
						if (state = 0) then
							out_sig <= '0';
							--deb_reg <= "00000100";
						elsif (state = 5) then 
							state <= to_unsigned(0, 8);
							out_sig <= '1';
							--out_sig <= '0';
							done_sig <= '1';
						else
							out_sig <= '1';
							--deb_reg <= "00000101";
						end if;
					end if;
				end if;
			else
				state_enc <= '0';
				--deb_reg <= "00000110";
			end if;
		end if;
	end process;
end Behavioral;