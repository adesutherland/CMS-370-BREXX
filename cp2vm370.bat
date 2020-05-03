REM Copy and build source files to VM370
REM This tests the cmsbuild.sh and cmsinstall.sh scripts
REM that are used by the automated build process
yata -c -f tmp.txt
echo USERID  CMSUSER > yata.txt
echo :READ  YATA     TXT      >> yata.txt
type tmp.txt >> yata.txt
docker cp yata.txt vm370:/opt/hercules/vm370/io
erase yata.txt
erase tmp.txt
docker exec vm370 bash -c "cd /opt/hercules/vm370/io && yata -x"
docker exec vm370 bash -c "rm /opt/hercules/vm370/io/yata.txt"

REM Test Suite
yata -c -d tests -f tmp.txt
echo USERID  CMSUSER > yata.txt
echo :READ  YATA     TXT      >> yata.txt
type tmp.txt >> yata.txt
docker exec vm370 bash -c "mkdir /opt/hercules/vm370/io/tests"
docker cp yata.txt vm370:/opt/hercules/vm370/io/tests
erase yata.txt
erase tmp.txt
docker exec vm370 bash -c "cd /opt/hercules/vm370/io/tests && yata -x"
docker exec vm370 bash -c "rm /opt/hercules/vm370/io/tests/yata.txt"

REM Build
docker cp cmsbuild.sh vm370:/opt/hercules/vm370/io
docker exec vm370 bash -c "cd /opt/hercules/vm370/io && ./cmsbuild.sh"

REM Install
docker cp cmsinstall.sh vm370:/opt/hercules/vm370/io
docker exec vm370 bash -c "cd /opt/hercules/vm370/io && ./cmsinstall.sh"
