#include <iostream>
#include <fstream>
#include <json/json.h>
#include <variant>
#include <algorithm>
#include <unordered_map>
#include <any>
#include "base64.h"
#include "proto3_parser.h"

Json::Value read_json_packet(const std::string& json_name)
{
	std::ifstream ifs(json_name);
	Json::Value root;
	ifs >> root;
    ifs.close();
	return root;
}

Json::Value all_serial = read_json_packet("packet_serialization.json");
Json::Value ucn = read_json_packet("ucn_serialization.json");


std::pair<uint64_t, uint8_t> varint(int now_location, const std::string& byte_str)
{
    uint8_t offset = 0;
    uint64_t data = byte_str[now_location] & 0b1111111; // 待优化，要是头铁真塞一个uint64的数据，那结果就错了
    while (true) {
        if (byte_str[now_location] >> 7) {
            offset += 1;
            now_location += 1;
            data = ((byte_str[now_location] & 0b1111111) << (7 * offset)) | data;
        }
        else {
            break;
        }
    }
    return { data, offset };
}

int judge_type(const std::string& prop_name)
{
    std::vector<std::string> zero = { "int32", "int64", "uint32", "uint64", "sint32", "sint64", "bool", "enum" };
    std::vector<std::string> one = { "fixed64", "sfixed64", "double" };
    std::vector<std::string> five = { "fixed32", "sfixed32", "float" };
    if (std::find(zero.begin(), zero.end(), prop_name) != zero.end()) {
        return 0;
    }
    else if (std::find(one.begin(), one.end(), prop_name) != one.end()) {
        return 1;
    }
    else if (std::find(five.begin(), five.end(), prop_name) != five.end()) {
        return 5;
    }
    else {
        return 2;
    }
}

bool IsBigEndian()
{
	int i = 0x1243;
	char* ch = (char*) &i;
	return *ch == 0x12;
}

bool is_big_endian = IsBigEndian();

template <typename T>
T bytes_to_num_le(const std::string& byte_str, size_t offset)
{
    T value;
    std::memcpy(&value, byte_str.data() + offset, sizeof(T));
    if (is_big_endian)
    {
        const size_t size = sizeof(T);
        char* p = reinterpret_cast<char*>(&value);
        std::reverse(p, p + size);
    }
    return value;
}

