#ifndef __CP2P_UDT_H__
#define __CP2P_UDT_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

// ------------------------------------------------------------
// UDT Core Transport API
// ------------------------------------------------------------

namespace ncore
{
    namespace nudt
    {

        // -----------------------------------------------------------------------------
        // Packet descriptor (opaque to UDT)
        // -----------------------------------------------------------------------------
        struct packet_t
        {
            const void* m_data;
            u32         m_data_len;
        };

        // -----------------------------------------------------------------------------
        // Sequence map storage and operations
        // -----------------------------------------------------------------------------

        // Functional Behavior:
        // - It is allowed to push the same sequence multiple times into a map, but
        //   only the first push will have an effect. Subsequent pushes of the same
        //   sequence will return false.
        // - It is allowed to remove a sequence that is not present in the map, but
        //   this will have no effect.
        // - It is allowed to remove the same sequence multiple times, but only the
        //   first remove will have an effect.

        // Sequence-map storage (these are not owned by UDT, but UDT will call ops on them)
        struct sequence_maps_t
        {
            void* m_tx_in_flight_map;
            void* m_tx_missing_map;
            void* m_rx_received_map;
            void* m_rx_missing_map;
        };

        struct sequence_ops_t
        {
            // Returns true if the sequence was inserted, false if it was already present
            bool (*push)(void* map, u32 seq);

            // Removes a sequence from the map
            void (*remove)(void* map, u32 seq);

            // Removes all sequences in [start_seq, end_seq)
            void (*remove_range)(void* map, u32 start_seq, u32 end_seq);

            // Removes all sequences from the map, leaving it empty
            void (*remove_all)(void* map);

            // Returns true if the sequence is present in the map, false otherwise
            bool (*has)(const void* map, u32 seq);

            // Pops and returns a sequence from the map according to the implementation-defined
            // scheduling policy. Returns -1 if the map is empty.
            i32 (*pop)(void* map);

            // Merges the contents of src map into dst map. After this operation, src map
            // is left as is, and dst map contains all sequences that were present in either
            // dst or src before the merge.
            void (*merge)(void* dst, const void* src);

            // Returns the number of sequences currently in the map
            u32 (*size)(const void* map);

            // Serialize the map into a byte buffer for transmission (e.g. in ACK/NAK packets).
            // The exact format of the serialized map is implementation-defined.
            void (*serialize)(const void* map, u8* out_buf, u32* out_buf_len);

            // Deserialize a byte buffer back into the map format used by UDT.
            // The input buffer is expected to be in the format produced by serialize().
            void (*deserialize)(void* map, const u8* buf, u32 buf_len);
        };

        // -----------------------------------------------------------------------------
        // Packet construction and send callbacks
        // -----------------------------------------------------------------------------
        typedef bool (*build_data_fn)(void* user_ctx, u32 seq, packet_t* pkt_out);
        typedef bool (*build_ack_fn)(void* user_ctx, u32 ack_seq, u32 flow_window, packet_t* pkt_out);
        typedef bool (*build_nak_fn)(void* user_ctx, const void* missing_map, u32 flow_window, packet_t* pkt_out);
        typedef void (*send_packet_fn)(void* user_ctx, const packet_t* pkt);

        // Packet construction + transmission
        struct packet_ops_t
        {
            build_data_fn  m_build_data;
            build_ack_fn   m_build_ack;
            build_nak_fn   m_build_nak;
            send_packet_fn m_send_packet;
        };

        // -----------------------------------------------------------------------------
        // Congestion control operations
        // -----------------------------------------------------------------------------
        struct cc_ops_t
        {
            void (*on_packet_sent)(void* cc_ctx, u32 seq, u64 now_ts);
            void (*on_packet_received)(void* cc_ctx, u32 seq, u64 now_ts);
            void (*on_ack)(void* cc_ctx, u32 ack_seq, u64 now_ts);
            void (*on_loss)(void* cc_ctx, u32 seq, u32 hint_loss_count, u64 now_ts);
            void (*on_timeout)(void* cc_ctx, u64 now_ts);

            // Returns whether we are currently allowed to send a packet before hitting
            // congestion control limits.
            u32 (*budget_before_congestion)(void* cc_ctx);

