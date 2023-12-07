#pragma once

#ifndef __PACKETLOGGER__
#define __PACKETLOGGER__
#include "MiscUtils.hpp"
#include <PcapFileDevice.h>
#include <RawPacket.h>
#include <functional>
#include <string>
#include <sys/time.h>

using namespace std;

class PacketLogger {

private:
    bool _file_opened = false;
    string _file_path;
    bool _is_pcap;
    bool _is_pcapng;
    bool _is_fifo_file;
    bool _is_append;
    int _default_link_type;
    bool *_enable_ptr = NULL;

    bool _add_pseudoheader;
    uint _pseudo_header_len;
    uint8_t _pseudo_header[32];
    uint8_t _PacketData[32768]; // Data buffer
    function<void(const string, bool)> post_log_callback = nullptr;
    function<void(const string)> pre_init_callback = nullptr;

    pcpp::PcapNgFileWriterDevice *_writerNG = nullptr;
    pcpp::PcapFileWriterDevice *_writer = nullptr;

    pcpp::RawPacket *_rawPacket;

    uint64_t pkt_counter = 0;

public:
    const char *TAG = "[PacketLogger] ";

    uint32_t logs_count;

    ~PacketLogger()
    {
        Close();
    }

    bool init(string file_path, int default_link_type = 1, bool append = false, bool fifo_file = false, bool *enable_ptr = NULL)
    {
        _file_path = "";
        _file_opened = false;
        _is_pcap = false;
        _is_pcapng = false;
        _is_append = append;
        _file_path = file_path;
        _is_fifo_file = fifo_file;
        _pseudo_header_len = 0;
        _add_pseudoheader = false;
        _default_link_type = default_link_type;
        _enable_ptr = enable_ptr;

        timeval time;
        // Initialize rawpacket structure with persistent packetdata buffer
        _rawPacket = new pcpp::RawPacket((const uint8_t *)_PacketData, 256, time, false);

        // Work on file extension
        string file_extension = string_file_extension(file_path);

        if (string_contains(file_extension, "pcapng") || string_contains(file_extension, "PCAPNG")) {
            _is_pcapng = true;
        }
        else if (string_contains(file_extension, "pcap") || string_contains(file_extension, "PCAP")) {
            _is_pcap = true;
        }
        else {
            _is_pcapng = true;
            _file_path += ".pcapng";
        }

        if (_is_fifo_file) {
            _file_path += ".fifo";

            mkfifo(_file_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
            _is_pcapng = true;
            _is_pcap = false;

            if (pre_init_callback)
                pre_init_callback(_file_path);

            _writerNG = new pcpp::PcapNgFileWriterDevice(_file_path.c_str());

            _file_opened = _writerNG->open(false);
        }
        else {
            // Open pcapng writter
            if (_is_pcapng) {
                _writerNG = new pcpp::PcapNgFileWriterDevice(_file_path.c_str());
                if (pre_init_callback)
                    pre_init_callback(_file_path);
                _file_opened = _writerNG->open(_is_append);
            }
            // Open pcap writter
            else if (_is_pcap) {
                _writer = new pcpp::PcapFileWriterDevice(_file_path.c_str(), (pcpp::LinkLayerType)_default_link_type);
                if (pre_init_callback)
                    pre_init_callback(_file_path);
                _file_opened = _writer->open(_is_append);
            }
        }

        return _file_opened;
    }

    void SetLinkType(int default_link_type)
    {
        _default_link_type = (pcpp::LinkLayerType)default_link_type;
    }

    void SetEnablePtr(bool *save_enable_var)
    {
        _enable_ptr = save_enable_var;
    }

    void Close()
    {
        _file_opened = false;
        if (_writerNG != nullptr && _writerNG->open()) {
            _writerNG->close();
            delete _writerNG;
            _writerNG = nullptr;
        }

        if (_writer != nullptr && _writer->open()) {
            _writer->close();
            delete _writer;
            _writer = nullptr;
        }
    }

    void writeLog(const string msg, bool error = false)
    {
        if (G_UNLIKELY(!_file_opened))
            return;

        if ((_enable_ptr != NULL) && (*_enable_ptr == false))
            return;

        ++logs_count;

        timeval time;
        gettimeofday(&time, NULL);
        uint s_size = msg.size() + 6;
        memcpy(_PacketData + 6, (uint8_t *)&msg[0], s_size - 6);
        _PacketData[4] = 0xA;
        if (error)
            _PacketData[5] = 0xFA;
        else
            _PacketData[5] = 0xFC;

        ++pkt_counter;
        _rawPacket->setRawData(_PacketData, s_size, time, (pcpp::LinkLayerType::LINKTYPE_BLUETOOTH_HCI_H4_WITH_PHDR));

        if (_is_pcapng) {
            _writerNG->writePacket(*_rawPacket);
            if (_is_fifo_file)
                _writerNG->flush();
        }
        else if (_is_pcap) {
            _writer->writePacket(*_rawPacket);
            if (_is_fifo_file)
                _writer->flush();
        }

        if (error)
            GL1(format("{}Packet Number:{}, Comment: {}", TAG, pkt_counter, msg));

        if (post_log_callback)
            post_log_callback(msg, error);

        
    }

    void write(uint8_t *buf, int buf_size, timeval &time, const char *comment = NULL)
    {
        if (G_UNLIKELY(!_file_opened))
            return;

        if ((_enable_ptr != NULL) && (*_enable_ptr == false))
            return;

        int total_size = buf_size + _pseudo_header_len;
        if (_pseudo_header_len)
            memcpy(_PacketData, _pseudo_header, _pseudo_header_len);
        memcpy(_PacketData + _pseudo_header_len, buf, buf_size);

        ++pkt_counter;
        _rawPacket->setRawData(_PacketData, total_size, time, (pcpp::LinkLayerType)_default_link_type);

        if (_is_pcapng) {
            if (comment != NULL)
                _writerNG->writePacket(*_rawPacket, comment);
            else
                _writerNG->writePacket(*_rawPacket);
            if (_is_fifo_file)
                _writerNG->flush();
        }
        else if (_is_pcap) {
            _writer->writePacket(*_rawPacket);
            if (_is_fifo_file)
                _writer->flush();
        }
    }

    void write(uint8_t *buf, int buf_size, const char *comment = NULL)
    {
        timeval time;
        gettimeofday(&time, NULL);

        write(buf, buf_size, time, comment);
    }

    void write(vector<uint8_t> &pkt, const char *comment = NULL)
    {
        timeval time;
        gettimeofday(&time, NULL);

        write(&pkt[0], pkt.size(), time, comment);
    }

    void write(vector<uint8_t> &pkt, timeval &time, const char *comment = NULL)
    {
        write(&pkt[0], pkt.size(), time, comment);
    }

    void write(fast_vector<uint8_t> &pkt, const char *comment = NULL)
    {
        timeval time;
        gettimeofday(&time, NULL);

        write(&pkt[0], pkt.size(), time, comment);
    }

    void write(fast_vector<uint8_t> &pkt, timeval &time, const char *comment = NULL)
    {
        write(&pkt[0], pkt.size(), time, comment);
    }

    void write(list_data<uint8_t> &pkt, const char *comment = NULL)
    {
        timeval time;
        gettimeofday(&time, NULL);

        write(&pkt[0], pkt.size(), time);
    }

    void write(list_data<uint8_t> &pkt, timeval &time, const char *comment = NULL)
    {
        write(&pkt[0], pkt.size(), time, comment);
    }

    void SetPseudoHeader(uint8_t *pseudo_hdr_buf, int pseudo_hdr_len)
    {
        memcpy(_pseudo_header, pseudo_hdr_buf, pseudo_hdr_len);
        _pseudo_header_len = pseudo_hdr_len;
        _add_pseudoheader = true;
    }

    void SetPseudoHeader(uint32_t val)
    {
        memcpy(_pseudo_header, (uint8_t *)&val, 4);
        _pseudo_header_len = 4;
        _add_pseudoheader = true;
    }

    void SetPseudoHeader(uint16_t val)
    {
        memcpy(_pseudo_header, (uint8_t *)&val, 2);
        _pseudo_header_len = 2;
        _add_pseudoheader = true;
    }

    void SetPseudoHeader(uint8_t val)
    {
        memcpy(_pseudo_header, (uint8_t *)&val, 1);
        _pseudo_header_len = 1;
        _add_pseudoheader = true;
    }

    void ClearPseudoHeader()
    {
        _add_pseudoheader = false;
    }

    void SetPostLogCallback(function<void(const string, bool)> fcn)
    {
        post_log_callback = fcn;
    }

    void SetPreInitCallback(function<void(const string)> fcn)
    {
        pre_init_callback = fcn;
    }

    void stop()
    {
        Close();
    }
};

#endif