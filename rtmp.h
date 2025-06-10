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

#include <logger.hpp>

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
            if (!file.is_open())
            {
                std::cerr << "无法打开配置文件: " << config_file << std::endl;

                Config config;
                config.display_filter = "tcp port 1935";
                config.interface_name = "Realtek Gaming 2.5GbE Family Controller";
                config.obs_path = "";
                config.obs_config_path = "";

                return config;
            }

            nlohmann::json data;
            file >> data;

            Config config;
            config.display_filter = data.value("display_filter", "tcp port 1935");
            config.interface_name = data.value("interface", "Realtek Gaming 2.5GbE Family Controller");
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
void extract_server_and_code(pcap_t* handle, std::string& server, std::string& code,int& flag,int timeout=30);
// 修改OBS配置文件
void modify_obs_config(const std::string& file_path, const std::string& server, const std::string& code);
// 启动OBS
void start_obs(const std::string& path);

class rtmp
{
    public:
        Config config;
        std::vector<std::pair<std::string, std::string>> interfaces_name;
        std::string server, code;

        rtmp(const std::string& cfg_path="config.json")
        {
            config = Config::read_config(cfg_path);
            interfaces_name=get_network_interfaces();
        }

        bool getByInterface(const std::string& interface_name,int& flag,int timeout)
        {
            // azh::logger()<<"Start get RTMP timeout, flag:"<<flag<<",timeout:"<<timeout;
            // 打开网络接口
            char errbuf[PCAP_ERRBUF_SIZE];
            pcap_t *handle = pcap_open_live(interface_name.c_str(), 65536, 1, 1000, errbuf);
            if (handle == nullptr)
            {
                std::cerr << "无法打开设备 " << interface_name << ": " << errbuf << std::endl;
                return false;
            }

            // 设置BPF过滤器
            struct bpf_program fp;
            if (pcap_compile(handle, &fp, config.display_filter.c_str(), 0, 0xffffffff) == -1)
            {
                std::cerr << "无法解析过滤器: " << pcap_geterr(handle) << std::endl;
                pcap_close(handle);
                return false;
            }

            if (pcap_setfilter(handle, &fp) == -1)
            {
                std::cerr << "无法安装过滤器: " << pcap_geterr(handle) << std::endl;
                pcap_close(handle);
                return false;
            }

            extract_server_and_code(handle, server, code,flag,timeout);
            pcap_close(handle);

            return true;
        }

        Config getConfig() { return config; }
        std::string getServer() { return server; }
        std::string getCode() { return code; }
};