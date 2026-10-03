/*
 * smart_energy_agent.cpp
 * User-space C++ program for the Smart Energy capstone project.
 *
 * What this program does:
 *   1. Opens the device file /dev/smart_meter
 *   2. Shows a simple text menu
 *   3. Generates simulated pulses (one pulse = one meter click)
 *   4. Reads the total pulses from the kernel driver
 *   5. Calculates energy and cost
 *   6. Saves a small text report
 *
 * Formulas used:
 *   Energy (kWh)   = total pulses / 1000
 *   Cost (Rupees)  = Energy (kWh) * rate per kWh
 *
 * Compile:
 *   g++ -std=c++17 smart_energy_agent.cpp -o smart_energy_agent
 */

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>     // open(), close()
#include <unistd.h>    // write(), read()
#include <sys/ioctl.h> // ioctl()
#include <cerrno>      // errno

#include "smart_meter_ioctl.h"

/* ---- Simple settings the student can change ---------------------- */
const double PULSES_PER_KWH   = 1000.0;   // 1000 pulses = 1 kWh
const double RATE_PER_KWH    = 8.0;      // Rs 8 per kWh
const char   *REPORT_FILE    = "reports/energy_report.txt";

/* File descriptor of the opened device. -1 means "not open". */
int device_fd = -1;

/* ---- Function A: open /dev/smart_meter --------------------------- */
bool open_device()
{
    device_fd = open(SMART_METER_DEV_PATH, O_RDWR);
    if (device_fd < 0) {
        std::cout << "ERROR: cannot open " << SMART_METER_DEV_PATH << "\n";
        std::cout << "Load the driver first:  sudo insmod smart_meter.ko\n";
        return false;
    }
    std::cout << "Connected to " << SMART_METER_DEV_PATH << "\n";
    return true;
}

/* ---- Function B: close the device ------------------------------- */
void close_device()
{
    if (device_fd >= 0) {
        close(device_fd);
        device_fd = -1;
    }
}

/* ---- Function C: get pulse count from driver using ioctl -------- *
 * ioctl() asks the driver for the current pulse counter value.      */
unsigned long get_pulses_from_driver()
{
    unsigned long pulses = 0;
    if (ioctl(device_fd, SM_IOCTL_GET_PULSES, &pulses) < 0) {
        std::cout << "ERROR: ioctl GET_PULSES failed\n";
        return 0;
    }
    return pulses;
}

/* ---- Function D: reset pulse count using ioctl ------------------ */
void reset_pulses_in_driver()
{
    if (ioctl(device_fd, SM_IOCTL_RESET) < 0) {
        std::cout << "ERROR: ioctl RESET failed\n";
    } else {
        std::cout << "Meter reset done. Pulse counter is 0.\n";
    }
}

/* ---- Function E: send simulated pulses to the driver ----------- *
 * We build the number as text and write() it to the device.         *
 * The driver reads this number and adds it to its counter.          */
bool send_pulses_to_driver(int number_of_pulses)
{
    if (number_of_pulses <= 0) {
        std::cout << "Please enter a number greater than 0.\n";
        return false;
    }

    std::string text = std::to_string(number_of_pulses);
    ssize_t written = write(device_fd, text.c_str(), text.size());

    if (written < 0) {
        std::cout << "ERROR: cannot write pulses to the driver\n";
        return false;
    }

    std::cout << "Sent " << number_of_pulses << " simulated pulse(s) to the driver.\n";
    return true;
}

/* ---- Function F: energy and cost calculation -------------------- *
 * Energy (kWh)  = pulses / 1000                                      *
 * Cost          = Energy * Rs 8                                       */
double pulses_to_energy(unsigned long pulses)
{
    return pulses / PULSES_PER_KWH;
}

double energy_to_cost(double energy_kwh)
{
    return energy_kwh * RATE_PER_KWH;
}

