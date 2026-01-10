#include "dpi_bypasser.h"

DPIBypasser::DPIBypasser(std::string_view listener_filter)
    : filter_(listener_filter)
{
    handle_ = WinDivertOpen(filter_.data(), WINDIVERT_LAYER_NETWORK, 0, 0);
    if (handle_ == INVALID_HANDLE_VALUE) {
        int i = static_cast<int>(GetLastError());
        if (i == 1) {
            std::cout << "Incorrect filter\n";
        }
        else if (i == 2) {
            std::cout << "WinDivert.sys not installed\nRun windivert_install.bat as admin";
        }
        else if (i == 5) {
            std::cout << "Admin rights requred...\n";
        }
        else if (i == 1060) {
            std::cout << "WinDivert is not installed\nRun windivert_install.bat with admin rights";
        }
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        std::terminate();
    }
}

DPIBypasser::DPIBypasser() {
    handle_ = WinDivertOpen(filter_.data(), WINDIVERT_LAYER_NETWORK, 0, 0);
    if (handle_ == INVALID_HANDLE_VALUE) {
        std::cout << GetLastError() << std::endl;
        throw std::runtime_error("Should run as admin or incorrect filer or smth else idk");
    }
}

DPIBypasser::~DPIBypasser() {
    WinDivertClose(handle_);
}


void DPIBypasser::Start() {
    while (true) {
        if (!RecvPacket()) {
            continue;
        }

        ParseHeaders();
        if (!ValidatePacket()) {
            continue;
        }

        if (auto method = IsNeedToByPass()) {
            Bypass(*method);
            continue;
        }

        if (!WinDivertSend(handle_, packet_.data(), packet_len_, NULL, &addr_)) {
            std::cerr << "Error sending packet: " << GetLastError() << std::endl;
        }
        ++packet_count_;
    }
}

void DPIBypasser::AddBypassRequiredHostname(
    const std::string_view hostname,
    const BypassMethod method) {
    resource_to_bypass_method_[std::string(hostname)] = method;
}

void DPIBypasser::SetFakeSNIRepeats(UINT8 repeats) {
    sni_fake_repeats_ = repeats;
}

void DPIBypasser::PrintCurrentPacket() {
    std::cout << "[" << packet_count_ << "]"
        << "[size=" << packet_len_ << "]"
        << "[Direction=" << (addr_.Outbound ? "OUT" : "IN") << "]"
        << "[TTL=" << static_cast<int>(iphdr_->TTL)
        << "][ID=" << iphdr_->Id
        << "][SrcIP=" << IpToString(iphdr_->SrcAddr)
        << "][SrcPort=" << htons(tcphdr_->SrcPort)
        << "][DstIP=" << IpToString(iphdr_->DstAddr)
        << "][DstPort=" << htons(tcphdr_->DstPort) << "]\n";
}

void DPIBypasser::ActualizePacketHeaders(char* packet,
    UINT16 ip_id_increment,
    UINT seq_num_increment,
    UINT len) {
    PWINDIVERT_IPHDR iphdr = reinterpret_cast<PWINDIVERT_IPHDR>(packet);
    PWINDIVERT_TCPHDR tcphdr = reinterpret_cast<PWINDIVERT_TCPHDR>(packet + 20);
    SetLength(iphdr, len);
    IncrementIPID(iphdr, ip_id_increment);
    IncrementSeqNum(tcphdr, seq_num_increment);
}

RaiiPacket DPIBypasser::GlueTogether(char* first, char* second, UINT first_len, UINT second_len) {
    RaiiPacket result(first_len + second_len);
    result.append(first, 0, first_len);
    result.append(second, first_len, second_len);
    return result;
}

void DPIBypasser::SimpleSNIFake() {
    SendFakeSni(1);
    SendPacket(packet_.data(), packet_len_);
}

void DPIBypasser::SimpleSNISplit() {
    SendSplitPacketPayload({2});
}

void DPIBypasser::MultiSNIFake() {
    SendFakeSni(sni_fake_repeats_);
    SendPacket(packet_.data(), packet_len_);
}

void DPIBypasser::SniFakeAndFakedSplit() {
    SendFakeSni(sni_fake_repeats_);
    auto garbage_mask = GetCurrentCapturedPacketHeaders();
    auto split_packet_part0 = GetCurrentCapturedPacketHeaders();
    split_packet_part0.append(packet_.data() + data_offset_, data_offset_, 2);

    SendMaskedPacket(garbage_mask.data(), data_offset_ + 2,
        split_packet_part0.data(), data_offset_ + 2, 0);

    auto mask = GetCurrentCapturedPacketHeaders();
    auto splited_data_part1 = GetCurrentCapturedPacketHeaders();
    splited_data_part1.append(packet_.data() + data_offset_ + 2, data_offset_, packet_len_ - 2);

    SendMaskedPacket(mask.data(), packet_len_, splited_data_part1.data(), packet_len_ - 2, 2);
}

