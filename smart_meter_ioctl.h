/*
 * smart_meter_ioctl.h
 * Shared header between kernel driver and user-space C++ program.
 *
 * It defines the "commands" that the user program can send to the driver
 * using the ioctl() system call, and the device file path.
 */

#ifndef SMART_METER_IOCTL_H
#define SMART_METER_IOCTL_H

/* The device file created by the driver. */
#define SMART_METER_DEV_PATH "/dev/smart_meter"

/*
 * ioctl command numbers are built from three parts:
 *   type  - one letter that identifies our driver ('S' for Smart meter)
 *   nr    - which command number (1, 2, 3 ...)
 *   size  - how many bytes of data is passed
 *
 * _IO  = command with NO data
 * _IOW = command where the user program WRITES data to the driver
 */
#define SM_IOCTL_MAGIC 'S'

/* Command 1: get the pulse count. The driver writes an unsigned long back. */
#define SM_IOCTL_GET_PULSES  _IOW(SM_IOCTL_MAGIC, 1, unsigned long)

/* Command 2: reset the pulse count back to 0. No data. */
#define SM_IOCTL_RESET       _IO(SM_IOCTL_MAGIC, 2)

/* Command 3: add a single pulse. No data. */
#define SM_IOCTL_ADD_PULSE  _IO(SM_IOCTL_MAGIC, 3)

#endif /* SMART_METER_IOCTL_H */