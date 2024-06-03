#include "Location.h"

#include <sstream>
#include <iostream>
#include <curl/curl.h>

#include "rapidjson/document.h"

size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *buffer)
{
    buffer->append((char *)contents, size * nmemb);
    return size * nmemb;
}

bool RequestToIpInfo(std::string& read_buffer)
{
    const auto curl = curl_easy_init();
    if (!curl) {
        std::cerr << "RequestToIpInfo - failed to init curl" << std::endl;
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://ipinfo.io/json");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

    const auto res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "RequestToIpInfo - error while using curl: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    return true;
}

bool ParseJsonForLocationField(const std::string& read_buffer, std::string& location)
{
    rapidjson::Document json_response;
    json_response.Parse(read_buffer.c_str());

    if (json_response.HasParseError() || !json_response.HasMember("loc")) {
        std::cerr << "ParseJsonForLocationField - failed to parse response from ipinfo" << std::endl;
        return false;
    }

    const rapidjson::Value& loc = json_response["loc"];
    if (!loc.IsString()){
        std::cerr << "ParseJsonForLocationField - loc field is not a string";
        return false;
    }

    location = loc.GetString();

    return true;
}

void ParseLocationString(const std::string& location_string, projection::Epsg4326Point& epsg_4326_point)
{
    auto stream = std::istringstream(location_string);

    /* In other locales separator symbol ',' may be treated as a part
    of a number resulting in inability to read correctly */
    stream.imbue(std::locale("en_US.utf8"));

    stream >> epsg_4326_point.latitude;
    stream.ignore(); // Skip separator ','
    stream >> epsg_4326_point.longitude;
}

namespace location {

bool RequestDeviceLocation(projection::Epsg4326Point& epsg_4326_point)
{
    auto read_buffer = std::string();

    if(!RequestToIpInfo(read_buffer))
        return false;

    auto location_string = std::string();
    if(!ParseJsonForLocationField(read_buffer, location_string))
        return false;

    ParseLocationString(location_string, epsg_4326_point);

    return true;
}

} // namespace location
