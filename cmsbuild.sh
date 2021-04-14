#!/bin/sh
# Make BREXX on CMS

# Exit if there is an error
set -e

# Get the latest gccbrx.cckd disk image
herccontrol "detach 09F0"
wget -nv https://github.com/adesutherland/CMS-370-GCCLIB/releases/download/v1.0.0/GCCLIB.zip
unzip GCCLIB.zip
cp GCCLIB/gccbrx.cckd ..
rm GCCLIB.zip
rm -r GCCLIB
herccontrol "attach 09F0 3350 gccbrx.cckd"

# IPL
herccontrol "ipl 141" -w "USER DSC LOGOFF AS AUTOLOG1"
herccontrol "/cp start c" -w "RDR"
herccontrol "/cp start d class a" -w "PUN"

# LOGON MAINTC
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon maintc maintc" -w "^CMS"
herccontrol "/" -w "^Ready;"
herccontrol "/purge rdr" -w "^Ready;"

herccontrol "/ACCESS 393 B" -w "^Ready;"
herccontrol "/ERASE * * B1" -w "^Ready;"

# Get Source
yata -c -f archive.yata
herccontrol -m >tmp; read mark <tmp; rm tmp
echo "USERID MAINTC\n:READ  ARCHIVE  YATA    " > tmp
cat archive.yata >> tmp
netcat -q 0 localhost 3505 < tmp
rm tmp
herccontrol -w "HHCRD012I" -f $mark
herccontrol "/" -w "RDR FILE"
herccontrol "/yata -x -f READER -d b" -w "^Ready;"

# Get test suite
yata -c -d tests -f archive.yata
herccontrol -m >tmp; read mark <tmp; rm tmp
echo "USERID  MAINTC\n:READ  ARCHIVE  YATA    " > tmp
cat archive.yata >> tmp
netcat -q 0 localhost 3505 < tmp
rm tmp
herccontrol -w "HHCRD012I" -f $mark
herccontrol "/" -w "RDR FILE"
herccontrol "/yata -x -f READER -d b" -w "^Ready;"

# Get test tools
yata -c -d tools -f archive.yata
herccontrol -m >tmp; read mark <tmp; rm tmp
echo "USERID  MAINTC\n:READ  ARCHIVE  YATA    " > tmp
cat archive.yata >> tmp
netcat -q 0 localhost 3505 < tmp
rm tmp
herccontrol -w "HHCRD012I" -f $mark
herccontrol "/" -w "RDR FILE"
herccontrol "/yata -x -f READER -d b" -w "^Ready;"

# Fix Source Files
herccontrol "/COPYFILE * MACRO    B (RECFM F LRECL 80" -w "^Ready"
herccontrol "/COPYFILE * COPY     B (RECFM F LRECL 80" -w "^Ready"
herccontrol "/COPYFILE * ASSEMBLE B (RECFM F LRECL 80" -w "^Ready"
#herccontrol "/COPYFILE * EXEC     B (RECFM F" -w "^Ready"
herccontrol "/RENAME NEWBREXX TMPFTYPE B NEWBREXX CONTROL B" -w "^Ready"

# Make source tape and vmarc
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon operator operator" -w "RECONNECTED AT"
hetinit -n -d brexxsrc.aws
herccontrol "devinit 480 io/brexxsrc.aws" -w "^HHCPN098I"
herccontrol "/attach 480 to maintc as 181" -w "TAPE 480 ATTACH"
herccontrol "devinit 00d io/brexxsrc.vmarc" -w "^HHCPN098I"
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon maintc maintc" -w "RECONNECTED"
herccontrol "/begin"
herccontrol "/tape dump * * b (noprint" -w "^Ready;"
herccontrol "/detach 181" -w "^Ready;"
herccontrol "/vmarc pack * * b (pun notrace" -w "^Ready;"

# Close and remove extra record from VMARC file
herccontrol "devinit 00d dummy" -w "^HHCPN098I"
truncate -s-80 brexxsrc.vmarc

# Put tools in the T drive
herccontrol "/COPYFILE BRXASM EXEC B = = T (REPLACE" -w "^Ready"
herccontrol "/ERASE BRXASM EXEC B" -w "^Ready"

