savedcmd_smart_meter.mod := printf '%s\n'   smart_meter_driver.o | awk '!x[$$0]++ { print("./"$$0) }' > smart_meter.mod
