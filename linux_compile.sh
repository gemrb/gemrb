#!/bin/bash
#
# linux_compile version 0.1
# gemrb linux_compile by Zero_Dogg (webmaster@goldenfiles.com)
# Copyright (C) Zero_Dogg 2003
# Licensed under the GNU GPL.
#
if [ "$1" == "mklink" ]; then
	if [ "`whoami`" != "root" ]; then
		exit
	fi
	echo ""
	echo "Creating link..."
	ln -s $PWD/gemrb/gemrb /usr/local/bin/gemrb
	chmod +x /usr/local/bin/gemrb
	exit 0
fi
echo ""
echo "Preparing..."
fixold ()
{
	if [ -e $2 ]; then
		return
	else
		cp $1 $2
	fi
}
# Make sure the files are in place
fixold ./admin/old-ltcf-c.sh ./admin/ltcf-c.sh
fixold ./admin/old-ltcf-cxx.sh ./admin/ltcf-cxx.sh
fixold ./admin/old-ltcf-gcj.sh ./admin/ltcf-gcj.sh
fixold ./admin/old-ltconfig ./admin/ltconfig
fixold ./admin/old-ltmain.sh ./admin/ltmain.sh
fixold ./admin/old-libtool.m4.in ./admin/libtool.m4.in
chmod 744 ./*.sh
chmod 744 ./admin/*.sh
# Make sure we have ./configure
if [ -e ./configure ]; then
	var=0
else
	if [ `which autoconf`]; then
		echo ""
		echo "Could not find ./configure, running autoconf..."
		autoconf
	else
		echo ""
		echo "Could not find ./configure and could not find autoconf!"
		echo "Unable to continue!"
		exit
	fi
fi
echo ""
echo "Running configure script..."
./configure
if [ "$?" != "0" ]; then
	echo ""
	echo "Re-run $0 again when you have fixed the error from configure."
fi
echo ""
echo "Making core..."
cd gemrb/plugins/Core
make
if [ "$?" != "0" ]; then
	echo ""
	echo "An error occured during make of gemrb/plugins/Core."
	echo "Exiting..."
	exit
fi
echo ""
echo "Making gemrb"
cd ../../..
make
if [ "$?" != "0" ]; then
	echo ""
	echo "An error occured during make."
	echo "Exiting..."
	exit
fi
echo ""
echo "Preparing plugins......"
cd ./gemrb
# copy all plugins
cp -f ./plugins/*/.libs/lib*.so ./plugins/
# remove the cuckoo's egg
rm -f plugins/libcore.so
cd ..
# Check if the link exists, if not prompt the user for link creation
if [ -e /usr/local/bin/gemrb ]; then
	var=
else
	echo ""
	echo "Do you want to put GemRB into path so that you can simply run \"gemrb\""
	echo "to start it?"
	read -p "[Y/N] " -n 1 var
	echo ""
	if [ "$var" == "y" ] || [ "$var" == "Y" ]; then
		if [ "`whoami`" == "root" ]; then
			./$0 mklink
		else
			reqroot ()
			{
				echo ""
				echo "This requires you to be root!"
				echo "Please enter root's password!"
				su -c "./$0 mklink"
				if [ "$?" == "1" ]; then
					echo ""
					echo "Press "t" to try again"
					echo "Enter to skip."
					read -n 1 var
					echo ""
					if [ "$var" == "t" ] || [ "$var" == "T" ]; then
						reqroot
					else
						return
					fi
				fi
			}
			reqroot
		fi
	fi
fi
echo ""
# Display done messages :)
if [ -e /usr/local/bin/gemrb ]; then
	echo "Done!"
	echo "Type \"gemrb\" to start GemRB!"
else
	echo "Done!"
	echo "Type ./gemrb/gemrb to start GemRB!"
fi
