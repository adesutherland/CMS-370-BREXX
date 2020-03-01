REM Copy and build source files to VM370
REM This is designed to allow code changes to be tested on
REM on the VM/370 relatively quickly
REM NOTE: It assumes that:
REM       container is IPLed
REM       Reader is started - start c
REM       CMSUSER is NOT logged on

REM Notwithstanding the above - IPL VM
herccontrol "ipl 141" -w "USER DSC LOGOFF AS AUTOLOG1"
herccontrol "/cp start c" -w "RDR"

REM Create the Source Archive Deck
yata -c -f tmp.txt
echo USERID  CMSUSER > yata.txt
echo :READ  YATA     TXT      >> yata.txt
type tmp.txt >> yata.txt

REM Send it to the docker container
docker cp yata.txt vm370:/opt/hercules/vm370/io
erase yata.txt
erase tmp.txt

REM Send it to CMSSUER via the card reader
docker exec vm370 bash -c "cd /opt/hercules/vm370/io && netcat -q 0 localhost 3505 < yata.txt"
docker exec vm370 bash -c "rm /opt/hercules/vm370/io/yata.txt"

REM LOGON CMSUSER
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon cmsuser cmsuser" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"

REM Read the file
herccontrol "/read *" -w "^Ready;"

REM Extract Build
herccontrol "/yata -x -d f" -w "^Ready;"
herccontrol "/erase yata txt a" -w "^Ready;"
herccontrol "/mkbrexx" -w "^Ready;" -t 120
herccontrol "/rename * * e = = e2" -w "^Ready;"

REM Sanity test
herccontrol "/copy * * e = = a ( replace" -w "^Ready;"
herccontrol "/copy sysprof exec s = = a" -w "^Ready;"
herccontrol "/ipl cms" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"
herccontrol "/rexxtry" -w "^Rexxtry;"
herccontrol "/parse version myver" -w "^Rexxtry;"
herccontrol "/say myver" -w "^Rexxtry;"
herccontrol "/exit" -w "^Ready;"

# LOGOFF
herccontrol "/logoff" -w "^VM/370 Online"
herccontrol "/logon operator operator" -w "RECONNECTED AT"
