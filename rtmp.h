#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <codecvt>
// #include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <pcap.h>
#include <nlohmann/json.hpp>
#include <fstream>

// 配置结构体
class Config
{
    public:
        std::string display_filter;
        std::string interface_name;
        std::string obs_path;
        std::string obs_config_path;

        static Config read_config(const std::string& config_file)
        {
            std::ifstream file(config_file);
            if (!file.is_open()) {
                std::cerr << "无法打开配置文件: " << config_file << std::endl;
                exit(1);
            }

            nlohmann::json data;
            file >> data;

            Config config;
            config.display_filter = data.value("display_filter", "tcp port 1935");
            config.interface_name = data.value("_interface", "Realtek Gaming 2.5GbE Family Controller");
            config.obs_path = data.value("obs_path", "");
            config.obs_config_path = data.value("obs_config_path", "");

            return config;
        }
};

// 将宽字符串转换为UTF-8字符串
std::string wstring_to_utf8(const std::wstring& wstr);
// 获取所有网络接口的友好名称和实际名称
std::vector<std::pair<std::string, std::string>> get_network_interfaces();
// 查找匹配的网络接口
std::string find_matching_interface(const std::string& interface_name);
// 在字符串中查找目标子串并返回包含该子串的单词
std::string filter_strings(const std::string& input_str, const std::string& target_str);
// 从RTMP数据包中提取信息
void extract_rtmp_info(const u_char* packet, u_int length, std::string& server, std::string& code);
// 从包中提取服务器地址和推流码
void extract_server_and_code(pcap_t* handle, std::string& server, std::string& code);
// 修改OBS配置文件
void modify_obs_config(const std::string& file_path, const std::string& server, const std::string& code);
// 启动OBS
void start_obs(const std::string& path);