/* ---- Function G: print the full meter status -------------------- */
void show_status()
{
    unsigned long pulses  = get_pulses_from_driver();
    double        energy  = pulses_to_energy(pulses);
    double        cost    = energy_to_cost(energy);

    std::cout << "\n----- METER STATUS -----\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Total Pulses            : " << pulses << "\n";
    std::cout << "Energy Consumption (kWh): " << energy << "\n";
    std::cout << "Rate per kWh (Rs)       : " << RATE_PER_KWH << "\n";
    std::cout << "Estimated Cost (Rs)     : " << cost << "\n";
    std::cout << "------------------------\n";
}

/* ---- Function H: save a simple text report ---------------------- */
void save_report()
{
    unsigned long pulses = get_pulses_from_driver();
    double energy = pulses_to_energy(pulses);
    double cost   = energy_to_cost(energy);

    std::ofstream file(REPORT_FILE);
    if (!file.is_open()) {
        std::cout << "ERROR: cannot write " << REPORT_FILE
                  << " (does the reports/ folder exist?)\n";
        return;
    }

    file << "========================================\n";
    file << " SMART ENERGY - METER REPORT\n";
    file << "========================================\n";
    file << std::fixed << std::setprecision(3);
    file << "Total Pulses            : " << pulses << "\n";
    file << "Energy Consumption (kWh): " << energy << "\n";
    file << "Rate per kWh (Rs)       : " << RATE_PER_KWH << "\n";
    file << "Estimated Cost (Rs)     : " << cost << "\n";
    file << "Formula used            : kWh = pulses / 1000\n";
    file << "========================================\n";

    file.close();
    std::cout << "Report saved to " << REPORT_FILE << "\n";
}

/* ---- Function I: print the menu ---------------------------------- */
void show_menu()
{
    std::cout << "\n========== SMART ENERGY AGENT ==========\n";
    std::cout << "1. Generate / Simulate Meter Pulse\n";
    std::cout << "2. Show Total Pulses\n";
    std::cout << "3. Show Energy Consumption\n";
    std::cout << "4. Show Estimated Electricity Cost\n";
    std::cout << "5. Reset Meter\n";
    std::cout << "6. Save Simple Report\n";
    std::cout << "7. Exit\n";
    std::cout << "=========================================\n";
    std::cout << "Enter your choice: ";
}

/* ---- Function J: ask user how many pulses to generate ------------ */
void generate_pulses()
{
    std::string input;
    std::cout << "How many pulses do you want to generate? ";

    if (!std::getline(std::cin, input)) {
        std::cout << "Invalid input.\n";
        return;
    }

    // atoi() converts the text "25" into the number 25
    int count = atoi(input.c_str());

    if (count <= 0) {
        std::cout << "Invalid number. Please type a positive number.\n";
        return;
    }

    send_pulses_to_driver(count);
}

/* ---- MAIN FUNCTION ----------------------------------------------- */
int main()
{
    int choice = 0;

    if (!open_device()) {
        return 1;      // cannot continue without the driver
    }

    while (choice != 7) {
        show_menu();
        if (!(std::cin >> choice)) {     // user typed something wrong
            std::cout << "Please type a number between 1 and 7.\n";
            std::cin.clear();
            std::cin.ignore();
            continue;
        }
        std::cin.ignore();              // remove the leftover newline

        switch (choice) {

        case 1:
            generate_pulses();
            break;

        case 2: {
            std::cout << "Total Pulses: " << get_pulses_from_driver() << "\n";
            break;
        }

        case 3: {
            unsigned long pulses = get_pulses_from_driver();
            std::cout << std::fixed << std::setprecision(3);
            std::cout << "Energy Consumption: "
                      << pulses_to_energy(pulses) << " kWh\n";
            break;
        }

        case 4: {
            unsigned long pulses = get_pulses_from_driver();
            double energy = pulses_to_energy(pulses);
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Estimated Cost: Rs " << energy_to_cost(energy) << "\n";
            break;
        }

        case 5:
            reset_pulses_in_driver();
            break;

        case 6:
            save_report();
            break;

        case 7:
            std::cout << "Exiting Smart Energy Agent. Goodbye!\n";
            break;

        default:
            std::cout << "Invalid choice. Please pick 1 to 7.\n";
        }
    }

    close_device();
    return 0;
}