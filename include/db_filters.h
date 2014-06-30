#ifndef DB_FILTERS_H_INCLUDED
#define DB_FILTERS_H_INCLUDED

#include <functional>
#include <string>
#include <vector>
#include <boost/format.hpp>

/**
 * Klasa przechowująca nazwę pola, nazwę wyświetlaną i filtr
 */
class FilterCols {
    public:
        typedef boost::function<std::string(const std::string&)> Filter;
        FilterCols(const std::string &name, const std::string &verbose, Filter filter)
            : name(name), verbose(verbose), filter(filter) {}

        std::string name;
        std::string verbose;
        Filter filter;
};

/**
 * Konwersja aby pozbyć się ostatniej kolumny (filtra)
 */
std::vector<DB::Column> conv(const std::vector<FilterCols> &cols)
{
    std::vector<DB::Column> res;
    for(auto c : cols)
        res.push_back(DB::Column(c.name, c.verbose));
    return res;
}

#define COLLAMBDA [](const std::string &val)

/**
 * Some basic lambdas for use in more than one place
 */
/// Leave unchanged
auto raw_field_ = COLLAMBDA{ return val != "" ? val : "-"; };

/// RRRRMMDD date to DD-MM-RRRR
auto birthday_ = COLLAMBDA{
    return (val.length() != 8) ? "nieznana" : (val.substr(6) + "-" + val.substr(4,2) + "-" + val.substr(0,4));
};

/// User's gender
auto gender_ = COLLAMBDA{
    return val == "1" ? "mężczyzna" : (val == "2" ? "kobieta" : "nieokreślona");
};

/// Convert from UNIX timestamp to normal date
auto timestamp_ = COLLAMBDA{
    if(val == "" or val == "0") return std::string("nieznany");
    return Poco::DateTimeFormatter::format(Poco::Timestamp(std::stoull(val) * 1000000), "%H:%M:%S %d/%n/%Y");
};

/// Convert specific UNIX timestamp to date (i.e. timestamp /60 to date)
auto spec_timestamp_ = COLLAMBDA{
    if(val == "" or val == "0") return std::string("nieznany");
    return timestamp_(std::to_string(std::stoull(val) * 60));
};

/// Account status
auto acc_status_ = COLLAMBDA{
    if(val == "") return "0";
    switch(std::stoi(val)) {
        case 1: return "niepodłączony";
        case 2: return "dostępny";
        case 3: return "zaraz wracam";
        case 5: return "nie przeszkadzać";
        case 6: return "niewidoczny";
    }
    return "#err";
};

/// Amount of time formatted as Xh Ymin Zs instead of just seconds
auto time_ = COLLAMBDA{
    if(val == "") return std::string("0");
    const auto h = std::stoi(val) / 3600, m = (std::stoi(val) % 3600) / 60, s = std::stoi(val) % 60;
    return (h != 0 ? std::to_string(h)+"h " : "") + (m != 0 ? std::to_string(m)+"min " : "") + (s != 0 ? std::to_string(s)+"s" : "");
};

/// Timezone in format GMT +/-X
auto timezone_ = COLLAMBDA{
    if(val == "") return std::string("-");
    else if(val == "93600") return std::string("czas komputera");
    else if(val == "86400" or val == "0") return std::string("GMT");
    auto tmp = std::stoi(val);
    return "GMT " +
        ( tmp < 86400 ? ("-" + std::to_string((86400 - tmp) / 3600)) : ("+" + std::to_string((tmp - 86400) / 3600)) );
};

/// Size of the file in human readable format
constexpr unsigned KILO = 1024;
constexpr auto convs = {"B", "KB", "MB", "GB", "TB"};
auto filesize_ = COLLAMBDA{
    auto vald = std::stod(val);
    int i;
    for(i = 0; vald > KILO; ++i)
        vald /= KILO;
    return str(boost::format("%.2f") % vald) + *(convs.begin() + i);
};

#endif // DB_FILTERS_H_INCLUDED
