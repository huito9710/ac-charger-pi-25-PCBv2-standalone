#!/bin/bash

# Check firmware-update script first
# Note: the firmware-update script, if found installation-package, should reboot after installation to avoid starting the ChargerV2 pre-maturely.
# do not use 'sudo' because /usr/local/Qt-xxx is created by 'pi'
~/Charger/evmega_firmware_update.py install

# Start ChargerV2
~/Desktop/ChargerV2


# Display Maintenance Screen after it dies/quits
# Start watchdog program also
feh -F -Y ~/Charger/JPG/MaintenanceMode.jpg & sh ~/Desktop/watchdog.sh
