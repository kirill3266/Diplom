#include "RBUData.h"
#include <iostream>
#include <iomanip>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>
#include <libhackrf/hackrf.h>

#define DEFAULT_DEVICE 0
#define FREQUENCY 433932500
#define AMPLITUDE 10
#define SUBFREQUENCY_1 100000
#define SUBFREQUENCY_2 312500
#define SAMPLE_RATE 10000000
#define AMP_ENABLE 0
#define UNDERRUN_LIMIT 10000
#define VGA_GAIN 20
#define M_IDX 0.698

int g_xfered_samples = 0;
int g_samples_to_xfer = 10 * SAMPLE_RATE; // 5 секунд передачи

RBUData g_data(SAMPLE_RATE, AMPLITUDE, M_IDX, SUBFREQUENCY_1, SUBFREQUENCY_2);

std::condition_variable g_cv;
std::mutex g_mutex;

int transfer_callback(hackrf_transfer *t_transfer) {
        auto *signed_buffer = reinterpret_cast<int8_t *>(t_transfer->buffer);
        std::tuple<int8_t, int8_t> tuple;
        for (int i = 0; i < t_transfer->buffer_length; i += 2) {
                tuple = g_data.getData();
                signed_buffer[i] = std::get<0>(tuple); // I component
                signed_buffer[i + 1] = std::get<1>(tuple); // Q component
        }
        t_transfer->valid_length = t_transfer->buffer_length;
        g_xfered_samples += t_transfer->buffer_length;
        if (g_xfered_samples >= g_samples_to_xfer) {
                return 1;
        }
        return 0;
}

void flush_callback(void *, int) {
        std::lock_guard<std::mutex> lk(g_mutex);
        g_cv.notify_all();
}

