echo ; echo "Welcome to Rugged Board";
echo ; echo "eLinux Package is Installing";
echo ; echo "=============================";
echo ; echo "Updating OS";
echo ; echo "=============================";
sudo apt-get update
echo ; echo "Updating OS Completed";
echo ; echo "=============================";
echo ; echo "Installing Additional Components";
echo ; echo "=============================";
sudo apt-get -y install git-core flex bison gperf libesd0-dev zip libwxgtk2.6-dev zlib1g-dev build-essential gettext texinfo
sudo apt-get -y install  fakeroot gnupg libsdl1.2-dev squashfs-tools uboot-mkimage expect libncurses5-dev
sudo apt-get -y install minicom lrzsz tftpd-hpa nfs-kernel-server nfs-common portmap patch vim gawk
sudo apt-get -y install xinetd tftpd tftp qtcreator ctags quilt 
echo ; echo "your setup is completed ";
echo ; echo "=============================";
echo; echo "all the best";