void DPIBypasser::SendMaskedPacket(
    char* mask,
    UINT mask_len,
    char* packet,
    UINT packet_len,
    UINT mask_seq_incr) {
    PWINDIVERT_IPHDR mask_iphdr = reinterpret_cast<PWINDIVERT_IPHDR>(mask);
    PWINDIVERT_IPHDR packet_iphdr = reinterpret_cast<PWINDIVERT_IPHDR>(packet);
    PWINDIVERT_TCPHDR mask_tcphdr = reinterpret_cast<PWINDIVERT_TCPHDR>(mask + 20);
    PWINDIVERT_TCPHDR packet_tcphdr = reinterpret_cast<PWINDIVERT_TCPHDR>(packet + 20);
    SetTimeStamp(mask_tcphdr, -600000);
    SetLength(mask_iphdr, mask_len);
    SetLength(packet_iphdr, packet_len);
    IncrementSeqNum(mask_tcphdr, mask_seq_incr);
    IncrementSeqNum(packet_tcphdr, mask_seq_incr);
    IncrementIPID(mask_iphdr, 1);

    for (int i = 0; i < 6; ++i) {
        SendPacket(mask, mask_len, true, false);
    }

    IncrementIPID(packet_iphdr, 1);
    SendPacket(packet, packet_len, true, false);
    IncrementIPID(mask_iphdr, 2);

    for (int i = 0; i < 6; ++i) {
        SendPacket(mask, mask_len, true, false);
    }
}

void DPIBypasser::IncrementIPID(PWINDIVERT_IPHDR iphdr, UINT16 increment) {
    iphdr->Id = htons(ntohs(iphdr->Id) + increment);
}

void DPIBypasser::IncrementSeqNum(PWINDIVERT_TCPHDR tcphdr, UINT32 increment) {
    tcphdr->SeqNum = htonl(ntohl(tcphdr->SeqNum) + increment);
}

void DPIBypasser::SetLength(PWINDIVERT_IPHDR iphdr, UINT new_len) {
    iphdr->Length = htons(static_cast<UINT16>(new_len));
}

RaiiPacket DPIBypasser::GetCurrentCapturedPacketHeaders() {
    RaiiPacket garbage_packet;
    memcpy(garbage_packet.data(), packet_.data(), data_offset_);
    return garbage_packet;
}

RaiiPacket DPIBypasser::GetCurrentCapturedPacketPayloadPart(UINT bytes) {
    RaiiPacket payload_part;
    memcpy(payload_part.data(), packet_.data() + data_offset_, bytes);
    return payload_part;
}

RaiiPacket DPIBypasser::GetPacketSnipet(char* packet, UINT from, UINT count) {
    RaiiPacket snipet(count);
    memcpy(snipet.data(), packet + from, count);
    return snipet;
}

bool DPIBypasser::RecvPacket() {
    if (!WinDivertRecv(handle_, packet_.data(), PACKET_SIZE, &packet_len_, &addr_)) {
        std::cerr << "Recv error: " << GetLastError() << std::endl;
        return false;
    }

    if (packet_len_ < VALIDATE_FILTER) {
        SendPacket(packet_.data(), packet_len_, false);
        return false;
    }
    return true;
}

bool DPIBypasser::ValidatePacket() {
    if (!iphdr_ || !tcphdr_ || !payload_) {
        SendPacket(packet_.data(), packet_len_, false);
        return false;
    }

    UINT16 ip_hdr_len = iphdr_->HdrLength * WORD_WEIGHT;
    UINT16 tcp_hdr_len = tcphdr_->HdrLength * WORD_WEIGHT;
    data_offset_ = ip_hdr_len + tcp_hdr_len;

    return true;
}

void DPIBypasser::Bypass(BypassMethod bypass_method) {
    switch (bypass_method) {
    case BypassMethod::NON:
        SendPacket(packet_.data(), packet_len_);
        return;
    case BypassMethod::SIMPLE_SNI_FAKE:
        SimpleSNIFake();
        return;
    case BypassMethod::MULTI_SNI_FAKE:
        MultiSNIFake();
        return;
    case BypassMethod::SSF_FAKED_SPLIT:
        SniFakeAndFakedSplit();
        return;
    case BypassMethod::SIMPLE_SNI_SPLIT:
        SimpleSNISplit();
        return;
        //...
    }
}

