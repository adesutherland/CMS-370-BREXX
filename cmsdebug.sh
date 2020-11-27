#!/bin/sh
# Make BREXX on CMS (Debug only)

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

# Build
herccontrol "/ACC 193 E (ERASE" -w "^Ready;"
herccontrol "/mkbrexx DEBUG" -w "^Ready;" -t 350
herccontrol "/rename * * e = = e2" -w "^Ready;"

# Sanity test
herccontrol "/copy * * e = = a ( replace" -w "^Ready;"
herccontrol "/copy sysprof exec s = = a" -w "^Ready;"
herccontrol "/ipl cms" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"
herccontrol "/rexxtry" -w "^Rexxtry;"
herccontrol "/parse version myver" -w "^Rexxtry;"
herccontrol "/say myver" -w "^Rexxtry;"
herccontrol "/exit" -w "^Ready;"

# Test suite
herccontrol "/runtest_" -w "^Ready;"

# LOGOFF
herccontrol "/logoff" -w "^VM/370 Online"

# SHUTDOWN
herccontrol "/logon operator operator" -w "RECONNECTED AT"
herccontrol "/shutdown" -w "^HHCCP011I"
