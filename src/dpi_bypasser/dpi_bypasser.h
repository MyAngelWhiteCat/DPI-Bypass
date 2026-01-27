#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <Windows.h>
#include "windivert.h"


const size_t PACKET_SIZE = 20000;
const UINT8 WORD_WEIGHT = 4;
const UINT8 MIN_PAYLOAD_SIZE = 5;
const UINT8 SNI_MIN_SIZE = 50;
const UINT8 MINIMUM_IPV4HDR_SIZE = 20;

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
const size_t CLIENT_HELLO_PACKET_MINIMUM_SIZE = SNI_MIN_SIZE + MINIMUM_IPV4HDR_SIZE;


enum class BypassMethod {
    NON = 0,
    SIMPLE_SNI_FAKE = 1,
    SIMPLE_SNI_SPLIT = 2,
    MULTI_SNI_FAKE = 3,
    SSF_FAKED_SPLIT = 4,
    DEAD_FAKE = 5
    //...
};

class RaiiPacket {
public:
    RaiiPacket();
    RaiiPacket(UINT size);

    RaiiPacket(RaiiPacket&& other) noexcept;
    RaiiPacket(const RaiiPacket& other) = delete;

    ~RaiiPacket();
    RaiiPacket operator=(const RaiiPacket& other) = delete;

    char* data();
    void append(char* src, UINT pos, UINT count);

private:
    char* packet_;
};

class DPIBypasser {
public:
    DPIBypasser(std::string_view listener_filter);
    DPIBypasser();
    ~DPIBypasser();
    void Start();
    void AddBypassRequiredHostname(const std::string_view hostname, const BypassMethod method);
    void AddBypassIngnoreHostname(const std::string_view hostname);
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
    void SimpleSNISplit();
    void MultiSNIFake();
    void SniFakeAndFakedSplit();
    void DeadFake();

    UINT FindSniOffset(unsigned char* tls_data, UINT tls_len);
    RaiiPacket GetFakeSni();

    void IncrementSeqNum(PWINDIVERT_TCPHDR tcphdr, UINT32 increment);
    void IncrementIPID(PWINDIVERT_IPHDR iphdr, UINT16 increment);

    RaiiPacket GetCurrentCapturedPacketHeaders();
    RaiiPacket GetCurrentCapturedPacketPayloadPart(UINT bytes);
    RaiiPacket GetPacketSnipet(char* packet, UINT from, UINT to);

    void SetLength(PWINDIVERT_IPHDR iphdr, UINT new_len);
    void IncrementTimeStamp(PWINDIVERT_TCPHDR tcphdr, INT new_ts);

    void SendSplitPacketPayload(const std::vector<UINT>& cut_marks);
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

    
    std::string IpToString(UINT32 ip);
    void PrintCurrentPacket(std::ostream& out);
    void IncrementIdSeqNumAndSetLen(char* packet,
        UINT16 ip_id_increment,
        UINT seq_num_increment,
        UINT len);
    RaiiPacket GlueTogether(char* first, char* second, UINT first_len, UINT second_len);
};