#!/bin/sh
# Install BREXX on CMS

# Exit if there is an error
set -e

# IPL
herccontrol -v
herccontrol "ipl 141" -w "USER DSC LOGOFF AS AUTOLOG1"

# LOGON MAINT AND READ TAPE
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon maint cpcms" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"
herccontrol "devinit 480 io/brexxbin.aws" -w "^HHCPN098I"
herccontrol "/attach 480 to maint as 181" -w "TAPE 480 ATTACH"

# MAINT 19D DISK
herccontrol "/access 19D z" -w "^Ready;"
herccontrol "/tape load * helpcmd z" -w "^Ready;"
herccontrol "/tape rew" -w "^Ready;"
herccontrol "/tape load * helpcmd2 z" -w "^Ready;"
herccontrol "/tape rew" -w "^Ready;"
herccontrol "/release z"  -w "^Ready;"

# MAINT 19E DISK
herccontrol "/access 19E z" -w "^Ready;"
herccontrol "/tape load rexxtry exec z" -w "^Ready;"
herccontrol "/tape rew" -w "^Ready;"
herccontrol "/release z"  -w "^Ready;"

# MAINT 190 CMS SYSTEM DISK
herccontrol "/access 190 z" -w "^Ready;"
herccontrol "/tape load brexx module z" -w "^Ready;"
herccontrol "/tape rew" -w "^Ready;"
herccontrol "/tape load brexx text z" -w "^Ready;"
herccontrol "/tape rew" -w "^Ready;"
herccontrol "/release z"  -w "^Ready;"

# Done with tape
herccontrol "/detach 181" -w "^Ready;"

# Regenerate System
herccontrol "/define storage 16m"  -w "^CP ENTERED"
herccontrol "/ipl 190" -w "^CMS VERSION"
herccontrol "/access ( noprof" -w "^Ready;"
herccontrol "/access 093 b" -w "^Ready;"
herccontrol "/access 193 c" -w "^Ready;"
herccontrol "/cmsxgen f00000 cmsseg" -w "^Ready;"
herccontrol "/ipl 190" -w "^CMS VERSION"
herccontrol "/savesys cms" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"
herccontrol "/logoff" -w "^VM/370 Online"

# Load source
herccontrol "/logon maint cpcms" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"
herccontrol "devinit 480 io/brexxsrc.aws" -w "^HHCPN098I"
herccontrol "/attach 480 to maint as 181" -w "TAPE 480 ATTACH"
herccontrol "/link brexx 191 291 mr mult" -w "^Ready;"
herccontrol "/access 291 g (erase" -w "^Ready;"
herccontrol "/tape load * * g" -w "^Ready;"
herccontrol "/detach 291" -w "^Ready;"
herccontrol "/detach 181" -w "^Ready;"
herccontrol "/logoff" -w "^VM/370 Online"

# LOGON CMSUSER
herccontrol "/logon cmsuser cmsuser" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"

# Sanity test
herccontrol "/rexxtry" -w "^Rexxtry;"
herccontrol "/parse version myver" -w "^Rexxtry;"
herccontrol "/say myver" -w "^Rexxtry;"
herccontrol "/exit" -w "^Ready;"

# LOGOFF
herccontrol "/logoff" -w "^VM/370 Online"

# SHUTDOWN
herccontrol "/logon operator operator" -w "RECONNECTED AT"
herccontrol "/shutdown" -w "^HHCCP011I"
