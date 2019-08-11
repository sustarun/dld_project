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

entity decrypter is
    Port ( clock : in  STD_LOGIC;
           K : in  STD_LOGIC_VECTOR (31 downto 0);
           C : in  STD_LOGIC_VECTOR (31 downto 0);
           P : out  STD_LOGIC_VECTOR (31 downto 0);
           reset : in  STD_LOGIC;
		   done: out std_logic:='0';
           enable : in  STD_LOGIC);
end decrypter;

architecture Behavioral of decrypter is
signal T:std_logic_vector (3 downto 0);
signal Concat_T:std_logic_vector (31 downto 0);
signal all_ors, last_digit:std_logic;
signal state_enc:STD_LOGIC:='0';
signal P_internal:STD_LOGIC_VECTOR (31 downto 0);
signal Bit32_register : STD_LOGIC_VECTOR (31 downto 0);
begin
	process(Bit32_register)
		begin
			last_digit <= Bit32_register(0);
			all_ors <= Bit32_register(31) or Bit32_register(30) or Bit32_register(29) or Bit32_register(28) or Bit32_register(27) or Bit32_register(26) or Bit32_register(25) or Bit32_register(24) or Bit32_register(23) or Bit32_register(22) or Bit32_register(21) or Bit32_register(20) or Bit32_register(19) or Bit32_register(18) or Bit32_register(17) or Bit32_register(16) or Bit32_register(15) or Bit32_register(14) or Bit32_register(13) or Bit32_register(12) or Bit32_register(11) or Bit32_register(10) or Bit32_register(9) or Bit32_register(8) or Bit32_register(7) or Bit32_register(6) or Bit32_register(5) or Bit32_register(4) or Bit32_register(3) or Bit32_register(2) or Bit32_register(1) or Bit32_register(0);
		end process;
	
	process(T)
		begin
			Concat_T <= T & T & T & T & T & T & T & T;
		end process;

	P <= P_internal;
   process(clock, reset, enable)
	 begin
		if (reset = '1') then
			P_internal <= "00000000000000000000000000000000";
			Bit32_register <= "00000000000000000000000000000000";
			done <='0';
			state_enc <= '0';
		elsif (clock'event and clock = '1') then
			if (enable = '1') then
				if(state_enc = '0') then
					P_internal <= C;
					T(0) <= K(0) xor K(4) xor K(8) xor K(12) xor K(16) xor K(20) xor K(24) xor K(28);
					T(1) <= K(1) xor K(5) xor K(9) xor K(13) xor K(17) xor K(21) xor K(25) xor K(29);
					T(2) <= K(2) xor K(6) xor K(10) xor K(14) xor K(18) xor K(22) xor K(26) xor K(30);
					T(3) <= K(3) xor K(7) xor K(11) xor K(15) xor K(19) xor K(23) xor K(27) xor K(31);
					Bit32_register <= K;
					state_enc <= '1';
				elsif(state_enc = '1') then
					Bit32_register <= '0' & Bit32_register(31 downto 1);
					if (all_ors = '1' and last_digit = '1') then
						P_internal<=P_internal xor Concat_T;
						T <= T + 1;
					elsif(all_ors ='0') then
						done <= '1';
					end if;
				end if;
			else
				state_enc <= '0';
				done <= '0';
			end if;
		end if;
	 end process;
end Behavioral;