#---
# Open Source Software Image Map(OSSIM) bash initialization script (sh).
# This file is installed by ossim rpm in: /etc/profile.d/ossim.sh
#---
if [ -z "$OSSIM_INSTALL_PREFIX" ]; then
   export OSSIM_INSTALL_PREFIX=/usr
fi

if [ -z "$OSSIM_PREFS_FILE" ]; then
   export OSSIM_PREFS_FILE=$OSSIM_INSTALL_PREFIX/share/ossim/ossim-site-preferences
fi

#---
# Stub to set path for non-standard install:
#---
# ossim_bin=/usr/local/ossim/bin
# if ! echo ${PATH} | /bin/grep -q $ossim_bin ; then
#    PATH=$ossim_bin:${PATH}
# fi
# export PATH
