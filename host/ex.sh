clear
make clean
make deps
cd ../../
# export PATH=$PATH:~/XILINIX/14.7/ISE_DS/ISE/bin/lin64/
# Fetch and build the VHDL version of the cksum example:
# scripts/msget.sh makestuff/hdlmake
cd hdlmake/apps
# ../bin/hdlmake.py -g makestuff/swled
cd makestuff/swled/cksum/vhdl
if [ $# -gt 1 ] 
then
	echo 'Compiling FPGA';
	../../../../../bin/hdlmake.py -t ../../templates/fx2all/vhdl -b atlys -p fpga ;
	ls ;
	sl;
	echo 'Done Compiling FPGA'
fi

# Load FPGALink firmware into the Atlys:


sudo ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -i 1443:0007
# sudo ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -p J:D0D2D3D4:fpga.xsvf
if [ $1 -gt 9 ]
then
	sudo ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -p J:D0D2D3D4:fpga.xsvf -b -s
else
	sudo ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -p J:D0D2D3D4:fpga.xsvf -b -e
fi