std::variant<Json::Value, std::pair<int, Json::Value>> parse(const std::string& byte_str, const std::string& packet_id, const int& flag, Json::Value encoding_rules, Json::Value prop_names){
    Json::Value decode_data;
    if (flag == 0) {
        Json::Value store = all_serial[packet_id];
        if (store.isArray()) {
            encoding_rules = store[0];
            prop_names = store[1];
        }
    }
    int i = 0;
    while (i < byte_str.length()){ 
        int data_type;
        std::string data_id;
        uint8_t offset;
        if (flag == 3) {
            data_type = judge_type(encoding_rules["1"].asString());
            data_id = "1";
        }
        else {
            data_type = byte_str[i] & 0b111;
            //std::pair<uint64_t, uint8_t> varint_res = varint(i, byte_str);
            std::pair<uint64_t, uint8_t> varint_res = varint(0, byte_str.substr(i, 10));
            data_id = std::to_string(varint_res.first >> 3);
            i += varint_res.second;
            i += 1;
        }
        if (encoding_rules.isMember(data_id)){
            if (data_type == 0){
                //std::pair<uint64_t, uint8_t> varint_res = varint(i, byte_str);
                std::pair<uint64_t, uint8_t> varint_res = varint(0, byte_str.substr(i, 10));
                uint64_t data = varint_res.first;
                offset = varint_res.second;
                if (encoding_rules[data_id].isArray()) {
                    if (encoding_rules[data_id][0].asString() == "enum") {
                        Json::Value enum_encode = encoding_rules[data_id][1];
                        decode_data[prop_names[data_id].asString()] = enum_encode[std::to_string(data)].asString();
                    }
                }
                else {
                    std::string encode_type = encoding_rules[data_id].asString();
                    if (encode_type == "bool") {
                        decode_data[prop_names[data_id].asString()] = bool(data);
                    }
                    else if (encode_type == "enum") {
                        Json::Value enum_encoding_rules = prop_names[data_id];
                        std::string enum_name = enum_encoding_rules.begin().name();
                        Json::Value enum_prop = *enum_encoding_rules.begin();
                        decode_data[enum_name] = enum_prop[std::to_string(data)].asString();
                    }
                    else {
                        decode_data[prop_names[data_id].asString()] = data;
                    }
                }
                i += offset;
                i += 1;
            }
            else if (data_type == 1){
                std::string encode_type = encoding_rules[data_id].asString();
                std::string prop_name = prop_names[data_id].asString();
                if (encode_type == "double") {
                    decode_data[prop_name] = bytes_to_num_le<double>(byte_str, i);
                }
                else if (encode_type == "sfixed64") {
                    uint64_t num = bytes_to_num_le<uint64_t>(byte_str, i);
                    int64_t data = (num >> 1) ^ - static_cast<int64_t>(num & 1);
                    decode_data[prop_name] = data;
                }
                else if (encode_type == "fixed64") {
                    decode_data[prop_name] = bytes_to_num_le<uint64_t>(byte_str, i);
                }
                i += 8;
            }
            else if (data_type == 5){
                std::string encode_type = encoding_rules[data_id].asString();
                std::string prop_name = prop_names[data_id].asString();
                if (encode_type == "float") {
                    decode_data[prop_name] = bytes_to_num_le<float>(byte_str, i);
                }
                else if (encode_type == "sfixed32") {
                    uint32_t num = bytes_to_num_le<uint32_t>(byte_str, i);
                    int32_t data = (num >> 1) ^ - static_cast<int32_t>(num & 1);
                    decode_data[prop_name] = data;
                }
                else if (encode_type == "fixed32") {
                    decode_data[prop_name] = bytes_to_num_le<uint32_t>(byte_str, i);;
                }
                i += 4;
            }
            else if (data_type == 2){
                /*std::pair<uint64_t, uint8_t> varint_res = varint(i, byte_str);*/
                std::pair<uint64_t, uint8_t> varint_res = varint(0, byte_str.substr(i, 10));
                offset = varint_res.second;
                uint64_t length = varint_res.first;
                i += offset;
                i += 1;
                if (encoding_rules[data_id].isString()){
                    std::string encode_type = encoding_rules[data_id].asString();
                    std::string prop_name;
                    if (encode_type == "string") {
                        prop_name = prop_names[data_id].asString();
                        decode_data[prop_name] = byte_str.substr(i, length);
                    }
                    else if (encode_type == "bytes") {
                        prop_name = prop_names[data_id].asString();
                        std::string sub_str = byte_str.substr(i, length);
                        decode_data[prop_name] = Base64::base64_encode(sub_str.c_str(), sub_str.size());
                    }
                    else if (encode_type.starts_with("repeated_")) {
                        std::string data_type = encode_type.substr(9); // repeated_
                        int j = i;
                        Json::Value repeated_encoding_rules;
                        Json::Value repeated_prop_names;
                        Json::Value enum_encoding_rules;
                        if (data_type == "enum") {
                            prop_name = prop_names[data_id][0].asString();
                            repeated_encoding_rules["1"] = "uint32";
                            enum_encoding_rules = prop_names[data_id][1];
                        }
                        else
                        {   
                            prop_name = prop_names[data_id].asString();
                            repeated_encoding_rules["1"] = data_type;
                        }
                        repeated_prop_names["1"] = "1";
                        
                        if (!decode_data.isMember(prop_name)) {
                            decode_data[prop_name] = Json::Value(Json::arrayValue);
                        }
                        if (data_type == "string") {
                            std::string sub_str = byte_str.substr(j, length);
                            decode_data[prop_name].append(sub_str);
                        }
                        else
                        {
                            while (j < i + length) {
                                std::pair<int, Json::Value> repeated_data = std::get<std::pair<int, Json::Value>>(parse(byte_str.substr(j, length), packet_id, 3, repeated_encoding_rules, repeated_prop_names));
                                int repeated_offset = repeated_data.first;
                                Json::Value repeated_data_value = repeated_data.second["1"];
                                if (data_type == "enum") {
                                    repeated_data_value = enum_encoding_rules[repeated_data_value.asString()];
                                }
                                decode_data[prop_name].append(repeated_data_value);
                                j += repeated_offset;
                            }
                        }
                    }
                }
                else if (encoding_rules[data_id].isObject()) {
                    if (encoding_rules[data_id].isMember("map")){
                        Json::Value type_map, map_private_prop_name;
                        type_map["1"] = encoding_rules[data_id]["map"][0];
                        type_map["2"] = encoding_rules[data_id]["map"][1];
                        std::string prop_name = prop_names[data_id].asString();
                        map_private_prop_name["1"] = "first";
                        map_private_prop_name["2"] = "second";
                        Json::Value map_data = std::get<Json::Value>(parse(byte_str.substr(i, length), packet_id, 1, type_map, map_private_prop_name));
                        if (!map_data.isMember("first")){
                            map_data["first"] =  0;
                        }
                        if (!map_data.isMember("second")){
                            map_data["second"] = 0;
                        }
                        if (!decode_data.isMember(prop_name)) {
                            decode_data[prop_name] = Json::Value(Json::objectValue);
                        }
                        decode_data[prop_name][map_data["first"].asString()] = map_data["second"];
                    }
                    else if (encoding_rules[data_id].isMember("repeated")){
                        std::string prop_name = prop_names[data_id].asString();
                        Json::Value repeat_encoding_rules = encoding_rules[data_id]["repeated"][0];
                        Json::Value repeat_prop_names = encoding_rules[data_id]["repeated"][1];
                        Json::Value repeat_data = std::get<Json::Value>(parse(byte_str.substr(i, length), packet_id, 1, repeat_encoding_rules, repeat_prop_names));
                        if (!decode_data.isMember(prop_name)) {
                            decode_data[prop_name] = Json::Value(Json::arrayValue);
                        }
                        decode_data[prop_name].append(repeat_data);
                    }
                }
                else if (encoding_rules[data_id].isArray()) {
                    Json::Value list_data = std::get<Json::Value>(parse(byte_str.substr(i, length), packet_id, 1, encoding_rules[data_id][0], encoding_rules[data_id][1]));
                    decode_data[prop_names[data_id].asString()] = list_data;
                }
                i += length;
            }
            if (flag == 3){
                return std::pair(i, decode_data);
            }
        }
    }
    return decode_data;
}

