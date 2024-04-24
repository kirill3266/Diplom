#include <iostream>
#include <iomanip>
#include <exception>
#include <libhackrf/hackrf.h>

#define DEFAULT_DEVICE 0

int main() {
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
        std::cerr << "hackrf_device_list_open() failed: " << hackrf_error_name(static_cast<hackrf_error>(result));
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
    hackrf_device_list_free(device_list);
//    hackrf_set_freq(device, 144500000);
//    hackrf_set_sample_rate(device, 10000000);
//    hackrf_set_amp_enable(device, 1);
//    hackrf_set_txvga_gain(device, 20);
//    hackrf_set_tx_underrun_limit(device, 100000); // new-ish library function, not always available
//    hackrf_enable_tx_flush(device, flush_callback, NULL);
//    hackrf_start_tx(device, transfer_callback, NULL);
//    pthread_mutex_lock(&mutex);
//    pthread_cond_wait(&cond, &mutex); // wait fo transfer to complete
//    hackrf_stop_tx(device);
//    hackrf_close(device);
    hackrf_exit();
    return EXIT_SUCCESS;
}