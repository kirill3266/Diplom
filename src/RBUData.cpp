//
// Created by kirill3266 on 19.05.24.
//

#include "RBUData.h"
#include <Poco/Net/HTTPStreamFactory.h>
#include <Poco/Net/HTTPSStreamFactory.h>
#include <Poco/Exception.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>
#include <Poco/URIStreamOpener.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/InvalidCertificateHandler.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/SSLManager.h>
#include <chrono>
#include <vector>
#include <iostream>
#include <regex>
#include <cmath>
#include <mutex>
#include <numeric>
#include <numbers>

RBUData::RBUData(const int t_sample_rate, const double t_modulation_index,
                 const double t_frequency_1,
                 const double t_frequency_2) : m_sample_rate(t_sample_rate), m_frequency_1(t_frequency_1),
                                            m_frequency_2(t_frequency_2),
                                            m_modulation_index(t_modulation_index) {
}

void RBUData::setup() {
        std::vector<double> tmp = makePhaseSamples(m_frequency_1);
        m_subvector_1.resize(tmp.size());
        for (int i = 0; i < tmp.size(); ++i) {
                m_subvector_1[i].real(static_cast<int>(m_amplitude * std::sin(m_modulation_index * tmp[i])));
                m_subvector_1[i].imag(static_cast<int>(-m_amplitude * std::cos(m_modulation_index * tmp[i])));
        }
        tmp = makePhaseSamples(m_frequency_2);
        m_subvector_2.resize(tmp.size());
        for (int i = 0; i < tmp.size(); ++i) {
                m_subvector_2[i].real(static_cast<int>(m_amplitude * std::sin(m_modulation_index * tmp[i])));
                m_subvector_2[i].imag(static_cast<int>(-m_amplitude * std::cos(m_modulation_index * tmp[i])));
        }
        generateData();
}

std::vector<double> RBUData::makeTime() const {
        std::vector<double> tmp(m_sample_rate / 1000 * 80);
        std::iota(tmp.begin(), tmp.end(), 0);
        for (auto &i: tmp) i /= m_sample_rate;
        return tmp;
}

std::vector<double> RBUData::makePhaseSamples(const double t_frequency) {
        std::vector<double> t = makeTime();
        std::vector<double> tmp(t.size());
        for (int i = 0; i < t.size(); ++i)
                tmp[i] = std::sin(2 * std::numbers::pi * t_frequency * t[i]);
        return tmp;
}

std::string RBUData::getHttpContent(const std::string &t_url) {
        std::string content{};
        try {
                auto &opener = Poco::URIStreamOpener::defaultOpener();
                auto uri = Poco::URI{t_url};
                auto is = std::shared_ptr<std::istream>{opener.open(uri)};
                Poco::StreamCopier::copyToString(*(is), content);
        } catch (Poco::Exception &e) {
                std::cerr << e.displayText() << std::endl;
        }
        return content;
}

void RBUData::updateDUT() {

        Poco::Net::SSLManager::InvalidCertificateHandlerPtr ptr_handler(new Poco::Net::AcceptCertificateHandler(false));
        Poco::Net::Context::Ptr ptr_context(new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, ""));
        Poco::Net::SSLManager::instance().initializeClient(nullptr, ptr_handler, ptr_context);

        std::string content = getHttpContent(m_url);

        std::vector<std::string> matches;
        std::regex_token_iterator<std::string::iterator> send;

        std::regex_token_iterator<std::string::iterator> it(content.begin(), content.end(), m_regex);

        while (it != send) matches.push_back(*it++);

        m_DUT1 = std::stod(matches.back()) / 1000;
}

std::tuple<bool, int, bool, int> RBUData::convertDUT() {
        bool sign_DUT = false, sign_dUT = false; // true means - sign
        int idx_DUT = 0;
        int idx_dUT = 0;
        double DUT1 = m_DUT1;
        double eps = 0.001;
        double DUT = std::round(DUT1 * 10) * 0.1 == -0 ? 0 : std::round(DUT1 * 10) * 0.1;
        double dUT = std::round(DUT1 * 50) * 0.02 - DUT;
        if (DUT < 0.0)
                sign_DUT = true;
        if (dUT < 0.0)
                sign_dUT = true;
        for (int i = 0; i < 9; ++i) {
                double diff = std::abs(DUT) - 0.1 * i;
                if (diff < eps) {
                        idx_DUT = i;
                        break;
                }
        }
        for (int i = 0; i < 5; ++i) {
                double diff = std::abs(dUT) - 0.02 * i;
                if (diff < eps) {
                        idx_dUT = i;
                        break;
                }
        }
        return std::tuple<bool, int, bool, int>{sign_DUT, idx_DUT, sign_dUT, idx_dUT};
}