            // If pacing is supported, returns the time until which we should wait before
            // sending the next packet. If the current time (now_ts) is greater than or
            // equal to the returned timestamp, we are allowed to send a packet.
            u64 (*pacing_timeout_ts)(void* cc_ctx, u64 now_ts, u64 last_tx_ts);
        };

        // -----------------------------------------------------------------------------
        // TX state machine (latched)
        // -----------------------------------------------------------------------------
        enum tx_state_t
        {
            TX_STATE_ACTIVE,
            TX_STATE_WAIT,
        };

        // -----------------------------------------------------------------------------
        // UDT core instance
        // One instance corresponds to exactly one object generation
        // -----------------------------------------------------------------------------
        struct udt_t
        {
            // User context
            void* m_user_ctx;

            // Sequence-map operations
            sequence_ops_t*  m_seq_ops;
            sequence_maps_t* m_seq_maps;

            // Packet construction and sending
            packet_ops_t* m_pkt_ops;

            // Congestion control (used by TX)
            void*     m_cc_ctx;
            cc_ops_t* m_cc_ops;

            // -----------------------------------------------------------------
            // TX: Parameters + Timeout & time-tracking state
            // -----------------------------------------------------------------
            u32        m_tx_next_seq;        // Next sequence number to use for new data packets (not including retransmissions)
            u32        m_tx_last_acked;      // Cumulative ACK, all sequences below this have been ACKed
            tx_state_t m_tx_state;           // Whether we are currently able to send packets (active) or waiting for some event to allow us to send (wait)
            u32        m_tx_max_seq;         // Sequence-space limits
            u32        m_tx_flow_window;     // Flow control receiver's advertised window (receiver-authoritative)
            u64        m_tx_last_send_ts;    // Last time we send a data packet
            u64        m_tx_last_ack_ts;     // Last time an ACK was sent (for ACK pacing)
            u64        m_tx_exp_interval;    // retransmission timeout
            u64        m_tx_exp_timeout_ts;  // Next timestamp at which we should retransmit if no ACKs are received

            // ----------------------------
            // RX: Parameters + Timing (ACK / NAK / EXP)
            // ----------------------------
            u32 m_rx_highest_contig;  // Receiver-side state
            u32 m_rx_flow_window;     // Flow control (receiver-authoritative)

            u64 m_rx_last_progress_ts;  // last time receiver made progress
            u64 m_rx_last_ack_sent_ts;  // last ACK sent
            u64 m_rx_last_nak_sent_ts;  // last NAK sent

            u64 m_rx_ack_interval;  // ACK timer period
            u64 m_rx_nak_interval;  // NAK timer period
            u64 m_rx_exp_interval;  // EXP timer period (receiver expiration)
            u64 m_rx_immediate_nak_min_interval;
        };

        // -----------------------------------------------------------------------------
        // Core UDT entry points
        // -----------------------------------------------------------------------------
        struct udt_config_t
        {
            // TX parameters
            u32 m_tx_max_seq;
            u32 m_tx_initial_exp_interval;
            u32 m_tx_initial_flow_window;

            // RX parameters
            u32 m_rx_initial_flow_window;
            u64 m_rx_ack_interval;
            u64 m_rx_nak_interval;
            u64 m_rx_exp_interval;
            u64 m_rx_immediate_nak_min_interval;
        };
        void init_config(udt_config_t* config);

        void init(udt_t* udt, void* user_ctx, const packet_ops_t* pkt_ops, const sequence_ops_t* seq_ops, const sequence_maps_t* seq_maps, void* cc_ctx, const cc_ops_t* cc_ops, const udt_config_t* config);

        void tx_tick(udt_t* udt, u64 now_ts);
        u64  tx_next_tick_ts(udt_t* udt, u64 now_ts);
        void on_tx_ack_received(udt_t* udt, u32 ack_seq, u32 flow_window, u64 now_ts);
        void on_tx_nak_received(udt_t* udt, const void* nak_map, u32 flow_window, u64 now_ts);

        void rx_tick(udt_t* udt, u64 now_ts);
        u64  rx_next_tick_ts(udt_t* udt, u64 now_ts);
        void on_rx_data_received(udt_t* udt, u32 seq, u64 now_ts);

    }  // namespace nudt
}  // namespace ncore

#endif  // __CP2P_UDT_H__
