#include "rtmp.h"

int main()
{
    system("chcp 65001");
    // 读取配置
    Config config = Config::read_config("config.json");

    // 查找匹配的网络接口
    std::string _interface = find_matching_interface(config.interface_name);
    if (_interface.empty())
    {
        std::cerr << "找不到匹配的网络接口: " << config.interface_name << std::endl;
        return 1;
    }

    std::cout << "使用网络接口: " << _interface << std::endl;

    // 打开网络接口
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_live(_interface.c_str(), 65536, 1, 1000, errbuf);
    if (handle == nullptr)
    {
        std::cerr << "无法打开设备 " << _interface << ": " << errbuf << std::endl;
        return 1;
    }

    // 设置BPF过滤器
    struct bpf_program fp;
    if (pcap_compile(handle, &fp, config.display_filter.c_str(), 0, 0xffffffff) == -1)
    {
        std::cerr << "无法解析过滤器: " << pcap_geterr(handle) << std::endl;
        pcap_close(handle);
        return 1;
    }

    if (pcap_setfilter(handle, &fp) == -1)
    {
        std::cerr << "无法安装过滤器: " << pcap_geterr(handle) << std::endl;
        pcap_close(handle);
        return 1;
    }

    // 提取服务器地址和推流码
    std::string server, code;
    extract_server_and_code(handle, server, code);
    pcap_close(handle);

    if (!server.empty() && !code.empty())
    {
        std::cout << "服务器地址: " << server << std::endl;
        std::cout << "推流码: " << code << std::endl;

        // 修改OBS配置并启动
        if (!config.obs_config_path.empty())
        {
            modify_obs_config(config.obs_config_path, server, code);
        }

        if (!config.obs_path.empty())
        {
            start_obs(config.obs_path);
        }
    }
    else
    {
        std::cerr << "未能成功提取服务器地址和推流码" << std::endl;
        return 1;
    }

    return 0;
}