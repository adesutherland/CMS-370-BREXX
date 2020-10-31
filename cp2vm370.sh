#!/bin/sh
# Copy and build source files to VM370
# This tests the cmsbuild.sh and cmsinstall.sh scripts
# that are used by the automated build process

docker kill vm370
docker pull adriansutherland/vm370:builder
docker run --rm -d -p 3270:3270 -p 8038:8038 -p 3505:3505 --name vm370 adriansutherland/vm370:builder
# docker run --rm -d -p 3270:3270 -p 8038:8038 -p 3505:3505 --name vm370 adriansutherland/vm370local:latest

yata -c -f tmp.txt
echo "USERID  CMSUSER\n:READ  YATA     TXT     " > yata.txt
cat tmp.txt >> yata.txt
docker cp yata.txt vm370:/opt/hercules/vm370/io
rm yata.txt
rm tmp.txt
docker exec vm370 bash -c "cd /opt/hercules/vm370/io && yata -x"
docker exec vm370 bash -c "rm /opt/hercules/vm370/io/yata.txt"

# Test Suite
yata -c -d tests -f tmp.txt
echo "USERID  CMSUSER\n:READ  YATA     TXT     " > yata.txt
cat tmp.txt >> yata.txt
docker cp yata.txt vm370:/opt/hercules/vm370/io
docker exec vm370 bash -c "mkdir /opt/hercules/vm370/io/tests"
docker cp yata.txt vm370:/opt/hercules/vm370/io/tests
rm yata.txt
rm tmp.txt
docker exec vm370 bash -c "cd /opt/hercules/vm370/io/tests && yata -x"
docker exec vm370 bash -c "rm /opt/hercules/vm370/io/tests/yata.txt"

# Build
#docker cp cmsbuild.sh vm370:/opt/hercules/vm370/io
#docker exec vm370 bash -c "cd /opt/hercules/vm370/io && chmod +x cmsbuild.sh && ./cmsbuild.sh"
docker cp cmsdebug.sh vm370:/opt/hercules/vm370/io
docker exec vm370 bash -c "cd /opt/hercules/vm370/io && chmod +x cmsdebug.sh && ./cmsdebug.sh"

# Install
#docker cp cmsinstall.sh vm370:/opt/hercules/vm370/io
#docker exec vm370 bash -c "cd /opt/hercules/vm370/io && chmod +x cmsinstall.sh && ./cmsinstall.sh"