void DPIBypasser::ParseHeaders() {
    WinDivertHelperParsePacket(
        packet_.data(), packet_len_,
        &iphdr_,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &tcphdr_,
        nullptr,
        &payload_,
        &payload_len_,
        nullptr,
        nullptr
    );
}

std::string DPIBypasser::IpToString(UINT32 ip) {
    return std::to_string((ip >> 0) & 0xFF) + '.' +
        std::to_string((ip >> 8) & 0xFF) + '.' +
        std::to_string((ip >> 16) & 0xFF) + '.' +
        std::to_string((ip >> 24) & 0xFF);
}

void DPIBypasser::SetTimeStamp(PWINDIVERT_TCPHDR tcphdr, INT new_ts) {
    const UINT ts_type = 8;
    const UINT ts_len = 10;
    const UINT ts_value_offset = 2;
    const UINT NOP = 1;

    UINT tcphdr_len = tcphdr->HdrLength * WORD_WEIGHT;
    UINT pos = sizeof(WINDIVERT_TCPHDR);
    char* tcphdr_bytes = (char*)tcphdr;
    while (pos + 1 < tcphdr_len) {
        if (tcphdr_bytes[pos] == NOP) {
            ++pos;
            continue;
        }
        if (tcphdr_bytes[pos] == ts_type && tcphdr_bytes[pos + 1] == ts_len) {
            UINT32* ts_value_ptr = (UINT32*)(tcphdr_bytes + pos + ts_value_offset);
            UINT32 ts_value = ntohl(*ts_value_ptr);

            std::cout << "TSVALUE = " << ts_value << std::endl;
            *ts_value_ptr = htonl(ts_value + new_ts);
            return;
        }
        pos += tcphdr_bytes[pos + 1];
    }
}

std::optional<BypassMethod> DPIBypasser::IsNeedToByPass() {
    unsigned char* payload_ptr = static_cast<unsigned char*>(payload_);

    if (payload_ptr[HANDSHAKE_POS] == HANDSHAKE &&
        payload_ptr[TLS_MAJOR_POS] == TLS_MAJOR &&
        (payload_ptr[TLS_VERSION_POS] == TLS_1X || payload_ptr[TLS_VERSION_POS] == TLS_MAJOR) &&
        payload_ptr[CLIENT_HELLO_POS] == CLIENT_HELLO) {

        std::cout << ">>> Client Hello detected!\n";
        PrintCurrentPacket();

        if (UINT sni_pos = FindSniOffset(payload_ptr, payload_len_)) {
            UINT hostname_len = (payload_ptr[sni_pos - 2] << 8) | payload_ptr[sni_pos - 1];
            std::string hostname(reinterpret_cast<char*>(payload_ptr + sni_pos), hostname_len);
            std::cout << "Detected SNI: " << hostname << std::endl;
            if (auto it = resource_to_bypass_method_.find(hostname);
                it != resource_to_bypass_method_.end()) {
                return it->second;
            }

            for (const auto& [resource, method] : resource_to_bypass_method_) {
                if (hostname.find(resource) != std::string::npos) {
                    return method;
                }
            }

        }
    }
    return std::nullopt;
}

UINT DPIBypasser::FindSniOffset(unsigned char* tls_data, UINT tls_len) {
    if (tls_len < SNI_MIN_SIZE) return 0;
    UINT pos = NONCONST_DATA_START;

    if (pos + SESSION_ID_LEN_SIZE > tls_len) return 0;

    UINT8 session_id_len = tls_data[pos];
    pos += SESSION_ID_LEN_SIZE + session_id_len;

    if (pos + CIPHER_SUITES_LEN_SIZE > tls_len) return 0;

    UINT16 cipher_suites_len = (tls_data[pos] << 8) | tls_data[pos + 1];
    pos += CIPHER_SUITES_LEN_SIZE + cipher_suites_len;

    if (pos + COMPRESSION_METHODS_LEN_SIZE > tls_len)  return 0;

    UINT8 compression_methods_len = tls_data[pos];
    pos += COMPRESSION_METHODS_LEN_SIZE + compression_methods_len;

    if (pos + EXTENTIONS_LEN_SIZE > tls_len) return 0;

    UINT16 extentions_len = (tls_data[pos] << 8) | tls_data[pos + 1];
    pos += EXTENTIONS_LEN_SIZE;

    UINT extentions_end = pos + extentions_len;

    while (pos + EXTENTION_HEADER_SIZE <= extentions_end) {
        if (pos + EXTENTION_HEADER_SIZE > tls_len) return 0;

        UINT16 extention_type = (tls_data[pos] << 8) | tls_data[pos + 1];
        if (pos + EXTENTION_HEADER_SIZE + 2 > tls_len) return 0;

        UINT16 extention_len = (tls_data[pos + EXTENTIONS_LEN_SIZE] << 8) |
            tls_data[pos + EXTENTIONS_LEN_SIZE + 1];

        if (extention_type == SNI_EXTENTION) {
            UINT sni_pos = pos + EXTENTION_HEADER_SIZE + EXTENTION_TYPE_SIZE
                + EXTENTIONS_LEN_SIZE + SNI_LIST_LEN_SIZE;

            if (sni_pos + 2 > tls_len) {
                return 0;
            }
            return sni_pos;
        }
        pos += EXTENTION_HEADER_SIZE + extention_len;
    }
    return 0;
}

