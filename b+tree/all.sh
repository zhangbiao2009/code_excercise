#! /bin/sh
rm -f *.dot *.png 
./a.out <cmds.txt
./gen_pic.sh *.dot
sz *.png
