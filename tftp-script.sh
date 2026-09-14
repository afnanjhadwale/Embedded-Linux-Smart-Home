#!/bin/bash
#===============================================================================
#  tftp-script.sh
#  Configures the TFTP server (and NFS server for network booting) on the
#  host PC for the Rugged Board A5D2X workshop.
#
#  Reference: developer.ruggedboard.com
#             g5-system-development-guide -> flash-the-nor-using-tftp
#
#  Usage:   chmod 777 tftp-script.sh
#           sudo su
#           sh tftp-script.sh
#===============================================================================

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'; NC='\033[0m'

#--- must run as root ----------------------------------------------------------
if [ "$(id -u)" -ne 0 ]; then
    echo -e "${RED}Please run as root:   sudo su   then   sh tftp-script.sh${NC}"
    exit 1
fi

echo -e "${YELLOW}==============================================================${NC}"
echo -e "${YELLOW}  Rugged Board A5D2X - TFTP / NFS Server Setup${NC}"
echo -e "${YELLOW}==============================================================${NC}"

#--- 0. Remove any existing/conflicting xinetd + tftpd installation ------------
echo -e "\n${GREEN}[1/6] Removing existing xinetd / tftpd (clean slate) ...${NC}"
apt-get remove --purge -y xinetd tftpd  >/dev/null 2>&1
apt-get remove --purge -y xinetd tftp   >/dev/null 2>&1

#--- 1. Install TFTP + NFS packages --------------------------------------------
echo -e "\n${GREEN}[2/6] Installing xinetd, tftpd, tftp, nfs-kernel-server ...${NC}"
apt-get update
apt-get -y install xinetd            || { echo -e "${RED}xinetd install failed${NC}"; exit 1; }
apt-get -y install nfs-kernel-server || { echo -e "${RED}nfs-kernel-server install failed${NC}"; exit 1; }
apt-get -y install tftpd             || { echo -e "${RED}tftpd install failed${NC}"; exit 1; }
apt-get -y install tftp              || { echo -e "${RED}tftp install failed${NC}"; exit 1; }

#--- 2. Create the TFTP root directory -----------------------------------------
echo -e "\n${GREEN}[3/6] Creating /var/lib/tftpboot ...${NC}"
mkdir -p /var/lib/tftpboot
chmod 777 /var/lib/tftpboot

#--- 3. Write the xinetd TFTP service configuration ----------------------------
echo -e "\n${GREEN}[4/6] Writing /etc/xinetd.d/tftp ...${NC}"
cat > /etc/xinetd.d/tftp <<'EOF'
service tftp
{
protocol        = udp
port            = 69
socket_type     = dgram
wait            = yes
user            = nobody
server          = /usr/sbin/in.tftpd
server_args     = /var/lib/tftpboot -s
disable         = no
}
EOF

#--- 4. Restart the TFTP (xinetd) service --------------------------------------
echo -e "\n${GREEN}[5/6] Restarting xinetd (TFTP server) ...${NC}"
service xinetd restart || systemctl restart xinetd

#--- 5. Configure NFS for network (NFS) booting --------------------------------
echo -e "\n${GREEN}[6/6] Configuring NFS export /nfsroot ...${NC}"
mkdir -p /nfsroot
chmod 777 /nfsroot
# add the export line only once
if ! grep -q "^/nfsroot" /etc/exports 2>/dev/null; then
    echo "/nfsroot *(rw,sync,no_subtree_check,no_root_squash)" >> /etc/exports
fi
exportfs -ra
/etc/init.d/nfs-kernel-server restart 2>/dev/null || systemctl restart nfs-kernel-server

#--- Verification ----------------------------------------------------------------
echo -e "\n${YELLOW}==============================================================${NC}"
OK=1
if [ -d /var/lib/tftpboot ]; then
    echo -e "  ${GREEN}[ok]${NC} /var/lib/tftpboot created"
else
    echo -e "  ${RED}[FAIL]${NC} /var/lib/tftpboot missing"; OK=0
fi
if pgrep xinetd >/dev/null; then
    echo -e "  ${GREEN}[ok]${NC} xinetd (TFTP) running on UDP port 69"
else
    echo -e "  ${RED}[FAIL]${NC} xinetd not running"; OK=0
fi
if showmount -e localhost 2>/dev/null | grep -q "/nfsroot"; then
    echo -e "  ${GREEN}[ok]${NC} NFS export /nfsroot active"
else
    echo -e "  ${YELLOW}[warn]${NC} NFS export not visible (needed only for network boot)"
fi

if [ $OK -eq 1 ]; then
    echo -e "\n${GREEN}  TFTP server ready.${NC}"
    echo -e "  Test it (Lab 0-C):"
    echo -e "    PC   :  touch test.txt && cp test.txt /var/lib/tftpboot"
    echo -e "    Board:  tftp -r test.txt -g <PC_IP>      (e.g. 192.168.1.50)"
    echo -e "\n  For network (NFS) booting, additionally:"
    echo -e "    cp zImage a5d2x-rugged_board.dtb /var/lib/tftpboot"
    echo -e "    tar -xvf rb-sd-core-image-minimal-rugged-board-a5d2x-sd1.tar.gz -C /nfsroot"
else
    echo -e "\n${RED}  Setup completed with errors - see FAIL lines above.${NC}"
fi
echo -e "${YELLOW}==============================================================${NC}"
