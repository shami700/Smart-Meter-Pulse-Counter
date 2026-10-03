# Smart Energy – Smart Meter Pulse Counter & Analytics Agent

A small beginner-friendly Linux capstone project written in **C** and **C++**.

It simulates an electricity meter. Every "pulse" (meter click) is counted by a
Linux kernel driver, and a C++ program turns that count into energy and cost.

---

## 1. Project Objective

Real smart meters send a small electrical pulse for every fixed amount of energy
used. This project:

1. Simulates those pulses (no real hardware needed).
2. Counts them inside a Linux kernel driver.
3. Reads the count from a simple C++ menu program.
4. Calculates energy in kWh and estimated cost in Rupees.
5. Saves the result in a plain text report.

---

## 2. How the System Works

```
  You type a menu choice
            |
            v
  smart_energy_agent.cpp      (user space / normal program)
            |
            |  write()   -> "simulate 500 pulses"
            |  ioctl()   -> "what is the count?"  /  "reset it"
            v
  /dev/smart_meter            (the device file)
            |
            v
  smart_meter_driver.c        (kernel space / driver)
            |
            v
  pulse_counter  ->  Energy (kWh)  ->  Cost (Rs)
```

Key idea in one line: **the driver owns the counter, the C++ program owns the maths.**

---

## 3. Folder Structure

```
SmartEnergy/
│
├── smart_meter_ioctl.h     Shared header: device path + ioctl commands
├── smart_meter_driver.c    Linux kernel character device driver
├── smart_energy_agent.cpp  User-space C++ menu program
├── Makefile                Builds the driver and the C++ program
├── README.md               This file
└── reports/
    └── energy_report.txt   Simple text report output
```

---

## 4. Linux Driver Concept (in simple words)

A **kernel module / device driver** is a piece of code that talks to hardware.
It runs inside the kernel (very privileged), so it must be careful.

What our driver does:

| Step | Function | Meaning |
|------|----------|---------|
| 1 | `misc_register()` | Registers the device so Linux creates `/dev/smart_meter` |
| 2 | `smart_meter_open()` | Called when a user program opens the device |
| 3 | `smart_meter_read()` | Sends the pulse count back to the user program |
| 4 | `smart_meter_write()` | A pulse arrives → counter increases |
| 5 | `smart_meter_ioctl()` | Special commands: get count, reset, add one pulse |
| 6 | `smart_meter_release()` | Called when the device is closed |
| 7 | `module_init()` / `module_exit()` | Run on `insmod` and `rmmod` |

**Why a mutex?**
If two programs used the device at the same time, one might read the counter
while the other is changing it. `DEFINE_MUTEX()` + `mutex_lock()` /
`mutex_unlock()` makes sure only one program uses the counter at a time, so no
data is lost.

**Why `printk`?**
`pr_info()` / `pr_err()` print messages from the kernel into the kernel log,
which we read using `dmesg`. This is how we debug a driver.

**Why `copy_to_user` / `copy_from_user`?**
Kernel memory and user memory are two separate, protected areas. You are not
allowed to touch user memory directly from a driver — you must use these two
functions to copy data safely.

---

## 5. How Pulse Counting Works

1. In the driver there is one variable: `pulse_counter`, starting at 0.
2. The user program writes a number (for example `500`) to the device.
3. The driver's `smart_meter_write()` receives that number.
4. It does `pulse_counter = pulse_counter + 500;` while holding the lock.
5. It prints the new total with `pr_info()`.
6. When the user program asks for the count, `smart_meter_ioctl()` copies
   `pulse_counter` into the user program's variable.

The counter stays in **kernel memory**, so it survives as long as the driver is
loaded and it is shared correctly by every program that opens the device.

---

## 6. Energy Calculation

Assumption given in the project:

```
1000 pulses = 1 kWh
```

So:

```
Energy (kWh) = total pulses / 1000
```

**Example:** 2500 pulses → 2500 / 1000 = 2.5 kWh

This is written as one short line in the C++ program:

```cpp
double pulses_to_energy(unsigned long pulses)
{
    return pulses / PULSES_PER_KWH;    // PULSES_PER_KWH = 1000.0
}
```

---

## 7. Cost Calculation

Assumption given in the project:

```
Rate = Rs 8 per kWh
```

So:

```
Cost (Rs) = Energy (kWh) * 8
```

**Example:** 2.5 kWh → 2.5 × 8 = Rs 20

The rate is a single constant at the top of the C++ file, so you can change it
to Rs 7 or Rs 12 by editing one line.

---

## 8. Files and What Each One Does

### `smart_meter_ioctl.h`
The **shared header**. Both the driver (C) and the program (C++) include it, so
both agree on the device name and the command numbers.

- `SMART_METER_DEV_PATH` = `"/dev/smart_meter"`
- `SM_IOCTL_GET_PULSES` — ask for the count
- `SM_IOCTL_RESET` — clear the count
- `SM_IOCTL_ADD_PULSE` — add one pulse

### `smart_meter_driver.c`
The **kernel driver**. Owns the pulse counter, creates `/dev/smart_meter`, and
handles `read`, `write` and `ioctl`.

