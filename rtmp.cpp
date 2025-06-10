#include "rtmp.h"

using json = nlohmann::json;

// 将宽字符串转换为UTF-8字符串
std::string wstring_to_utf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// 获取所有网络接口的友好名称和实际名称
std::vector<std::pair<std::string, std::string>> get_network_interfaces()
{
    std::vector<std::pair<std::string, std::string>> interfaces;
    pcap_if_t *alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        std::cerr << "获取网络接口失败: " << errbuf << std::endl;
        return interfaces;
    }

    for (pcap_if_t *d = alldevs; d != nullptr; d = d->next)
    {
        std::string name = d->name ? d->name : "";
        std::string description = d->description ? d->description : "";

        // 如果描述为空，尝试使用其他方式获取友好名称
        if (description.empty())
        {
            // 使用Windows API获取适配器信息
            PIP_ADAPTER_INFO pAdapterInfo;
            PIP_ADAPTER_INFO pAdapter = NULL;
            DWORD dwRetVal = 0;
            ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

            pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
            if (pAdapterInfo == NULL)
            {
                continue;
            }

            if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
            {
                free(pAdapterInfo);
                pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
                if (pAdapterInfo == NULL)
                {
                    continue;
                }
            }

            if (dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen))
            {
                free(pAdapterInfo);
                continue;
            }

            for (pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next)
            {
                // 将适配器名称从ANSI转换为宽字符
                int wchars_num = MultiByteToWideChar(CP_ACP, 0, pAdapter->Description, -1, NULL, 0);
                wchar_t *wstr = new wchar_t[wchars_num];
                MultiByteToWideChar(CP_ACP, 0, pAdapter->Description, -1, wstr, wchars_num);

                // 将宽字符转换为UTF-8
                std::wstring wdesc(wstr);
                std::string utf8_desc = wstring_to_utf8(wdesc);
                delete[] wstr;

                // 检查名称是否匹配
                if (name.find(pAdapter->AdapterName) != std::string::npos)
                {
                    description = utf8_desc;
                    break;
                }
            }
            free(pAdapterInfo);
        }

        interfaces.push_back({name, description});
    }

    pcap_freealldevs(alldevs);
    return interfaces;
}

// 查找匹配的网络接口
std::string find_matching_interface(const std::string &interface_name)
{
    auto interfaces = get_network_interfaces();

    std::cout << "可用的网络接口:" << std::endl;
    for (const auto &iface : interfaces)
    {
        std::cout << "  名称: " << iface.first << std::endl;
        std::cout << "  描述: " << iface.second << std::endl;
        std::cout << "  -------------------------" << std::endl;
    }

    for (const auto &iface : interfaces)
    {
        // 检查描述是否包含目标接口名称
        if (!iface.second.empty() && iface.second.find(interface_name) != std::string::npos)
        {
            return iface.first;
        }

        // 检查名称是否包含目标接口名称
        if (!iface.first.empty() && iface.first.find(interface_name) != std::string::npos)
        {
            return iface.first;
        }
    }

    return "";
}

// 在字符串中查找目标子串并返回包含该子串的单词
std::string filter_strings(const std::string &input_str, const std::string &target_str)
{
    std::istringstream iss(input_str);
    std::string word;

    while (iss >> word)
    {
        // 去除单词两端的标点符号
        if (!word.empty() && ispunct(static_cast<unsigned char>(word.front())))
        {
            word = word.substr(1);
        }
        if (!word.empty() && ispunct(static_cast<unsigned char>(word.back())))
        {
            word.pop_back();
        }

        if (word.find(target_str) != std::string::npos)
        {
            return word;
        }
    }
    return "";
}