herccontrol "/COPYFILE BRXBUILD EXEC B = = T (REPLACE" -w "^Ready"
herccontrol "/ERASE BRXBUILD EXEC B" -w "^Ready"

herccontrol "/COPYFILE BRXCOMP EXEC B = = T (REPLACE" -w "^Ready"
herccontrol "/ERASE BRXCOMP EXEC B" -w "^Ready"

herccontrol "/COPYFILE BRXCOMPD EXEC B = = T (REPLACE" -w "^Ready"
herccontrol "/ERASE BRXCOMPD EXEC B" -w "^Ready"

herccontrol "/COPYFILE BRXGEN EXEC B = = T (REPLACE" -w "^Ready"
herccontrol "/ERASE BRXGEN EXEC B" -w "^Ready"

herccontrol "/COPYFILE BRXSRCH EXEC B = = T (REPLACE" -w "^Ready"
herccontrol "/ERASE BRXSRCH EXEC B" -w "^Ready"

herccontrol "/COPYFILE BRXSRCHD EXEC B = = T (REPLACE" -w "^Ready"
herccontrol "/ERASE BRXSRCHD EXEC B" -w "^Ready"

herccontrol "/COPYFILE BRXSRCHT EXEC B = = T (REPLACE" -w "^Ready"
herccontrol "/ERASE BRXSRCHT EXEC B" -w "^Ready"

herccontrol "/COPYFILE TOOLDISK MEMO B = = T (REPLACE" -w "^Ready"
herccontrol "/ERASE TOOLDISK MEMO B" -w "^Ready"

herccontrol "/COPYFILE NEWBREXX CONTROL B = = A (REPLACE" -w "^Ready"
herccontrol "/ERASE NEWBREXX CONTROL B" -w "^Ready"

herccontrol "/ipl cms" -w "^CMS"
herccontrol "/" -w "^Ready;"

herccontrol "/BRXBUILD" -w "^Ready;" -t 240
herccontrol "/ipl cms" -w "^CMS"
herccontrol "/" -w "^Ready;"

herccontrol "/BRXSRCH" -w "^Ready;"
herccontrol "/BRXGEN" -w "^Ready;"
herccontrol "/ipl cms" -w "^CMS"
herccontrol "/" -w "^Ready;"

# Make binary tape and vmarc
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon operator operator" -w "RECONNECTED AT"
hetinit -n -d brexxbin.aws
herccontrol "devinit 480 io/brexxbin.aws" -w "^HHCPN098I"
herccontrol "/attach 480 to maintc as 181" -w "TAPE 480 ATTACH"
herccontrol "devinit 00d io/brexxbin.vmarc" -w "^HHCPN098I"
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon maintc maintc" -w "RECONNECTED AT"
herccontrol "/begin"
herccontrol "/access 193 e" -w "^Ready"
herccontrol "/copyfile brexx * a = = e" -w "^Ready"
herccontrol "/copyfile brexxd * a = = e" -w "^Ready"
herccontrol "/tape dump * * e" -w "^Ready"
herccontrol "/detach 181" -w "^Ready;"
herccontrol "/vmarc pack * * e (pun" -w "^Ready;"

# Close and remove extra record from VMARC file
herccontrol "devinit 00d dummy" -w "^HHCPN098I"
truncate -s-80 brexxbin.vmarc

# LOGOFF
herccontrol "/logoff" -w "^VM/370 Online"

# REBUILD CMS
herccontrol "/logon maint cpcms" -w "^CMS"
herccontrol "/" -w "^Ready"
herccontrol "/NEWBREXX" -w "^Ready"
herccontrol "/define storage 16m"  -w "CP ENTERED"
herccontrol "/ipl 190 clear" -w "^CMS"
herccontrol "/savesys cms" -w "^CMS"
herccontrol "/" -w "^Ready;"
herccontrol "/logoff" -w "^VM/370 Online"

# Test suite
# herccontrol "/runtest_" -w "^Ready;"

# SHUTDOWN
herccontrol "/logon operator operator" -w "RECONNECTED AT"
herccontrol "/shutdown" -w "^HHCCP011I"
herccontrol "detach 09F0"