bool DPIBypasser::SendPacket(char* packet, UINT packet_len, bool recalc_check, bool damage_checksum) {
    if (recalc_check) {
        if (!WinDivertHelperCalcChecksums(packet, packet_len, &addr_, 0)) {
            std::cerr << "Failed to calculate checksums: " << GetLastError() << std::endl;
            return false;
        }
        else {
            std::cout << "CheckSum recalculated success\n";
        }
        if (damage_checksum) {
            PWINDIVERT_TCPHDR tcp_hdr = (PWINDIVERT_TCPHDR)(packet + MINIMUM_IPV4HDR_SIZE);
            tcp_hdr->Checksum = htons(ntohs(tcp_hdr->Checksum) - 1);
        }
    }

    if (!WinDivertSend(handle_, packet, packet_len, NULL, &addr_)) {
        std::cerr << "WinDivertSend failed: " << GetLastError() << std::endl;
        return false;
    }
    return true;
}
void DPIBypasser::SendSplitPacketPayload(const std::vector<UINT>& cut_marks) {
    UINT last_mark = 0;
    UINT16 id_increment = 0;
    for (UINT mark : cut_marks) {
        RaiiPacket packet_part = GetCurrentCapturedPacketHeaders();
        packet_part.append(packet_.data() + data_offset_ + last_mark, data_offset_, mark);
        ActualizePacketHeaders(packet_part.data(), id_increment, last_mark, data_offset_ + mark);
        SendPacket(packet_part.data(), data_offset_ + mark, true, false);
        last_mark = mark;
        ++id_increment;
    }
    
    RaiiPacket packet_part = GetCurrentCapturedPacketHeaders();
    packet_part.append(packet_.data() + data_offset_ + last_mark, data_offset_, packet_len_ - last_mark);
    ActualizePacketHeaders(packet_part.data(), id_increment, last_mark, packet_len_ - last_mark);
    SendPacket(packet_part.data(), packet_len_ - last_mark, true, false);
}

void DPIBypasser::SendFakeSni(int repeats) {
    RaiiPacket fake_client_hello;

    memcpy(fake_client_hello.data(), packet_.data(), data_offset_);

    std::ifstream fake_tls("tls_clienthello_www_google_com.bin", std::ios::binary);
    if (!fake_tls) {
        throw std::runtime_error("Can't find tls_clienthello_www_google_com.bin");
    }

    fake_tls.seekg(0, std::ios::end);
    UINT16 fake_packet_len = static_cast<UINT16>(fake_tls.tellg());
    fake_tls.seekg(std::ios::beg);
    {
        RaiiPacket buffer;
        fake_tls.read(buffer.data(), fake_packet_len);
        memcpy(fake_client_hello.data() + data_offset_, buffer.data(), fake_packet_len);
    }

    PWINDIVERT_IPHDR fake_iphdr = (PWINDIVERT_IPHDR)fake_client_hello.data();
    PWINDIVERT_TCPHDR fake_tcphdr = (PWINDIVERT_TCPHDR)(fake_client_hello.data() + 20);
    fake_iphdr->Length = htons(data_offset_ + fake_packet_len);

    SetTimeStamp(fake_tcphdr, -600000);
    for (int i = 0; i < repeats; ++i) {
        SendPacket(fake_client_hello.data(), data_offset_ + fake_packet_len, true, false);
    }
}

RaiiPacket::RaiiPacket(RaiiPacket&& other) noexcept {
    packet_ = other.packet_;
    other.packet_ = nullptr;
}

RaiiPacket::RaiiPacket() {
    packet_ = new char[PACKET_SIZE] {};
}

RaiiPacket::RaiiPacket(UINT size) {
    packet_ = new char[size] {};
}

RaiiPacket::~RaiiPacket() {
    delete[] packet_;
}

char* RaiiPacket::data() {
    return packet_;
}

void RaiiPacket::append(char* src, UINT pos, UINT count) {
    memcpy(packet_ + pos, src, count);
}