### `smart_energy_agent.cpp`
The **user program**. Shows the 7-option menu, sends pulses, reads the count,
does the energy/cost maths and writes the report file.

### `Makefile`
Has four jobs:

- `make driver` – compile the kernel module into `smart_meter.ko`
- `make app` – compile the C++ program into `smart_energy_agent`
- `make` – do both
- `make clean` – delete all generated files

### `reports/energy_report.txt`
The **output file** created by menu option 6. Plain text, easy to read.

---

## 9. How to Compile

Open a terminal inside the `SmartEnergy` folder.

```bash
# Install the tools you need (only once)
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)

# Build both the driver and the C++ program
make
```

Or build them separately:

```bash
make driver     # creates smart_meter.ko
make app        # creates smart_energy_agent
```

You can also compile the C++ program by hand:

```bash
g++ -std=c++17 smart_energy_agent.cpp -o smart_energy_agent
```

### IMPORTANT: no spaces in the path

The Linux kernel build system **cannot handle spaces** in folder names. If your
path looks like `My Projects/SmartEnergy`, the driver build fails with
`No rule to make target`. Copy the `SmartEnergy` folder to a simple path first:

```bash
cp -r ~/Desktop/SmartEnergy ~/SmartEnergy
cd ~/SmartEnergy
```

Only the **driver** is affected. The C++ program builds fine either way.

---

## 10. How to Run

```bash
# 1. Load the driver (this creates /dev/smart_meter)
sudo insmod smart_meter.ko

# 2. Check the driver is loaded
lsmod | grep smart_meter

# 3. Check the device file exists
ls -l /dev/smart_meter

# 4. Run the user-space program
./smart_energy_agent
```

The menu looks like this:

```
========== SMART ENERGY AGENT ==========
1. Generate / Simulate Meter Pulse
2. Show Total Pulses
3. Show Energy Consumption
4. Show Estimated Electricity Cost
5. Reset Meter
6. Save Simple Report
7. Exit
=========================================
```

---

## 11. How to Test

```bash
# Test 1: generate pulses from the program
./smart_energy_agent
#  choose 1, enter 1000

# Test 2: read the counter directly from the shell
cat /dev/smart_meter

# Test 3: send pulses from the shell instead of the program
echo 250 > /dev/smart_meter
cat /dev/smart_meter

# Test 4: reset from the shell using ioctl (needs a small C helper)
#         easier: just use menu option 5 in the program

# Test 5: read kernel logs
dmesg | tail -20

# Test 6: check the report file
cat reports/energy_report.txt
```

Expected log lines after pulses:

```
smart_meter: device opened
smart_meter: pulse received, total pulses = 1000
smart_meter: ioctl GET_PULSES = 1000
```

---

## 12. How to Stop and Remove the Driver

```bash
# Close the program first (choose option 7 in the menu)

# Remove the driver
sudo rmmod smart_meter

# Confirm it is gone
lsmod | grep smart_meter      # should print nothing

# Remove the device file check
ls -l /dev/smart_meter        # should say "No such file or directory"
```

The device file is created automatically when the driver loads and disappears
automatically when it is removed. You never create or delete it by hand.

---

## 13. Cleaning the Project

```bash
make clean
```

This removes `smart_meter.ko`, the object files, and the `smart_energy_agent`
binary. Your `.c`, `.cpp`, `.h`, `Makefile` and `reports/` folder stay safe.

---

## 14. Viva Questions and Simple Answers

**Q. What does this project do?**
It counts simulated electricity meter pulses in a Linux kernel driver and
converts them into energy (kWh) and estimated cost.

**Q. Why did you write a driver instead of only a C++ program?**
To show how user space and kernel space communicate through a device file, and
to keep the counter safely inside kernel memory.

**Q. What is a pulse?**
One click of the meter. 1000 pulses is treated as 1 kWh.

**Q. How does the C++ program talk to the driver?**
Through `/dev/smart_meter` using `open`, `write`, `read` and `ioctl`.

**Q. What is ioctl and why use it?**
`ioctl` (I/O control) sends a special command number to the driver. It is used
for actions that are not simple reads or writes, like *reset the counter*.

**Q. What does the mutex do?**
It stops two programs from changing the counter at the same moment, so no
pulses are lost.

**Q. What is `copy_to_user` / `copy_from_user`?**
They safely copy data between kernel memory and user memory. A driver must
never access user memory directly.

**Q. What happens on `insmod` and `rmmod`?**
`insmod` runs `smart_meter_init()` which registers the device and prints a
message. `rmmod` runs `smart_meter_exit()` which removes the device and prints
the final count.

**Q. What is `dmesg` for?**
It shows the kernel log, where all `pr_info()` and `pr_err()` messages appear.

---

## 15. Limits of This Project

- Pulses are simulated, there is no real hardware.
- The counter is lost when the driver is removed (it is in RAM).
- Rate is fixed in the source code.
- No database, no GUI, no web, no networking, no cloud.

These are intentional — the aim is to demonstrate the **basic idea clearly**.