// 从RTMP数据包中提取信息
void extract_rtmp_info(const u_char *packet, u_int length, std::string &server, std::string &code)
{
    // 将数据包内容转换为字符串（仅包含可打印字符）
    std::string packet_str;
    for (u_int i = 0; i < length; i++)
    {
        if (isprint(packet[i]) && packet[i] != '\r' && packet[i] != '\n')
        {
            packet_str += packet[i];
        }
        else
        {
            packet_str += ' '; // 替换非可打印字符为空格
        }
    }

    // 提取服务器地址
    if (server.empty())
    {
        std::string server_tmp = filter_strings(packet_str, "rtmp://");
        if (!server_tmp.empty())
        {
            server = server_tmp;
        }
    }

    // 提取推流码
    if (code.empty())
    {
        std::string code_tmp = filter_strings(packet_str, "stream-");
        if (!code_tmp.empty())
        {
            // 去除可能的引号
            if (code_tmp.front() == '"' || code_tmp.front() == '\'')
            {
                code_tmp = code_tmp.substr(1);
            }
            if (code_tmp.back() == '"' || code_tmp.back() == '\'')
            {
                code_tmp.pop_back();
            }
            code = code_tmp;
        }
    }
}

// 从包中提取服务器地址和推流码
void extract_server_and_code(pcap_t *handle, std::string &server, std::string &code,int& flag,int timeout)
{
    struct pcap_pkthdr *header;
    const u_char *packet;
    int res;
    time_t start_time = time(nullptr);

    while ((res = pcap_next_ex(handle, &header, &packet)) >= 0)
    {
        if ((time(nullptr) - start_time > timeout)||flag==1)
        {
            // azh::logger()<<"Get RTMP timeout or user cancel, flag:"<<flag<<",timeout:"<<timeout;
            std::cerr << "The packet capture timed out or user cancel." << std::endl;
            break;
        }

        // 分析数据包内容
        extract_rtmp_info(packet, header->len, server, code);

        // 如果两者都已找到，则退出
        if (!server.empty() && !code.empty())
        {
            break;
        }
    }

    if (res == -1)
    {
        std::cerr << "An error occurred while reading the packet: " << pcap_geterr(handle) << std::endl;
    }
}

// 修改OBS配置文件
void modify_obs_config(const std::string &file_path, const std::string &server, const std::string &code)
{
    std::ifstream in_file(file_path);
    if (!in_file.is_open())
    {
        std::cerr << "can't open obs config: " << file_path << std::endl;
        return;
    }

    json data;
    in_file >> data;
    in_file.close();

    if (data["settings"].contains("server"))
    {
        data["settings"]["server"] = server;
    }
    else
    {
        std::cout << "- server field not exist in json" << std::endl;
    }

    if (data["settings"].contains("key"))
    {
        data["settings"]["key"] = code;
    }
    else
    {
        std::cout << "- key not exist in json" << std::endl;
    }

    std::ofstream out_file(file_path);
    if (!out_file.is_open())
    {
        std::cerr << "can't wirte in json file: " << file_path << std::endl;
        return;
    }

    out_file << data.dump(4);
    out_file.close();

    std::cout << "* edit obs Stream Ingest Code success!" << std::endl;
}

// 启动OBS
void start_obs(const std::string &path)
{
    if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        std::cout << "- file " << path << " not exist!" << std::endl;
        return;
    }

    // 提取目录路径
    size_t pos = path.find_last_of("\\/");
    std::string dir = (pos != std::string::npos) ? path.substr(0, pos) : "";

    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;

    if (CreateProcessA(
            NULL,                             // 应用程序名称
            const_cast<LPSTR>(path.c_str()),  // 命令行
            NULL,                             // 进程安全属性
            NULL,                             // 线程安全属性
            FALSE,                            // 句柄继承选项
            0,                                // 创建标志
            NULL,                             // 环境块
            dir.empty() ? NULL : dir.c_str(), // 当前目录
            &si,                              // STARTUPINFO
            &pi))
    { // PROCESS_INFORMATION
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        std::cout << "* start OBS: " << path << std::endl;
    }
    else
    {
        std::cerr << "* start OBS, but error : " << GetLastError() << std::endl;
    }
}