// TODO add starting at 0ms
int main() {
//        std::thread tr1(&RBUData::startCycle, &g_data);
//        std::tuple<int, int> data;
//        std::chrono::microseconds time = std::chrono::microseconds(0);
//        for (int i = 0; i < SAMPLE_RATE * 2; ++i) {
//                std::chrono::time_point<std::chrono::high_resolution_clock> tp1 = std::chrono::high_resolution_clock::now();
//                data = g_data.getData();
//                std::chrono::time_point<std::chrono::high_resolution_clock> tp2 = std::chrono::high_resolution_clock::now();
//                time = std::max(std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1),time);
////                std::cout << static_cast<int>(std::get<0>(data)) <<  " " << static_cast<int>(std::get<1>(data)) << std::endl;
//        }
//        g_data.setCycleStop();
//        tr1.join();
//        std::cout << "Max elapsed time: " << time << std::endl;

        int result;
        result = hackrf_init();
        if (result) {
                std::cerr << "hackrf_init() failed: " << hackrf_error_name(static_cast<hackrf_error>(result));
                return EXIT_FAILURE;
        }
        hackrf_device_list_t *device_list = hackrf_device_list();
        int count = device_list->devicecount;
        if (count < 1) {
                std::cerr << "Error! No usable devices found!";
                hackrf_exit();
                return EXIT_FAILURE;
        }
        std::cout << "Library release: " << hackrf_library_release() << std::endl;
        std::cout << "Library version: " << hackrf_library_version() << std::endl;
        if (count > 1)
                std::cout << "Found " << count << " boards" << std::endl;
        else std::cout << "Found " << count << " board" << std::endl;
        for (int i = 0; i < count; ++i) {
                std::cout << "Board " << i << ":" << std::endl;
                std::cout << "\t" << "Name: " << hackrf_usb_board_id_name(device_list->usb_board_ids[i]) << std::endl;
                std::cout << "\t" << "Serial №: " << device_list->serial_numbers[i] << std::endl;
        }
        std::cout << "Trying to open " << DEFAULT_DEVICE << " device " << std::endl;
        hackrf_device *device;
        result = hackrf_device_list_open(device_list, DEFAULT_DEVICE, &device);
        if (result) {
                std::cerr << "hackrf_device_list_open() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }
        uint8_t board_id;
        result = hackrf_board_id_read(device, &board_id);
        if (result) {
                std::cerr << "hackrf_board_id_read() failed: " << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }
        uint8_t board_rev;
        result = hackrf_board_rev_read(device, &board_rev);
        if (result) {
                std::cerr << "hackrf_board_rev_read() failed: " << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }
        char firmware_version[255];
        result = hackrf_version_string_read(device, &firmware_version[0], 255);
        if (result) {
                std::cerr << "hackrf_board_version_string_read() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }
        uint16_t api_version;
        result = hackrf_usb_api_version_read(device, &api_version);
        if (result) {
                std::cerr << "hackrf_usb_api_version_read() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }
        read_partid_serialno_t partid_serialno;
        result = hackrf_board_partid_serialno_read(device, &partid_serialno);
        if (result) {
                std::cerr << "hackrf_board_partid_serialno_read() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }
        std::cout << "Device info: " << std::endl;
        std::cout << "\t" << "Board ID Number: " << static_cast<int>(board_id) << " ("
                  << hackrf_board_id_name(static_cast<hackrf_board_id>(board_id)) << ")" << std::endl;
        std::cout << "\t" << "Firmware Version: " << firmware_version << " (API:" << ((api_version >> 8) & 0xFF) << "."
                  << std::setfill('0') << std::setw(2) << (api_version & 0xFF) << ")" << std::endl;
        std::cout << "\t" << "Part ID Number: 0x" << std::hex << std::setfill('0') << std::setw(8)
                  << partid_serialno.part_id[0] << " 0x" << std::hex << std::setfill('0') << std::setw(8)
                  << partid_serialno.part_id[1] << std::endl;
        std::cout << "\t" << "Board revision: " << hackrf_board_rev_name(static_cast<hackrf_board_rev>(board_rev))
                  << std::endl;
//        hackrf_device_list_free(device_list);

        // SETUP
        result = hackrf_set_freq(device, FREQUENCY);
        if (result) {
                std::cerr << "hackrf_set_freq() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }

        result = hackrf_set_sample_rate(device, SAMPLE_RATE);
        if (result) {
                std::cerr << "hackrf_set_sample_rate() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }

        result = hackrf_set_amp_enable(device, AMP_ENABLE);
        if (result) {
                std::cerr << "hackrf_set_amp_enable() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }
        result = hackrf_set_txvga_gain(device, VGA_GAIN);
        if (result) {
                std::cerr << "hackrf_set_txvga_gain() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }

        result = hackrf_set_tx_underrun_limit(device, UNDERRUN_LIMIT);
        if (result) {
                std::cerr << "hackrf_set_tx_underrun_limit() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }

        result = hackrf_enable_tx_flush(device, flush_callback, nullptr);
        if (result) {
                std::cerr << "hackrf_enable_tx_flush() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }
        std::thread th1(&RBUData::startCycle, &g_data);
        std::chrono::time_point<std::chrono::high_resolution_clock> tp1 = std::chrono::high_resolution_clock::now();
        result = hackrf_start_tx(device, transfer_callback, nullptr);
        if (result) {
                std::cerr << "hackrf_start_tx() failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }
        std::unique_lock<std::mutex> mlock(g_mutex);
        g_cv.wait(mlock); //wait for transfer to complete
        std::chrono::time_point<std::chrono::high_resolution_clock> tp2 = std::chrono::high_resolution_clock::now();
        result = hackrf_stop_tx(device);
        if (result) {
                std::cerr << "hackrf_stop_tx failed: "
                          << hackrf_error_name(static_cast<hackrf_error>(result));
                hackrf_device_list_free(device_list);
                hackrf_exit();
                return EXIT_FAILURE;
        }
        g_data.setCycleStop();
        th1.join();
        std::cout << "Elapsed time: " << std::dec << std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1)
                  << std::endl;

        std::cout << "Transferred " << g_xfered_samples << " bytes" << std::endl;
        hackrf_close(device);
        hackrf_device_list_free(device_list);
        hackrf_exit();
        return EXIT_SUCCESS;
}