std::vector<int> RBUData::decimalFactoring(const int t_value, std::vector<int> &&t_vector) {
        std::vector<int> res(t_vector.size(), 0);
        int sum = 0;
        std::vector<int>::iterator iter;
        while (sum < t_value) {
                iter = std::upper_bound(t_vector.begin(), t_vector.end(), t_value - sum) - 1;
                sum += *iter;
                res[std::distance(t_vector.begin(), iter)] = 1;
        }
        return res;
}

int RBUData::calculateTJD(std::chrono::time_point<std::chrono::system_clock> &t_tp) {
        std::chrono::time_point<std::chrono::system_clock> tp = t_tp;
        auto dp = std::chrono::floor<std::chrono::days>(tp);
        std::chrono::year_month_day ymd{dp};
        std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::milliseconds>(tp - dp)};
        int a = (14 - static_cast<int>(static_cast<unsigned int>(ymd.month()))) / 12;
        int y = static_cast<int>(ymd.year()) + 4800 - a;
        int m = static_cast<int>(static_cast<unsigned int>(ymd.month())) + 12 * a - 3;
        int JDN =
                static_cast<int>(static_cast<unsigned int>(ymd.day())) + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 +
                y / 400 - 32045;
        double JD = static_cast<double>(JDN) + (static_cast<double>(hms.hours().count()) - 12) / 24 +
                    static_cast<double>(hms.minutes().count()) / 1440 +
                    static_cast<double>(hms.seconds().count()) / 86400;
        return static_cast<int>(std::floor(JD - 2440000.5)) % 10000;
}

void RBUData::generateData() {
        auto [sign_DUT, idx_DUT, sign_dUT, idx_dUT] = convertDUT();
        std::chrono::time_point<std::chrono::system_clock> tp = std::chrono::system_clock::now(); //UTC time
        auto dp = std::chrono::floor<std::chrono::days>(tp);
        std::chrono::year_month_day ymd{dp};
        std::chrono::weekday wd{ymd};
        std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::milliseconds>(tp - dp)};
        std::vector<int> year, month, day, hour, minute, weekday;
        std::vector<int> TJD = decimalFactoring(calculateTJD(tp), std::vector<int>{
                {1, 2, 4, 8, 10, 20, 40, 80, 100, 200, 400, 800, 1000, 2000, 4000, 8000}});

        year = decimalFactoring(static_cast<int>(ymd.year()), std::vector<int>{1, 2, 4, 8, 10, 20, 40, 80});
        month = decimalFactoring(static_cast<int>(static_cast<unsigned>(ymd.month())),
                                 std::vector<int>{1, 2, 4, 8, 10});
        weekday = decimalFactoring(static_cast<int>(wd.iso_encoding()),
                                   std::vector<int>{1, 2, 4});
        day = decimalFactoring(static_cast<int>(static_cast<unsigned>(ymd.day())),
                               std::vector<int>{1, 2, 4, 8, 10, 20});
        hour = decimalFactoring(static_cast<int>(hms.hours().count()),
                                std::vector<int>{1, 2, 4, 8, 10, 20});
        minute = decimalFactoring(static_cast<int>(hms.minutes().count()),
                                  std::vector<int>{1, 2, 4, 8, 10, 20, 40});
        m_data[0][0] = 1; // Always 1
        m_data[0][1] = 1; // Always 1
        m_data[1][0] = 0; // Always 0
        m_data[2][0] = 0; // Always 0
        if (!sign_DUT) {
                for (int i = 0; i < idx_DUT; ++i) {
                        m_data[1 + i][1] = 1;
                }
                for (int i = 0; i < idx_dUT; ++i) {
                        m_data[11 + i][0] = 1;
                }
                if (sign_dUT)
                        m_data[15][0] = 1;
        } else {
                for (int i = 0; i < idx_dUT; ++i) {
                        m_data[3 + i][0] = 1;
                }
                if (sign_dUT)
                        m_data[7][0] = 1;
                for (int i = 0; i < idx_DUT; ++i) {
                        m_data[9 + i][1] = 1;
                }
        }
