#!/bin/sh
# Make BREXX on CMS

# Exit if there is an error
set -e

# IPL
herccontrol -v
herccontrol "ipl 141" -w "USER DSC LOGOFF AS AUTOLOG1"
herccontrol "/cp start c" -w "RDR"
herccontrol "/cp start d class a" -w "PUN"

# LOGON CMSUSER
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon cmsuser cmsuser" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"

# Get Source
yata -c
herccontrol -m >tmp; read mark <tmp; rm tmp
echo "USERID  CMSUSER\n:READ  YATA     TXT     " > tmp
cat yata.txt >> tmp
netcat -q 0 localhost 3505 < tmp
rm tmp
herccontrol -w "HHCRD012I" -f $mark
herccontrol "/" -w "RDR FILE"
herccontrol "/yata -x -f READER -d f" -w "^Ready;"

# Get test suite
yata -c -d tests
herccontrol -m >tmp; read mark <tmp; rm tmp
echo "USERID  CMSUSER\n:READ  YATA     TXT     " > tmp
cat yata.txt >> tmp
netcat -q 0 localhost 3505 < tmp
rm tmp
herccontrol -w "HHCRD012I" -f $mark
herccontrol "/" -w "RDR FILE"
herccontrol "/read *" -w "^Ready;"
herccontrol "/yata -x -d f" -w "^Ready;"
herccontrol "/erase yata txt a" -w "^Ready;"

# Make source tape and vmarc
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon operator operator" -w "RECONNECTED AT"
hetinit -n -d brexxsrc.aws
herccontrol "devinit 480 io/brexxsrc.aws" -w "^HHCPN098I"
herccontrol "/attach 480 to cmsuser as 181" -w "TAPE 480 ATTACH TO CMSUSER"
herccontrol "devinit 00d io/brexxsrc.vmarc" -w "^HHCPN098I"
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon cmsuser cmsuser" -w "RECONNECTED AT"
herccontrol "/begin"
herccontrol "/COPYFILE * ASSEMBLE &DISK (RECFM F LRECL 80"
herccontrol "/tape dump * * f (noprint" -w "^Ready;"
herccontrol "/detach 181" -w "^Ready;"
herccontrol "/vmarc pack * * f (pun notrace" -w "^Ready;"

# Build
herccontrol "/ACC 193 E (ERASE" -w "^Ready;"
herccontrol "/mkbrexx DEBUG" -w "^Ready;" -t 350
herccontrol "/copy brexx text e brexxd = =" -w "^Ready;"
herccontrol "/copy brexx module e brexxd = =" -w "^Ready;"
herccontrol "/mkbrexx" -w "^Ready;" -t 350
herccontrol "/rename * * e = = e2" -w "^Ready;"

# Sanity test
herccontrol "/copy * * e = = a ( replace" -w "^Ready;"
herccontrol "/copy sysprof exec s = = a" -w "^Ready;"
herccontrol "/ipl cms" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"
herccontrol "/DMSREX" -w "^Ready;"
herccontrol "/rexxtry" -w "^Rexxtry;"
herccontrol "/parse version myver" -w "^Rexxtry;"
herccontrol "/say myver" -w "^Rexxtry;"
herccontrol "/exit" -w "^Ready;"

# Test suite
herccontrol "/runtest_" -w "^Ready;"

# Make binary tape and vmarc
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon operator operator" -w "RECONNECTED AT"
hetinit -n -d brexxbin.aws
herccontrol "devinit 480 io/brexxbin.aws" -w "^HHCPN098I"
herccontrol "/attach 480 to cmsuser as 181" -w "TAPE 480 ATTACH TO CMSUSER"
herccontrol "devinit 00d io/brexxbin.vmarc" -w "^HHCPN098I"
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon cmsuser cmsuser" -w "RECONNECTED AT"
herccontrol "/begin"
herccontrol "/tape dump * * e" -w "^Ready;"
herccontrol "/detach 181" -w "^Ready;"
herccontrol "/vmarc pack * * e (pun" -w "^Ready;"

# Clean Up
herccontrol "/erase brexx * a" -w "^Ready;"
herccontrol "/erase brexxd * a" -w "^Ready;"
herccontrol "/erase rexxtry exec a" -w "^Ready;"
herccontrol "/erase sysprof exec a" -w "^Ready;"

# LOGOFF
herccontrol "/logoff" -w "^VM/370 Online"

# SHUTDOWN
herccontrol "/logon operator operator" -w "RECONNECTED AT"
herccontrol "/shutdown" -w "^HHCCP011I"
