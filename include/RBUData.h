//
// Created by kirill3266 on 19.05.24.
//

#ifndef DIPLOM_RBUDATA_H
#define DIPLOM_RBUDATA_H

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <regex>
#include <atomic>
#include <complex>

class RBUData {

    std::atomic<double> m_DUT1{}; // delta UT1 time
    std::atomic<bool> m_stop = false; // Variable declaring m_DUT1 update stop

    std::string m_url{"https://datacenter.iers.org/eopOfToday/eopoftoday.php"}; // Link in which DUT1 will be parsed
    std::regex m_regex{R"((-?)(0|[1-9][0-9]*)\.?([1-9]+|0[1-9]+)?)"}; // Regex with which DUT1 will be parsed

    int m_get_index = 0; // Number of got packets
    int m_sample_rate; // Transmitter sample_rate
    int m_amplitude = 127; // Signal amplitude
    double m_modulation_index; // Phase modulation index
    double m_frequency_1; // Lower frequency
    double m_frequency_2; // Higher frequency
    std::vector<std::complex<int>> m_subvector_1; // Lower frequency samples vector
    std::vector<std::complex<int>> m_subvector_2; // Higher frequency samples vector
    int m_data[60][2] = {{}}; // RBU time code data

    // Function making 80ms time samples
    [[nodiscard]] std::vector<double> makeTime() const;

    // Function making PM samples at given frequency
    std::vector<double> makePhaseSamples(double t_frequency);

    // Function to update m_DUT1 from network
    // Can be time-consuming (from 300ms up to some seconds)
    void updateDUT();

    // Function converting DUT1 to Unary encoding
    // bool for DUT sign, DUT idx, dUT idx
    std::tuple<bool, int, bool, int> convertDUT();

    // Function calculating truncated julian day
    static int calculateTJD(std::chrono::time_point<std::chrono::system_clock> &t_tp);

    // Function for generating RBU time code
    void generateData();

public:

    RBUData() = delete;

    RBUData(int t_sample_rate, double t_modulation_index,
            double t_frequency_1,
            double t_frequency_2);

    void setup();

    // Function for getting html content with http request
    static std::string getHttpContent(const std::string &t_url);

    // Function for factoring decimal to components from vector
    static std::vector<int> decimalFactoring(int t_value, std::vector<int> &&t_vector);

    // Return I/Q components according to RBU specification
    std::tuple<int, int> getData();

    // Function for m_DUT1 update cycle start
    void startCycle();

    // Function for m_DUT1 update cycle stop
    void setCycleStop();

    ~RBUData() = default;

};


#endif //DIPLOM_RBUDATA_H