//using DataValue = std::variant<int, double, std::string, std::vector<std::any>, std::unordered_map<std::string, std::any>>;
//using DecodeData = std::unordered_map<std::string, DataValue>;

bool initialize = false;

void init(){
    for (auto& key : ucn.getMemberNames()) {
        all_serial[key] = ucn[key];
    }
}

void free_memory(char* memory) {
    delete[] memory;
}

const char* parser(const char* byte_str, int length, const char* packet_id) {
    if (not initialize){
        init();
        initialize = true;
    }
    std::string str = { byte_str, static_cast<size_t>(length) };
    std::variant parse_result = parse(str, packet_id, 0, {}, {});
    Json::Value json_result = std::get<Json::Value>(parse_result);
    std::string str_result = Json::FastWriter().write(json_result);
    auto reslen = str_result.length();
    auto res = new char[reslen+1];
    // Use memcpy to handle embedded nulls
    memcpy_s(res, reslen+1, str_result.data(), reslen);
    res[reslen] = '\0';
    return res;
}

//int main() {
//    //std::string byte_str = "";
//    std::ifstream file("long_string.bin", std::ios::binary);
//
//    std::stringstream buffer;
//    buffer << file.rdbuf();
//
//    std::string fileContent = buffer.str();
//    auto reslen = fileContent.length();
//    auto res = new char[reslen + 1];
//    // Use memcpy to handle embedded nulls
//    memcpy_s(res, reslen + 1, fileContent.data(), reslen);
//    res[reslen] = '\0';
//    const char* result = parser(res, reslen, "675");
//    //parser2(res, reslen, "675");
//    //std::cout << result << std::endl;
//    return 0;
//    
//    //std::cout << byte_str.length() << std::endl; 
//    //const char* result = parser(byte_str.c_str(), 114043, "675");
//    
//}