//        m_data[22][0] = 1; // UTC+3
//        m_data[23][0] = 1; // UTC+3
        for (int i = 0; i < 16; ++i) // TJD
                m_data[33 - i][1] = TJD[i];
        for (int i = 0; i < 8; ++i)
                m_data[32 - i][0] = year[i];
        for (int i = 0; i < 5; ++i)
                m_data[37 - i][0] = month[i];
        for (int i = 0; i < 3; ++i)
                m_data[40 - i][0] = weekday[i];
        for (int i = 0; i < 6; ++i)
                m_data[46 - i][0] = day[i];
        for (int i = 0; i < 6; ++i)
                m_data[52 - i][0] = hour[i];
        for (int i = 0; i < 7; ++i)
                m_data[59 - i][0] = minute[i];

        // Calculating checksum
        m_data[49][1] = (m_data[18][1] + m_data[19][1] + m_data[20][1] + m_data[21][1] + m_data[22][1] + m_data[23][1] +
                         m_data[24][1] + m_data[25][1]) % 2;
        m_data[50][1] = (m_data[26][1] + m_data[27][1] + m_data[28][1] + m_data[29][1] + m_data[30][1] + m_data[31][1] +
                         m_data[32][1] + m_data[33][1]) % 2;
        m_data[53][1] =
                (m_data[19][0] + m_data[20][0] + m_data[21][0] + m_data[22][0] + m_data[23][0] + m_data[24][0]) % 2;
        m_data[54][1] = (m_data[25][0] + m_data[26][0] + m_data[27][0] + m_data[28][0] + m_data[29][0] + m_data[30][0] +
                         m_data[31][0] + m_data[32][0]) % 2;
        m_data[55][1] = (m_data[33][0] + m_data[34][0] + m_data[35][0] + m_data[36][0] + m_data[37][0] + m_data[38][0] +
                         m_data[39][0] + m_data[40][0]) % 2;
        m_data[56][1] =
                (m_data[41][0] + m_data[42][0] + m_data[43][0] + m_data[44][0] + m_data[45][0] + m_data[46][0]) % 2;
        m_data[57][1] =
                (m_data[47][0] + m_data[48][0] + m_data[49][0] + m_data[50][0] + m_data[51][0] + m_data[52][0]) % 2;
        m_data[58][1] = (m_data[53][0] + m_data[54][0] + m_data[55][0] + m_data[56][0] + m_data[57][0] + m_data[58][0] +
                         m_data[59][0]) % 2;
}

std::tuple<int, int> RBUData::getData() {
        if (m_get_index == m_sample_rate * 60 ||
            m_get_index == 0) { // Reset get index and regenerate data before new minute start
                generateData();
                m_get_index = 0;
        }
        static int subvector_idx = 0;
        if (subvector_idx == m_sample_rate / 1000 * 80)
                subvector_idx = 0;
        int ms = (m_get_index % m_sample_rate) / (m_sample_rate / 1000); // ms in packets
        int ms_mod_100 = ms % 100; // RBU tenth second interval
        int ms_div_100 = ms / 100; // RBU second interval
        int s = m_get_index / m_sample_rate; // s in packets
        m_get_index++;
        if (ms_mod_100 < 10) { // 10ms interval
                return std::tuple<int, int>{0, -127}; // Transmit continious wave
        } else if (ms_mod_100 < 90) { // 80 ms interval
                switch (ms_div_100) { // 100ms in packets
                        case 0:
                                switch (m_data[s][0]) {
                                        case 0:
                                                return std::tuple<int, int>{m_subvector_1[subvector_idx].real(),
                                                                            m_subvector_1[subvector_idx++].imag()};
                                        case 1:
                                                return std::tuple<int, int>{m_subvector_2[subvector_idx].real(),
                                                                            m_subvector_2[subvector_idx++].imag()};
                                }
                        case 1:
                                switch (m_data[s][1]) {
                                        case 0:
                                                return std::tuple<int, int>{m_subvector_1[subvector_idx].real(),
                                                                            m_subvector_1[subvector_idx++].imag()};
                                        case 1:
                                                return std::tuple<int, int>{m_subvector_2[subvector_idx].real(),
                                                                            m_subvector_2[subvector_idx++].imag()};
                                }
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 6:
                                return std::tuple<int, int>{m_subvector_1[subvector_idx].real(),
                                                            m_subvector_1[subvector_idx++].imag()};
                        case 7:
                        case 8:
                                if (s == 59)
                                        return std::tuple<int, int>{m_subvector_2[subvector_idx].real(),
                                                                    m_subvector_2[subvector_idx++].imag()};
                                return std::tuple<int, int>{m_subvector_1[subvector_idx].real(),
                                                            m_subvector_1[subvector_idx++].imag()};
                        case 9:
                                return std::tuple<int, int>{m_subvector_2[subvector_idx].real(),
                                                            m_subvector_2[subvector_idx++].imag()};
                }
        } else if (ms_mod_100 < 95) { // first 5 ms interval
                return std::tuple<int, int>{0, -127};
        } else if (ms_mod_100 < 100) { // Second 5 ms interval
                return std::tuple<int, int>{0, 0};
        }
}

void RBUData::startCycle() {
        Poco::Net::HTTPStreamFactory::registerFactory();
        Poco::Net::HTTPSStreamFactory::registerFactory();
        Poco::Net::initializeSSL();
        while (!m_stop) {
                updateDUT();
                std::this_thread::sleep_for(std::chrono::seconds(5));
        }
}

void RBUData::setCycleStop() {
        m_stop = true;
}