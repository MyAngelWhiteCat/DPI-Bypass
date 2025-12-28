#pragma once

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "windivert.h"

#include <optional>
#include <string_view>
#include <unordered_map>
#include <Windows.h>


const size_t PACKET_SIZE = 65535;
const UINT8 WORD_WEIGHT = 4;
const UINT8 MIN_PAYLOAD_SIZE = 5;
const UINT8 SNI_MIN_SIZE = 50;
const UINT MINIMUM_IPV4HDR_SIZE = 20;

const char HANDSHAKE = 0x16;
const size_t HANDSHAKE_POS = 0;

const char TLS_MAJOR = 0x03;
const size_t TLS_MAJOR_POS = 1;

const char TLS_1X = 0x01;
const size_t TLS_VERSION_POS = 2;

const char CLIENT_HELLO = 0x01;
const size_t CLIENT_HELLO_POS = 5;
const size_t CLIENT_HELLO_LEN = 3;

const size_t SESSION_ID_LEN_SIZE = 1;
const size_t CLIENT_RANDOM_LEN = 32;
const size_t CIPHER_SUITES_LEN_SIZE = 2;
const size_t COMPRESSION_METHODS_LEN_SIZE = 1;
const size_t EXTENTIONS_LEN_SIZE = 2;
const size_t NONCONST_DATA_START = 43;

const char SNI_EXTENTION = 0x0000;
const size_t EXTENTION_HEADER_SIZE = 4;
const size_t SNI_LIST_LEN_SIZE = 2;
const size_t EXTENTION_TYPE_SIZE = 1;


enum class BypassMethod {
    NON = 0,
    SIMPLE_SNI_FAKE = 1,
    MULTI_SNI_FAKE = 2,
    SSF_FAKED_SPLIT = 3
    //...
};

class RaiiPacket {
public:
    RaiiPacket(RaiiPacket&& other) noexcept;
    RaiiPacket(const RaiiPacket& other) = delete;
    RaiiPacket();
    ~RaiiPacket();
    RaiiPacket operator=(const RaiiPacket& other) = delete;

    char* data();

private:
    char* packet_;
};

class DPIBypasser {
public:
    DPIBypasser(std::string_view listener_filter);
    DPIBypasser();
    ~DPIBypasser();
    void Listen();
    void AddBypassRequiredHostname(const std::string_view hostname, const BypassMethod method);
    void SetFakeSNIRepeats(UINT8 repeats);

private:
    std::string filter_;
    HANDLE handle_{ nullptr };
    RaiiPacket packet_;
    UINT packet_count_ = 0;
    UINT packet_len_ = 0;
    WINDIVERT_ADDRESS addr_{ 0 };

    PWINDIVERT_IPHDR iphdr_{ nullptr };
    PWINDIVERT_TCPHDR tcphdr_{ nullptr };
    PVOID payload_{ nullptr };
    UINT payload_len_ = 0;
    UINT16 data_offset_ = 0;

    std::unordered_map<std::string, BypassMethod> resource_to_bypass_method_;
    UINT sni_fake_repeats_ = 6;

    bool RecvPacket();
    void ParseHeaders();
    bool ValidatePacket();
    std::optional<BypassMethod> IsNeedToByPass();
    void Bypass(BypassMethod bypass_method);

    void SimpleSNIFake();
    void MultiSNIFake();
    void SniFakeAndFakedSplit();

    UINT FindSniOffset(unsigned char* tls_data, UINT tls_len);

    void IncrementSeqNum(PWINDIVERT_TCPHDR tcphdr, UINT32 increment);
    void IncrementIPID(PWINDIVERT_IPHDR iphdr, UINT16 increment);

    RaiiPacket GetPacketHeaders();
    RaiiPacket GetPayloadPart(UINT bytes);

    void SetLength(PWINDIVERT_IPHDR iphdr, UINT new_len);
    void SetTimeStamp(PWINDIVERT_TCPHDR tcphdr, INT new_ts);

    void SendFakeSni(int repeats);
    void SendMaskedPacket(
        char* mask
        , UINT mask_len
        , char* packet
        , UINT packet_len
        , UINT mask_seq_incr);
    bool SendPacket(
        char* packet,
        UINT packet_len,
        bool recalc_check = true,
        bool damage_checksum = false);


    void Append(char* dst, char* src, UINT dst_size, UINT src_size);
    std::string IpToString(UINT32 ip);
    void PrintCurrentPacket();

};